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

#ifndef STAGGERED_LOGGER
#define STAGGERED_LOGGER

#include <functional>
#include <chrono>
#include <thread>
#include <mutex>

#include "bspf.hxx"

/**
 * This class buffers log events and logs them after a certain time window has expired.
 * The timout increases after every log line by a factor of two until a maximum is reached.
 * If no events are reported, the window size decreases again.
 */

class StaggeredLogger {
  public:

    typedef std::function<void(const string&)> Logger;

  public:

    StaggeredLogger(const string& message, Logger logger = Logger());

    void log();

    void advance();

    void setLogger(Logger logger);

  private:

    void _log();

    void _advance();

    void increaseInterval();

    void decreaseInterval();

    void logLine();

    Logger myLogger;
    string myMessage;

    uInt32 myCurrentEventCount;
    bool myIsCurrentlyCollecting;

    std::chrono::high_resolution_clock::time_point myCurrentIntervalStartTimestamp;
    std::chrono::high_resolution_clock::time_point myLastLogEventTimestamp;

    uInt32 myCurrentIntervalSize;
    uInt32 myMaxIntervalFactor;
    uInt32 myCurrentIntervalFactor;
    uInt32 myCooldownTime;

    std::mutex myMutex;
};

#endif // STAGGERED_LOGGER
