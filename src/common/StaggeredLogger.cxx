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

#include "StaggeredLogger.hxx"
#include "Logger.hxx"

#include <chrono>
#include <format>
#include <limits>

using namespace std::chrono;

namespace {
  // Returns the current local time formatted as HH:MM:SS.
  // Uses strftime for broad toolchain compatibility; switch to
  // std::chrono::zoned_time + std::format once <chrono> TZ support is
  // universally available.
  string currentTimestamp()
  {
    const std::tm now = BSPF::localTime();
    return std::format("{:02}:{:02}:{:02}", now.tm_hour, now.tm_min, now.tm_sec);
  }
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StaggeredLogger::StaggeredLogger(string_view message, Logger::Level level)
  : myMessage{message},
    myLevel{level}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StaggeredLogger::~StaggeredLogger()
{
  myTimer->clear(myTimerId);

  // Explicitly reset (joins the worker thread) before any members are
  // destroyed, guaranteeing no reentrant callbacks during destruction.
  myTimer.reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaggeredLogger::log()
{
  const std::scoped_lock lock(myMutex);
  _log();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaggeredLogger::_log()
{
  if (!myIsCurrentlyCollecting) startInterval();
  ++myCurrentEventCount;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaggeredLogger::logLine()
{
  const auto elapsed = duration_cast<Ms>(
    Clock::now() - myLastIntervalStartTimestamp).count();

  Logger::log(
    std::format("{}: {} ({} times in {} milliseconds)",
      currentTimestamp(), myMessage, myCurrentEventCount, elapsed),
    myLevel);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaggeredLogger::increaseInterval()
{
  if (myCurrentIntervalFactor >= myMaxIntervalFactor) return;

  ++myCurrentIntervalFactor;
  // Guard against uInt32 overflow on repeated doubling
  myCurrentIntervalSize = std::min(
    myCurrentIntervalSize * 2U,
    std::numeric_limits<uInt32>::max() / 2U);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaggeredLogger::decreaseInterval()
{
  if (myCurrentIntervalFactor <= 1) return;

  --myCurrentIntervalFactor;
  myCurrentIntervalSize /= 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaggeredLogger::startInterval()
{
  if (myIsCurrentlyCollecting) return;

  myIsCurrentlyCollecting = true;

  const TimePoint now = Clock::now();

  // Decay the interval factor proportionally to how long we have been quiet.
  // Each full cooldown period that has elapsed reduces the factor by one step.
  // Note: std::cmp_greater is required here because myCooldownTime is uInt32
  // and msecSinceLastIntervalEnd is Int64; the mixed-sign comparison would
  // otherwise be implementation-defined.
  Int64 msecSinceLastIntervalEnd =
    duration_cast<Ms>(now - myLastIntervalEndTimestamp).count();

  while (std::cmp_greater(msecSinceLastIntervalEnd, myCooldownTime) &&
         myCurrentIntervalFactor > 1)
  {
    msecSinceLastIntervalEnd -= static_cast<Int64>(myCooldownTime);
    decreaseInterval();
  }

  myCurrentEventCount          = 0;
  myLastIntervalStartTimestamp = now;

  myTimer->clear(myTimerId);

  // Capture callbackId by value; avoids std::bind and makes the capture
  // intent explicit.  The id mismatch check in onTimerExpired handles the
  // case where a stale callback fires after a new interval has been armed.
  const uInt32 callbackId = ++myTimerCallbackId;
  myTimerId = myTimer->setTimeout(
    [this, callbackId]{ onTimerExpired(callbackId); },
    myCurrentIntervalSize);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaggeredLogger::onTimerExpired(uInt32 timerCallbackId)
{
  const std::scoped_lock lock(myMutex);

  // Stale callback: a new interval was armed before this one fired.
  if (timerCallbackId != myTimerCallbackId) return;

  logLine();

  myIsCurrentlyCollecting    = false;
  myLastIntervalEndTimestamp = Clock::now();

  increaseInterval();
}
