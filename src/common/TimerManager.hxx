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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef TIMER_MANAGER_HXX
#define TIMER_MANAGER_HXX

#include <algorithm>
#include <functional>
#include <chrono>
#include <unordered_map>
#include <set>
#include <cstdint>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "bspf.hxx"

/**
  This class provides a portable periodic/one-shot timer infrastructure
  using worker threads and generic C++11 code.

  @author  Doug Gale (doug65536)
           From "Code Review"
           https://codereview.stackexchange.com/questions/127552/portable-periodic-one-shot-timer-thread-follow-up

           Modifications and cleanup for Stella by Stephen Anthony
*/
class TimerManager
{
  public:
    // Each Timer is assigned a unique ID of type TimerId
    using TimerId = uInt64;

    // Function object we actually use
    using TFunction = std::function<void()>;

    // Values that are a large-range millisecond count
    using millisec = uInt64;

    // Constructor does not start worker until there is a Timer.
    explicit TimerManager();

    // Destructor is thread safe, even if a timer callback is running.
    // All callbacks are guaranteed to have returned before this
    // destructor returns.
    ~TimerManager();

    /**
      Create a new timer using milliseconds, and add it to the internal queue.

      @param msDelay  Callback starts firing this many milliseconds from now
      @param msPeriod If non-zero, callback is fired again after this period
      @param func     The callback to run at the specified interval

      @return  Id used to identify the timer for later use
    */
    TimerId addTimer(millisec msDelay, millisec msPeriod, const TFunction& func);

    /**
      Convenience function; setInterval API like browser javascript.

      Call function every 'period' ms, starting 'period' ms from now.
    */
    TimerId setInterval(const TFunction& func, millisec period) {
      return addTimer(period, period, std::move(func));
    }

    /**
      Convenience function; setTimeout API like browser javascript.

      Call function once 'timeout' ms from now.
    */
    TimerId setTimeout(const TFunction& func, millisec timeout) {
      return addTimer(timeout, 0, std::move(func));
    }

    /**
      Destroy the specified timer.

      Synchronizes with the worker thread if the callback for this timer
      is running, which guarantees that the handler for that callback is
      not running before clear() returns.

      You are not required to clear any timers. You can forget their
      TimerId if you do not need to cancel them.

      The only time you need this is when you want to stop a timer that
      has a repetition period, or you want to cancel a timeout that has
      not fired yet.
    */
    bool clear(TimerId id);

    /**
      Destroy all timers, but preserve id uniqueness.
      This carefully makes sure every timer is not executing its callback
      before destructing it.
    */
    void clear();

    // Peek at current state
    std::size_t size() const noexcept;
    bool empty() const noexcept;

    // Returns lazily initialized singleton
    static TimerManager& global();

  private:
    using Lock = std::mutex;
    using ScopedLock = std::unique_lock<Lock>;
    using ConditionVar = std::condition_variable;

    using Clock = std::chrono::steady_clock;
    using Timestamp = std::chrono::time_point<Clock>;
    using Duration = std::chrono::milliseconds;

    struct Timer
    {
      explicit Timer(TimerId id = 0);
      Timer(Timer&& r) noexcept;
      Timer& operator=(Timer&& r) noexcept;

      Timer(TimerId id, Timestamp next, Duration period, const TFunction& func) noexcept;

      // Never called
      Timer(Timer const& r) = delete;
      Timer& operator=(Timer const& r) = delete;

      TimerId id;
      Timestamp next;
      Duration period;
      TFunction handler;

      // You must be holding the 'sync' lock to assign waitCond
      std::unique_ptr<ConditionVar> waitCond;

      bool running;
    };

    // Comparison functor to sort the timer "queue" by Timer::next
    struct NextActiveComparator
    {
      bool operator()(Timer const& a, Timer const& b) const noexcept
      {
        return a.next < b.next;
      }
    };

    // Queue is a set of references to Timer objects, sorted by next
    using QueueValue = std::reference_wrapper<Timer>;
    using Queue = std::multiset<QueueValue, NextActiveComparator>;
    using TimerMap = std::unordered_map<TimerId, Timer>;

    void timerThreadWorker();
    bool destroy_impl(ScopedLock& lock, TimerMap::iterator i, bool notify);

    // Inexhaustible source of unique IDs
    TimerId nextId;

    // The Timer objects are physically stored in this map
    TimerMap active;

    // The ordering queue holds references to items in 'active'
    Queue queue;

    // One worker thread for an unlimited number of timers is acceptable
    // Lazily started when first timer is started
    // TODO: Implement auto-stopping the timer thread when it is idle for
    // a configurable period.
    mutable Lock sync;
    ConditionVar wakeUp;
    std::thread worker;
    bool done;

    // Valid IDs are guaranteed not to be this value
    static TimerId constexpr no_timer = TimerId(0);
};

#endif // TIMERTHREAD_H
