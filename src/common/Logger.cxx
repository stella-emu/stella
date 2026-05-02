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

#include "Logger.hxx"

#ifdef __LIB_RETRO__
extern void libretro_logger(int log_level, const char* string);
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Logger& Logger::instance()
{
  static Logger loggerInstance;
  return loggerInstance;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Logger::log(string_view message, Level level)
{
  instance().logMessage(message, level);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Logger::error(string_view message)
{
  instance().logMessage(message, Level::ERR);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Logger::info(string_view message)
{
  instance().logMessage(message, Level::INFO);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Logger::debug(string_view message)
{
  instance().logMessage(message, Level::DEBUG);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Logger::logMessage(string_view message, Level level)
{
  const std::scoped_lock lock(myMutex);

  // ALWAYS and ERR bypass the level filter; others must be within myLogLevel
  const bool shouldLog = level == Level::ERR
                      || level == Level::ALWAYS
                      || static_cast<int>(level) <= myLogLevel;
  if (!shouldLog) return;

  // ERR always goes to console regardless of myLogToConsole
  if (myLogToConsole || level == Level::ERR)
    cout << message << '\n' << std::flush;

  // Single append call per chunk avoids repeated reallocation
  myLogMessages.append(message).append(1, '\n');

#ifdef __LIB_RETRO__
  // libretro needs a null-terminated string; construct only when needed
  libretro_logger(static_cast<int>(level), string{message}.c_str());
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Logger::setLogParameters(int logLevel, bool logToConsole)
{
  if (logLevel >= static_cast<int>(Level::MIN) &&
      logLevel <= static_cast<int>(Level::MAX))
  {
    myLogLevel     = logLevel;
    myLogToConsole = logToConsole;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Logger::setLogParameters(Level logLevel, bool logToConsole)
{
  setLogParameters(static_cast<int>(logLevel), logToConsole);
}
