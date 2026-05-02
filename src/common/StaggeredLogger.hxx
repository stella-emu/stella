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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef STAGGERED_LOGGER_HXX
#define STAGGERED_LOGGER_HXX

#include <chrono>
#include <mutex>

#include "bspf.hxx"
#include "Logger.hxx"
#include "TimerManager.hxx"

/**
 * This class buffers log events and logs them after a certain time window has
 * expired.  The timout increases after every log line by a factor of two until
 * a maximum is reached.  If no events are reported, the window size decreases
 * again.
 */
class StaggeredLogger
{
  public:
    StaggeredLogger(string_view message, Logger::Level level);
    ~StaggeredLogger();

    void log();

  private:
    // Clock aliases: steady_clock is guaranteed monotonic, unlike
    // high_resolution_clock which may alias system_clock on some platforms.
    using Clock     = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Ms        = std::chrono::milliseconds;

    void _log();
    void onTimerExpired(uInt32 timerCallbackId);
    void startInterval();
    void increaseInterval();
    void decreaseInterval();
    void logLine();

    string         myMessage;
    Logger::Level  myLevel;

    uInt32  myCurrentEventCount{0};
    bool    myIsCurrentlyCollecting{false};

    TimePoint  myLastIntervalStartTimestamp;
    TimePoint  myLastIntervalEndTimestamp;

    uInt32  myCurrentIntervalSize{100};   // milliseconds
    uInt32  myMaxIntervalFactor{9};
    uInt32  myCurrentIntervalFactor{1};
    uInt32  myCooldownTime{1000};         // milliseconds

    std::mutex myMutex;

    // Heap-allocated so we control the exact point the worker thread joins
    // (TimerManager join happens in its destructor, called explicitly below).
    unique_ptr<TimerManager> myTimer{std::make_unique<TimerManager>()};
    TimerManager::TimerId    myTimerId{0};

    // Incremented each time a new timer is armed.  The callback captures this
    // value; a mismatch means the timer is stale and the callback is a no-op.
    uInt32  myTimerCallbackId{0};

  private:
    StaggeredLogger(const StaggeredLogger&) = delete;
    StaggeredLogger(StaggeredLogger&&) = delete;
    StaggeredLogger& operator=(const StaggeredLogger&) = delete;
    StaggeredLogger& operator=(StaggeredLogger&&) = delete;
};

#endif  // STAGGERED_LOGGER_HXX
