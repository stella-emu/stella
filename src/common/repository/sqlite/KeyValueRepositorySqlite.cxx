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

namespace {
  struct SqliteError {
    SqliteError(const string _message) : message(_message) {}

    const string message;
  };

  class Statement {
    public:

      Statement(sqlite3* handle, string sql) : myStmt(nullptr)
      {
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &myStmt, nullptr) != SQLITE_OK)
          throw SqliteError(sqlite3_errmsg(handle));
      }

      ~Statement()
      {
        if (myStmt) sqlite3_finalize(myStmt);
      }

      operator sqlite3_stmt*() const { return myStmt; }

    private:

      sqlite3_stmt* myStmt;

    private:

      Statement() = delete;
      Statement(const Statement&) = delete;
      Statement(Statement&&) = delete;
      Statement& operator=(const Statement&) = delete;
      Statement& operator=(Statement&&) = delete;
  };
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
KeyValueRepositorySqlite::KeyValueRepositorySqlite(
  const string& databaseDirectory,
  const string& databaseName
) : myDatabaseFile(databaseDirectory + SEPARATOR + databaseName + ".sqlite3"),
    myIsFailed(false),
    myDbHandle(nullptr)
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
  if (myIsFailed) return values;

  try {
    initializeDb();
    Statement stmt(myDbHandle, "SELECT `key`, `VALUE` FROM `values`");

    while (sqlite3_step(stmt) == SQLITE_ROW)
      values[reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))] =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
  }
  catch (SqliteError err) {
    cout << "failed to load from sqlite DB " << myDatabaseFile << ": " << err.message << endl;
  }

  return values;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyValueRepositorySqlite::save(const std::map<string, Variant>& values)
{
  if (myIsFailed) return;

  try {
    initializeDb();
    Statement stmt(myDbHandle, "INSERT OR REPLACE INTO `values` VALUES (?, ?)");

    if (sqlite3_exec(myDbHandle, "BEGIN TRANSACTION", nullptr, nullptr, nullptr) != SQLITE_OK)
      throw SqliteError(sqlite3_errmsg(myDbHandle));

    for (const auto& pair: values) {
      sqlite3_bind_text(stmt, 1, pair.first.c_str(), -1, SQLITE_STATIC);
      sqlite3_bind_text(stmt, 2, pair.second.toCString(), -1, SQLITE_STATIC);
      sqlite3_step(stmt);

      if (sqlite3_reset(stmt) != SQLITE_OK) throw SqliteError(sqlite3_errmsg(myDbHandle));
    }

    if (sqlite3_exec(myDbHandle, "COMMIT", nullptr, nullptr, nullptr) != SQLITE_OK)
      throw SqliteError(sqlite3_errmsg(myDbHandle));
  }
  catch (SqliteError err) {
    cout << "failed to write to sqlite DB " << myDatabaseFile << ": " << err.message << endl;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyValueRepositorySqlite::initializeDb()
{
  if (myIsFailed || myDbHandle) return;

  bool dbInitialized = false;

  for (int tries = 1; tries < 3 && !dbInitialized; tries++) {
    dbInitialized = (sqlite3_open(myDatabaseFile.c_str(), &myDbHandle) == SQLITE_OK);

    dbInitialized = dbInitialized && (sqlite3_exec(
      myDbHandle,
      "CREATE TABLE IF NOT EXISTS `values` (`key` TEXT PRIMARY KEY, `value` TEXT) WITHOUT ROWID",
      nullptr, nullptr, nullptr
    ) == SQLITE_OK);

    if (!dbInitialized && tries == 1) {
      cout << "sqlite DB " << myDatabaseFile << " seems to be corrupt, removing and retrying..." << endl;

      remove(myDatabaseFile.c_str());
    }
  }

  myIsFailed = !dbInitialized;

  if (myIsFailed) {
    if (myDbHandle) {
      string emsg = sqlite3_errmsg(myDbHandle);

      sqlite3_close(myDbHandle);
      myDbHandle = nullptr;

      throw SqliteError(emsg);
    }

    throw SqliteError("unable to initialize sqlite DB  " + myDatabaseFile);
  };
}
