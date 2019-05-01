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

#ifndef KEY_VALUE_REPOSITORY_SQLITE_HXX
#define KEY_VALUE_REPOSITORY_SQLITE_HXX

#include "bspf.hxx"
#include "repository/KeyValueRepository.hxx"
#include "SqliteDatabase.hxx"
#include "SqliteStatement.hxx"

class KeyValueRepositorySqlite : public KeyValueRepository
{
  public:

    KeyValueRepositorySqlite(SqliteDatabase& db, const string& tableName);

    virtual std::map<string, Variant> load();

    virtual void save(const std::map<string, Variant>& values);

    void initialize();

  private:

    string myTableName;
    SqliteDatabase& myDb;

    unique_ptr<SqliteStatement> myStmtInsert;
    unique_ptr<SqliteStatement> myStmtSelect;

  private:

    KeyValueRepositorySqlite(const KeyValueRepositorySqlite&) = delete;
    KeyValueRepositorySqlite(KeyValueRepositorySqlite&&) = delete;
    KeyValueRepositorySqlite& operator=(const KeyValueRepositorySqlite&) = delete;
    KeyValueRepositorySqlite operator=(KeyValueRepositorySqlite&&) = delete;
};

#endif // KEY_VALUE_REPOSITORY_SQLITE_HXX
