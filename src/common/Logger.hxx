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

#ifndef LOGGER_HXX
#define LOGGER_HXX

#include <functional>

#include "bspf.hxx"

class Logger {
  public:

    using logCallback = std::function<void(const string&, uInt8)>;

  public:

    static Logger& instance();

    static void log(const string& message, uInt8 level);

    void logMessage(const string& message, uInt8 level) const;

    void setLogCallback(logCallback callback);

    void clearLogCallback();

  protected:

    Logger() = default;

  private:

    logCallback myLogCallback;

  private:

    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(const Logger&&) = delete;
};

#endif // LOGGER_HXX
