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

#ifndef SQLITE_STATEMENT_HXX
#define SQLITE_STATEMENT_HXX

#include <sqlite3.h>

#include "bspf.hxx"

class SqliteStatement {
  public:

    SqliteStatement(sqlite3* handle, string sql);

    ~SqliteStatement();

    operator sqlite3_stmt*() const { return myStmt; }

    SqliteStatement& bind(int index, const string& value);

    bool step() const;

    void reset() const;

    string columnText(int index) const;

  private:

    sqlite3_stmt* myStmt{nullptr};

    sqlite3* myHandle{nullptr};

  private:

    SqliteStatement() = delete;
    SqliteStatement(const SqliteStatement&) = delete;
    SqliteStatement(SqliteStatement&&) = delete;
    SqliteStatement& operator=(const SqliteStatement&) = delete;
    SqliteStatement& operator=(SqliteStatement&&) = delete;
};

#endif // SQLITE_STATEMENT_HXX
