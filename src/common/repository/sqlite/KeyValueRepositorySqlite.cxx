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

#include <sqlite3.h>
#include <cstdio>

#include "KeyValueRepositorySqlite.hxx"
#include "bspf.hxx"

#ifdef BSPF_WINDOWS
  #define SEPARATOR "\"
#else
  #define SEPARATOR "/"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
KeyValueRepositorySqlite::KeyValueRepositorySqlite(
  const string& databaseDirectory,
  const string& databaseName
) : myDatabaseFile(databaseDirectory + SEPARATOR + databaseName + ".sqlite3"),
    myDbHandle(nullptr),
    myDbInitialized(false)
{}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
KeyValueRepositorySqlite::~KeyValueRepositorySqlite()
{
  if (myDbHandle) sqlite3_close(myDbHandle);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::map<string, Variant> KeyValueRepositorySqlite::load()
{
  std::map<string, Variant> values;

  initializeDb();

  if (!myDbInitialized) {
    cout << "Unable to load from sqlite DB " << myDatabaseFile << endl;
    return values;
  }

  sqlite3_stmt* stmt;
  if (sqlite3_prepare_v2(
    myDbHandle,
    "SELECT `key`, `VALUE` FROM `values`",
    -1, &stmt, nullptr
  ) != SQLITE_OK) return values;

  while (sqlite3_step(stmt) == SQLITE_ROW)
    values[reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))] =
      reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

  sqlite3_finalize(stmt);

  return values;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyValueRepositorySqlite::save(const std::map<string, Variant>& values)
{
  initializeDb();

  if (!myDbInitialized) {
    cout << "Unable to save to sqlite DB " << myDatabaseFile << endl;
    return;
  }

  sqlite3_stmt* stmt;
  if (sqlite3_prepare_v2(
    myDbHandle,
    "INSERT OR REPLACE INTO `values` VALUES (?, ?)",
    -1, &stmt, nullptr
  ) != SQLITE_OK) return;

  for (const auto& pair: values) {
    sqlite3_bind_text(stmt, 1, pair.first.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, pair.second.toCString(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);

    if (sqlite3_reset(stmt) != SQLITE_OK) break;
  }

  sqlite3_finalize(stmt);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyValueRepositorySqlite::initializeDb()
{
  if (myDbHandle) return;

  for (int tries = 1; tries < 3 && !myDbInitialized; tries++) {
    myDbInitialized = (sqlite3_open(myDatabaseFile.c_str(), &myDbHandle) == SQLITE_OK);

    myDbInitialized = myDbInitialized && (sqlite3_exec(
      myDbHandle,
      "CREATE TABLE IF NOT EXISTS `values` (`key` TEXT PRIMARY KEY, `value` TEXT) WITHOUT ROWID",
      nullptr, nullptr, nullptr
    ) == SQLITE_OK);

    if (!myDbInitialized && tries == 1) {
      cout << "sqlite DB " << myDatabaseFile << " seems to be corrupt, removing and retrying..." << endl;

      remove(myDatabaseFile.c_str());
    }
  }

  if (!myDbInitialized)
    cout << "unable to initialize sqlite DB " << myDatabaseFile << endl;
}
