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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef SQLITE_ERROR_HXX
#define SQLITE_ERROR_HXX

#include <sqlite3.h>
#include "bspf.hxx"

struct SqliteError {
  explicit SqliteError(const string& _message) : message(_message) {}

  explicit SqliteError(sqlite3* handle) : message(sqlite3_errmsg(handle)) {}

  const string message;
};

#endif // SQLITE_ERROR_HXX
