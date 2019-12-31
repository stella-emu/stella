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

#ifndef SQLITE_DATABASE_HXX
#define SQLITE_DATABASE_HXX

#include <sqlite3.h>

#include "bspf.hxx"

class SqliteDatabase
{
  public:

    SqliteDatabase(const string& databaseDirectory, const string& databaseName);

    ~SqliteDatabase();

    void initialize();

    const string fileName() const { return myDatabaseFile; }

    operator sqlite3*() const { return myHandle; }

    void exec(const string &sql) const;

  private:

    string myDatabaseFile;

    sqlite3* myHandle{nullptr};

  private:

    SqliteDatabase(const SqliteDatabase&) = delete;
    SqliteDatabase(SqliteDatabase&&) = delete;
    SqliteDatabase& operator=(const SqliteDatabase&) = delete;
    SqliteDatabase& operator=(SqliteDatabase&&) = delete;
};

#endif // SQLITE_DATABASE_HXX
