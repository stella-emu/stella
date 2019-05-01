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

#include "KeyValueRepositorySqlite.hxx"
#include "SqliteError.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
KeyValueRepositorySqlite::KeyValueRepositorySqlite(
  SqliteDatabase& db,
  const string& tableName
) : myTableName(tableName),
    myDb(db)
{}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::map<string, Variant> KeyValueRepositorySqlite::load()
{
  std::map<string, Variant> values;

  try {
    myStmtSelect->reset();

    while (myStmtSelect->step())
      values[myStmtSelect->columnText(0)] = myStmtSelect->columnText(1);

    myStmtSelect->reset();
  }
  catch (SqliteError err) {
    cout << err.message << std::endl;
  }

  return values;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyValueRepositorySqlite::save(const std::map<string, Variant>& values)
{
  try {
    myStmtInsert->reset();

    myDb.exec("BEGIN TRANSACTION");

    for (const auto& pair: values) {
      (*myStmtInsert)
        .bind(1, pair.first.c_str())
        .bind(2, pair.second.toCString())
        .step();

      myStmtInsert->reset();
    }

    myDb.exec("COMMIT");
  }
  catch (SqliteError err) {
    cout << err.message << endl;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyValueRepositorySqlite::save(const string& key, const Variant& value)
{
  try {
    myStmtInsert->reset();

    (*myStmtInsert)
      .bind(1, key.c_str())
      .bind(2, value.toCString())
      .step();

    myStmtInsert->reset();
  }
  catch (SqliteError err) {
    cout << err.message << endl;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyValueRepositorySqlite::initialize()
{
  myDb.exec(
    "CREATE TABLE IF NOT EXISTS `" + myTableName + "` (`key` TEXT PRIMARY KEY, `value` TEXT) WITHOUT ROWID"
  );

  myStmtInsert = make_unique<SqliteStatement>(myDb, "INSERT OR REPLACE INTO `" + myTableName + "` VALUES (?, ?)");
  myStmtSelect = make_unique<SqliteStatement>(myDb, "SELECT `key`, `VALUE` FROM `" + myTableName + "`");
}
