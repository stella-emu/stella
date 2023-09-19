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
// Copyright (c) 1995-2023 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef RT_EMULATION_WORKER_HXX
#define RT_EMULATION_WORKER_HXX

#include <thread>
#include <atomic>
#include <mutex>
#include <exception>
#include <condition_variable>

#include "bspf.hxx"

class TIA;
class DispatchResult;

class RTEmulationWorker {
  public:

    enum class Signal: uInt32 {
      start,
      suspend,
      resume,
      stop,
      quit,
      none
    };

    enum class State: uInt32 {
      initializing,
      stopped,
      running,
      paused,
      quit,
      exception
    };

  public:

    RTEmulationWorker();

    ~RTEmulationWorker();

    void start(uInt32 cyclesPerSecond, DispatchResult* dispatchResult, TIA* tia);

    void suspend();

    void resume();

    void stop();

  private:

    void threadMain();

    void rethrowPendingException();

    void dispatchEmulation();

  private:

    std::thread myThread;

    std::condition_variable myWakeupCondition;
    std::mutex myThreadIsRunningMutex;

    std::atomic<Signal> myPendingSignal{Signal::none};
    std::atomic<State> myState{State::initializing};

    std::exception_ptr myPendingException;

    TIA* myTia{nullptr};
    uInt64 myCyclesPerSecond{0};
    DispatchResult* myDispatchResult{nullptr};
};

#endif // RT_EMULATION_WORKER_HXX
