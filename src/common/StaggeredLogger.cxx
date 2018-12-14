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

#include "StaggeredLogger.hxx"

#include <ctime>

using namespace std::chrono;

namespace {
  string currentTimestamp()
  {
    std::tm now = BSPF::localTime();

    char formattedTime[100];
    formattedTime[99] = 0;
    std::strftime(formattedTime, 99, "%H:%M:%S", &now);

    return formattedTime;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StaggeredLogger::StaggeredLogger(const string& message, Logger logger)
  : myMessage(message),
    myCurrentEventCount(0),
    myIsCurrentlyCollecting(false),
    myCurrentIntervalSize(100),
    myMaxIntervalFactor(9),
    myCurrentIntervalFactor(1),
    myCooldownTime(1000)
{
  if (logger) myLogger = logger;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaggeredLogger::setLogger(Logger logger)
{
  myLogger = logger;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaggeredLogger::log()
{
  std::lock_guard<std::mutex> lock(myMutex);

  _log();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaggeredLogger::advance()
{
  std::lock_guard<std::mutex> lock(myMutex);

  _advance();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaggeredLogger::_log()
{
  _advance();

  if (!myIsCurrentlyCollecting) {
    myCurrentEventCount = 0;
    myIsCurrentlyCollecting = true;
    myCurrentIntervalStartTimestamp = high_resolution_clock::now();
  }

  myCurrentEventCount++;
  myLastLogEventTimestamp = high_resolution_clock::now();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaggeredLogger::logLine()
{
  stringstream ss;
  ss
    << currentTimestamp() << ": "
    << myMessage
    << " (" << myCurrentEventCount << " times in "
      << myCurrentIntervalSize << "  milliseconds"
    << ")";

    myLogger(ss.str());

    myIsCurrentlyCollecting = false;

    increaseInterval();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaggeredLogger::increaseInterval()
{
  if (myCurrentIntervalFactor >= myMaxIntervalFactor) return;

  myCurrentIntervalFactor++;
  myCurrentIntervalSize *= 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaggeredLogger::decreaseInterval()
{
  if (myCurrentIntervalFactor <= 1) return;

  myCurrentIntervalFactor--;
  myCurrentIntervalSize /= 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaggeredLogger::_advance()
{
  high_resolution_clock::time_point now = high_resolution_clock::now();

  if (myIsCurrentlyCollecting) {
    Int64 msecSinceIntervalStart =
      duration_cast<duration<Int64, std::milli>>(now - myCurrentIntervalStartTimestamp).count();

    if (msecSinceIntervalStart > myCurrentIntervalSize) logLine();
  }

  Int64 msec =
    duration_cast<duration<Int64, std::milli>>(now - myLastLogEventTimestamp).count();

  while (msec > myCooldownTime && myCurrentIntervalFactor > 1) {
    msec -= myCooldownTime;
    decreaseInterval();
  }
}
