#include "RTEmulationWorker.hxx"

#include <stdexcept>
#include <time.h>

#include "TIA.hxx"
#include "DispatchResult.hxx"

namespace {
  constexpr uint64_t TIMESLICE_NANOSECONDS = 100000;
  constexpr uint64_t MAX_LAG_NANOSECONDS = 10 * TIMESLICE_NANOSECONDS;

  Int64 timeDifferenceNanoseconds(const struct timespec& from, const struct timespec& to)
  {
    return (from.tv_sec - to.tv_sec) * 1000000000 + (from.tv_nsec - to.tv_nsec);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RTEmulationWorker::RTEmulationWorker()
{
  if (!myPendingSignal.is_lock_free()) {
    cerr << "FATAL: atomic<Command> is not lock free" << endl << std::flush;
    throw runtime_error("atomic<Command> is not lock free");
  }

  if (!myState.is_lock_free()) {
    cerr << "FATAL: atomic<State> is not lock free" << endl << std::flush;
    throw runtime_error("atomic<State> is not lock free");
  }

  myThread = std::thread(
    &RTEmulationWorker::threadMain, this
  );

  while (myState == State::initializing) {}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RTEmulationWorker::~RTEmulationWorker()
{
  while (myPendingSignal != Signal::none) {}

  switch (myState) {
    case State::exception:
      break;

    case State::stopped:
      {
        std::unique_lock<std::mutex> lock(myThreadIsRunningMutex);
        myPendingSignal = Signal::quit;
      }

      myWakeupCondition.notify_one();
      break;

    case State::running:
    case State::paused:
      myPendingSignal = Signal::quit;
      break;

    case State::initializing:
      throw runtime_error("unreachable");
      break;
  }

  myThread.join();

  rethrowPendingException();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RTEmulationWorker::suspend()
{
  while (myPendingSignal != Signal::none) {}

  if (myState != State::running || myState != State::paused) throw runtime_error("invalid state");

  myPendingSignal = Signal::suspend;

  while (myState != State::paused) rethrowPendingException();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RTEmulationWorker::resume()
{
  while (myPendingSignal != Signal::none) {}

  if (myState != State::running || myState != State::paused) throw runtime_error("invalid state");

  myPendingSignal = Signal::resume;

  while (myState != State::running) rethrowPendingException();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RTEmulationWorker::start(uInt32 cyclesPerSecond, DispatchResult* dispatchResult, TIA* tia)
{
  while (myPendingSignal != Signal::none) {}

  switch (myState) {
    case State::exception:
      rethrowPendingException();
      break;

    case State::stopped:
      break;

    case State::running:
    case State::paused:
      stop();
      break;

    default:
      throw runtime_error("unreachable");
      break;
  }

  myTia = tia;
  myDispatchResult = dispatchResult;
  myCyclesPerSecond = cyclesPerSecond;

  myPendingSignal = Signal::start;
  myWakeupCondition.notify_one();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RTEmulationWorker::stop()
{
  while (myPendingSignal != Signal::none) {}

  switch (myState) {
    case State::exception:
      break;

    case State::stopped:
      break;

    case State::running:
    case State::paused:
      myPendingSignal = Signal::stop;
      break;

    default:
      throw runtime_error("unreachable");
      break;
  }

  while (myState != State::stopped) rethrowPendingException();
}

void RTEmulationWorker::threadMain()
{
  std::unique_lock<std::mutex> lock(myThreadIsRunningMutex);

  try {
    while (myPendingSignal != Signal::quit) {
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
          throw runtime_error("invalid state");

        case Signal::start:
          break;
      }

      dispatchEmulation();
    }

    myState = State::quit;
  }
  catch (...) {
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

    if (deltaNanoseconds > MAX_LAG_NANOSECONDS) {
      virtualTimeNanoseconds = realTimeNanoseconds;
      deltaNanoseconds = 0.;
    }

    const Int64 cycleGoal = (deltaNanoseconds * myCyclesPerSecond) / 1000000000;
    uInt64 cyclesTotal = 0;

    while (cycleGoal > cyclesTotal && myDispatchResult->isSuccess()) {
      myTia->update(*myDispatchResult, cycleGoal - cyclesTotal);
      cyclesTotal += myDispatchResult->getCycles();
    }

    virtualTimeNanoseconds += cyclesTotal / myCyclesPerSecond * 1000000000;

    struct timespec now;
    do {
      clock_gettime(CLOCK_MONOTONIC, &now);

      switch (myPendingSignal) {
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
      }
    } while (timeDifferenceNanoseconds(now, timesliceStart) < TIMESLICE_NANOSECONDS || myState != State::running);
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
