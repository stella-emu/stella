//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <exception>

#include "EmulationWorker.hxx"
#include "DispatchResult.hxx"
#include "TIA.hxx"

using namespace std::chrono;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EmulationWorker::EmulationWorker(TIA& tia) : myTia(tia)
{
  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  std::condition_variable threadInitialized;

  myThread = std::thread(
    &EmulationWorker::threadMain, this, &threadInitialized, &mutex
  );

  // Wait until the thread has acquired myMutex and moved on
  while (myState == State::initializing) threadInitialized.wait(lock);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EmulationWorker::~EmulationWorker()
{
  // This has to run in a block in order to release the mutex before joining
  {
    std::unique_lock<std::mutex> lock(myMutex);

    if (myState != State::exception) {
      myPendingSignal = Signal::quit;
      mySignalCondition.notify_one();
    }
  }

  myThread.join();

  handlePossibleException();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EmulationWorker::handlePossibleException()
{
  if (myState == State::exception && myPendingException) {
    std::exception_ptr ex = myPendingException;
    // Make sure that the exception is not thrown a second time (destructor!!!)
    myPendingException = nullptr;

    std::rethrow_exception(ex);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EmulationWorker::start(uInt32 cyclesPerSecond, uInt32 maxCycles, uInt32 minCycles, DispatchResult* dispatchResult)
{
  // Optimization: run this in a block to unlock the mutex before notifying the thread;
  // otherwise, the thread would immediatelly block again until we have released the mutex,
  // sacrificing a timeslice
  {
    // Aquire the mutex -> wait until the thread is suspended
    std::unique_lock<std::mutex> lock(myMutex);

    // Pass on possible exceptions
    handlePossibleException();

    // NB: The thread does not suspend execution in State::initialized
    if (myState != State::waitingForResume)
      throw runtime_error("start called on running or dead worker");

    // Store the parameters for emulation
    myCyclesPerSecond = cyclesPerSecond;
    myMaxCycles = maxCycles;
    myMinCycles = minCycles;
    myDispatchResult = dispatchResult;

    // Set the signal...
    myPendingSignal = Signal::resume;
  }

  // ... and wakeup the thread
  mySignalCondition.notify_one();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EmulationWorker::stop()
{
  // See EmulationWorker::start for the gory details
  {
    std::unique_lock<std::mutex> lock(myMutex);
    handlePossibleException();

    // If the worker has stopped on its own, we return
    if (myState == State::waitingForResume) return;

    // NB: The thread does not suspend execution in State::initialized or State::running
    if (myState != State::waitingForStop)
      throw runtime_error("stop called on a dead worker");

    myPendingSignal = Signal::stop;
  }

  mySignalCondition.notify_one();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EmulationWorker::threadMain(std::condition_variable* initializedCondition, std::mutex* initializationMutex)
{
  std::unique_lock<std::mutex> lock(myMutex);

  try {
    // Optimization: release the lock before notifying our parent -> saves a timeslice
    {
      // Wait until our parent releases the lock and sleeps
      std::lock_guard<std::mutex> guard(*initializationMutex);

      // Update the state...
      myState = State::initialized;
    }

    // ... and wake up our parent to notifiy that we have initialized. From this point, the
    // parent can safely assume that we are running while the mutex is locked.
    initializedCondition->notify_one();

    while (myPendingSignal != Signal::quit) handleWakeup(lock);
  }
  catch (...) {
    myPendingException = std::current_exception();
    myState = State::exception;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EmulationWorker::handleWakeup(std::unique_lock<std::mutex>& lock)
{
  switch (myState) {
    case State::initialized:
      myState = State::waitingForResume;
      mySignalCondition.wait(lock);
      break;

    case State::waitingForResume:
      handleWakeupFromWaitingForResume(lock);
      break;

    case State::waitingForStop:
      handleWakeupFromWaitingForStop(lock);
      break;

    default:
      throw runtime_error("wakeup in invalid worker state");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EmulationWorker::handleWakeupFromWaitingForResume(std::unique_lock<std::mutex>& lock)
{
  switch (myPendingSignal) {
    case Signal::resume:
      myPendingSignal = Signal::none;
      myVirtualTime = high_resolution_clock::now();
      dispatchEmulation(lock);
      break;

    case Signal::none:
      mySignalCondition.wait(lock);
      break;

    case Signal::quit:
      break;

    default:
      throw runtime_error("invalid signal while waiting for resume");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EmulationWorker::handleWakeupFromWaitingForStop(std::unique_lock<std::mutex>& lock)
{
  switch (myPendingSignal) {
    case Signal::stop:
      myState = State::waitingForResume;
      myPendingSignal = Signal::none;

      mySignalCondition.wait(lock);
      break;

    case Signal::none:
      if (myVirtualTime <= high_resolution_clock::now())
        dispatchEmulation(lock);
      else
        mySignalCondition.wait_until(lock, myVirtualTime);

      break;

    case Signal::quit:
      break;

    default:
      throw runtime_error("invalid signal while waiting for stop");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EmulationWorker::dispatchEmulation(std::unique_lock<std::mutex>& lock)
{
  myState = State::running;

  uInt64 totalCycles = 0;

  do {
    myTia.update(*myDispatchResult, totalCycles > 0 ? myMinCycles - totalCycles : myMaxCycles);
  } while (totalCycles < myMinCycles && myDispatchResult->getStatus() == DispatchResult::Status::ok);

  if (myDispatchResult->getStatus() == DispatchResult::Status::ok) {
    // If emulation finished successfully, we can go for another round
    duration<double> timesliceSeconds(static_cast<double>(totalCycles) / static_cast<double>(myCyclesPerSecond));
    myVirtualTime += duration_cast<high_resolution_clock::duration>(timesliceSeconds);

    myState = State::waitingForStop;

    if (myVirtualTime > high_resolution_clock::now())
      // If we can keep up with the emulation, we sleep
      mySignalCondition.wait_until(lock, myVirtualTime);
    else {
      // If we are already lagging behind, we briefly relinquish control over the mutex
      // and yield to scheduler, to make sure that the main thread has a chance to stop us
      lock.release();
      std::this_thread::yield();
      lock.lock();
    }
  } else {
    // If execution trapped, we stop immediatelly.
    myState = State::waitingForResume;
    mySignalCondition.wait(lock);
  }
}
