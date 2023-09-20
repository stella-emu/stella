#include "RTEmulationWorker.hxx"

#include <stdexcept>
#include <time.h>
#include <sched.h>
#include <sstream>
#include <sys/sysinfo.h>

#include "TIA.hxx"
#include "Logger.hxx"
#include "DispatchResult.hxx"

#define XSTRINGIFY(x) #x
#define STRINGIFY(x) XSTRINGIFY(x)
#define TRACE_MSG(m) m " in " STRINGIFY(__FILE__) ":" STRINGIFY(__LINE__)

namespace {
  constexpr uint64_t TIMESLICE_NANOSECONDS = 100000;
  constexpr uint64_t MAX_LAG_NANOSECONDS = 10 * TIMESLICE_NANOSECONDS;

  inline Int64 timeDifferenceNanoseconds(const struct timespec& from, const struct timespec& to)
  {
    return (from.tv_sec - to.tv_sec) * 1000000000 + (from.tv_nsec - to.tv_nsec);
  }

  void configureScheduler() {
    struct sched_param schedParam = {.sched_priority = sched_get_priority_max(SCHED_FIFO)};
    if (sched_setscheduler(0, SCHED_FIFO, &schedParam) < 0) {
      ostringstream ss;
      ss << "failed to configure scheduler for realtime: " << errno;

      Logger::error(ss.str());
    }

    const int cores = get_nprocs();
    if (cores < 2) {
      Logger::error("failed to set scheduling affinity on emulation worker - not enough cores");
      return;
    }

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cores - 1, &cpuset);

    if (sched_setaffinity(0, sizeof(cpuset), &cpuset) < 0){
      ostringstream ss;
      ss << "failed to pin worker thread: " << errno;

      Logger::error(ss.str());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RTEmulationWorker::RTEmulationWorker()
{
  if (!myPendingSignal.is_lock_free())
    throw runtime_error(TRACE_MSG("atomic<Command> is not lock free"));

  if (!myState.is_lock_free())
    throw runtime_error(TRACE_MSG("atomic<State> is not lock free"));

  myThread = std::thread(&RTEmulationWorker::threadMain, this);

  // wait until the thread is running
  while (myState == State::initializing) {}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RTEmulationWorker::~RTEmulationWorker()
{
  while (myPendingSignal != Signal::none) {}

  // make sure that the main loop exits
  switch (myState) {
    // exception? the thread will terminate on its own
    case State::exception:
      break;

    // stopped? the thread is sleeping and needs to be woken
    case State::stopped:
      {
        // wait until the thread is sleeping before we wake it
        std::unique_lock<std::mutex> lock(myThreadIsRunningMutex);
        myPendingSignal = Signal::quit;
      }

      myWakeupCondition.notify_one();
      break;

    // If the thread is running we post quit. The thread *may* transition to
    // State::exception instead, but that's fine --- it will die either way.
    case State::running:
    case State::paused:
      myPendingSignal = Signal::quit;
      break;

    default:
      cerr << "FATAL: unable to terminate emulation worker" << endl << std::flush;
      abort();

      break;
  }

  myThread.join();

  rethrowPendingException();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RTEmulationWorker::suspend()
{
  while (myPendingSignal != Signal::none) {}

  if (myState == State::paused) return;
  if (myState != State::running) throw runtime_error(TRACE_MSG("invalid state"));

  myPendingSignal = Signal::suspend;

  // the thread may transition to State::exception instead, so make sure that we rethrow
  while (myState != State::paused) rethrowPendingException();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RTEmulationWorker::resume()
{
  while (myPendingSignal != Signal::none) {}

  if (myState == State::running) return;
  if (myState != State::paused) throw runtime_error(TRACE_MSG("invalid state"));

  myPendingSignal = Signal::resume;

  // the thread may transition to State::exception instead, so make sure that we rethrow
  while (myState != State::running) rethrowPendingException();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RTEmulationWorker::start(uInt32 cyclesPerSecond, DispatchResult* dispatchResult, TIA* tia)
{
  while (myPendingSignal != Signal::none) {}

  switch (myState) {
    // pending exception? throw
    case State::exception:
      rethrowPendingException();
      break;

    case State::stopped:
      break;

    // stop the thread if it is running
    case State::running:
    case State::paused:
      stop();
      break;

    default:
      throw runtime_error(TRACE_MSG("unreachable"));
      break;
  }

  myTia = tia;
  myDispatchResult = dispatchResult;
  myCyclesPerSecond = cyclesPerSecond;

  {
    // wait until the thread is sleeping before we wake it
    std::unique_lock<std::mutex> lock(myThreadIsRunningMutex);
    myPendingSignal = Signal::start;
  }

  myWakeupCondition.notify_one();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RTEmulationWorker::stop()
{
  while (myPendingSignal != Signal::none) {}

  switch (myState) {
    // exception? we throw below
    case State::exception:
      break;

    case State::stopped:
      break;

    // runinng or paused? send stop signal
    case State::running:
    case State::paused:
      myPendingSignal = Signal::stop;
      break;

    default:
      throw runtime_error(TRACE_MSG("unreachable"));
      break;
  }

  {
    // make sure that the thread either stops and sleeps or terminates after
    // an exception
    std::unique_lock<std::mutex> lock(myThreadIsRunningMutex);

    if (myState == State::exception) rethrowPendingException();
    if (myState != State::stopped) throw runtime_error(TRACE_MSG("invalid state"));
  }
}

void RTEmulationWorker::threadMain()
{
  configureScheduler();

  // this mutex is locked if and only if the thread is running
  std::unique_lock<std::mutex> lock(myThreadIsRunningMutex);

  try {
    while (myPendingSignal != Signal::quit) {
      // start stopped and sleeping
      myState = State::stopped;
      myPendingSignal = Signal::none;
      myWakeupCondition.wait(lock);

      switch (myPendingSignal) {
        case Signal::quit:
        case Signal::none:
        case Signal::stop:
          continue;

        case Signal::resume:
        case Signal::suspend:
          throw runtime_error(TRACE_MSG("invalid state"));

        case Signal::start:
          break;
      }

      // start? enter the emulation loop
      dispatchEmulation();
    }

    // this is not strictly necessary, but it keeps our states nice and tidy
    myState = State::quit;
  }
  catch (...) {
    // caught something? store away the exception and terminate
    myPendingException = std::current_exception();
    myState = State::exception;
  }

  myPendingSignal = Signal::none;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RTEmulationWorker::dispatchEmulation()
{
  struct timespec timeOffset;
  struct timespec timesliceStart;
  double virtualTimeNanoseconds = 0;

  clock_gettime(CLOCK_MONOTONIC, &timeOffset);

  myState = State::running;
  myPendingSignal = Signal::none;

  while (myState == State::running) {
    clock_gettime(CLOCK_MONOTONIC, &timesliceStart);

    uInt64 realTimeNanoseconds = timeDifferenceNanoseconds(timesliceStart, timeOffset);
    double deltaNanoseconds = realTimeNanoseconds - virtualTimeNanoseconds;

    // reset virtual clock if emulation lags
    if (deltaNanoseconds > MAX_LAG_NANOSECONDS) {
      virtualTimeNanoseconds = realTimeNanoseconds;
      deltaNanoseconds = 0.;
    }

    const Int64 cycleGoal = (deltaNanoseconds * myCyclesPerSecond) / 1000000000;
    Int64 cyclesTotal = 0;

    while (cycleGoal > cyclesTotal && myDispatchResult->isSuccess()) {
      myTia->update(*myDispatchResult, cycleGoal - cyclesTotal);
      cyclesTotal += myDispatchResult->getCycles();
    }

    virtualTimeNanoseconds += static_cast<double>(cyclesTotal) / static_cast<double>(myCyclesPerSecond) * 1000000000.;

    // busy wait and handle pending signals
    struct timespec now;
    do {
      switch (myPendingSignal) {
        // quit or stop? -> exit function and leave signal handling to the main loop
        case Signal::quit:
        case Signal::stop:
          return;

        case Signal::none:
          continue;

        case Signal::resume:
          myState = State::running;
          myPendingSignal = Signal::none;
          break;

        case Signal::suspend:
          myState = State::paused;
          myPendingSignal = Signal::none;
          break;

        case Signal::start:
          throw new runtime_error(TRACE_MSG("invalid state"));
      }

      clock_gettime(CLOCK_MONOTONIC, &now);
    } while (timeDifferenceNanoseconds(now, timesliceStart) < static_cast<Int64>(TIMESLICE_NANOSECONDS) || myState == State::paused);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RTEmulationWorker::rethrowPendingException()
{
    if (myState == State::exception && myPendingException) {
    const std::exception_ptr ex = myPendingException;
    myPendingException = nullptr;

    std::rethrow_exception(ex);
  }
}
