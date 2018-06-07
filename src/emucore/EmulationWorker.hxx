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

#ifndef EMULATION_WORKER_HXX
#define EMULATION_WORKER_HXX

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <exception>
#include <chrono>

#include "bspf.hxx"

class TIA;
class DispatchResult;

class EmulationWorker
{
  public:
    enum class State {
      initializing, initialized, waitingForResume, running, waitingForStop, exception
    };

    enum class Signal {
      resume, stop, quit, none
    };

  public:

    EmulationWorker(TIA& tia);

    ~EmulationWorker();

    void start(uInt32 cyclesPerSecond, uInt32 maxCycles, uInt32 minCycles, DispatchResult* dispatchResult);

    void stop();

  private:

    void handlePossibleException();

    // Passing references into a thread is awkward and requires std::ref -> use pointers here
    void threadMain(std::condition_variable* initializedCondition, std::mutex* initializationMutex);

    void handleWakeup(std::unique_lock<std::mutex>& lock);

    void handleWakeupFromWaitingForResume(std::unique_lock<std::mutex>& lock);

    void handleWakeupFromWaitingForStop(std::unique_lock<std::mutex>& lock);

    void dispatchEmulation(std::unique_lock<std::mutex>& lock);

  private:

    TIA& myTia;

    std::thread myThread;

    std::condition_variable mySignalCondition;
    std::mutex myMutex;
    std::exception_ptr myPendingException;
    Signal myPendingSignal;
    // The initial access to myState is not synchronized -> make this atomic
    std::atomic<State> myState;

    uInt32 myCyclesPerSecond;
    uInt32 myMaxCycles;
    uInt32 myMinCycles;
    DispatchResult* myDispatchResult;

    std::chrono::time_point<std::chrono::high_resolution_clock> myVirtualTime;
};

#endif // EMULATION_WORKER_HXX
