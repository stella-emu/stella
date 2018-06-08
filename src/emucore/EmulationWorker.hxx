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

/*
 * This class is the core of stella's real time scheduling. Scheduling is a two step
 * process that is shared between the main loop in OSystem and this class.
 *
 * In emulation mode (as opposed to debugger, menu, etc.), each iteration of the main loop
 * instructs the emulation worker to start emulation on a separate thread and then proceeds
 * to render the last frame produced by the TIA (if any). After the frame has been rendered,
 * the worker is stopped, and the main thread sleeps until the time allotted to the emulation
 * timeslice (as calculated from the 6507 cycles that have passed) has been reached. After
 * that, it iterates.
 *
 * The emulation worker contains its own microscheduling. After emulating a timeslice, it sleeps
 * until either the allotted time is up or it has been signalled to stop. If the time is up
 * without the signal, the worker will emulate another timeslice, etc.
 *
 * In combination, the scheduling in the main loop and the microscheduling in the worker
 * ensure that the emulation continues to run even if rendering blocks, ensuring the real
 * time scheduling required for cycle exact audio to work.
 */

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

    EmulationWorker();

    ~EmulationWorker();

    /**
      Wake up the worker and start emulation with the specified parameters.
     */
    void start(uInt32 cyclesPerSecond, uInt32 maxCycles, uInt32 minCycles, DispatchResult* dispatchResult, TIA* tia);

    /**
      Stop emulation and return the number of 6507 cycles emulated.
     */
    uInt64 stop();

  private:

    void handlePossibleException();

    // Passing references into a thread is awkward and requires std::ref -> use pointers here
    void threadMain(std::condition_variable* initializedCondition, std::mutex* initializationMutex);

    void handleWakeup(std::unique_lock<std::mutex>& lock);

    void handleWakeupFromWaitingForResume(std::unique_lock<std::mutex>& lock);

    void handleWakeupFromWaitingForStop(std::unique_lock<std::mutex>& lock);

    void dispatchEmulation(std::unique_lock<std::mutex>& lock);

    void clearSignal();

    void signalQuit();

    void waitUntilPendingSignalHasProcessed();

    void fatal(string message);

  private:

    TIA* myTia;

    std::thread myThread;

    // Condition variable for waking up the thread
    std::condition_variable myWakeupCondition;
    // THe thread is running while this mutex is locked
    std::mutex myThreadIsRunningMutex;

    std::condition_variable mySignalChangeCondition;
    std::mutex mySignalChangeMutex;

    std::exception_ptr myPendingException;
    Signal myPendingSignal;
    // The initial access to myState is not synchronized -> make this atomic
    std::atomic<State> myState;

    uInt32 myCyclesPerSecond;
    uInt32 myMaxCycles;
    uInt32 myMinCycles;
    DispatchResult* myDispatchResult;

    uInt64 myTotalCycles;
    std::chrono::time_point<std::chrono::high_resolution_clock> myVirtualTime;

  private:

    EmulationWorker(const EmulationWorker&) = delete;

    EmulationWorker(EmulationWorker&&) = delete;

    EmulationWorker& operator=(const EmulationWorker&) = delete;

    EmulationWorker& operator=(EmulationWorker&&) = delete;
};

#endif // EMULATION_WORKER_HXX
