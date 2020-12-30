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

#include "AbstractKeyValueRepositorySqlite.hxx"
#include "Logger.hxx"
#include "SqliteError.hxx"
#include "SqliteTransaction.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::map<string, Variant> AbstractKeyValueRepositorySqlite::load()
{
  std::map<string, Variant> values;

  try {
    SqliteStatement& stmt{stmtSelect()};

    while (stmt.step())
      values[stmt.columnText(0)] = stmt.columnText(1);

    stmt.reset();
  }
  catch (const SqliteError& err) {
    Logger::info(err.what());
  }

  return values;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AbstractKeyValueRepositorySqlite::save(const std::map<string, Variant>& values)
{
  try {
    SqliteTransaction tx{database()};

    for (const auto& pair: values) {
      SqliteStatement& stmt{stmtInsert(pair.first, pair.second.toString())};

      stmt.step();
      stmt.reset();
    }

    tx.commit();
  }
  catch (const SqliteError& err) {
    Logger::info(err.what());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AbstractKeyValueRepositorySqlite::save(const string& key, const Variant& value)
{
  try {
    SqliteStatement& stmt{stmtInsert(key, value.toString())};

    stmt.step();
    stmt.reset();
  }
  catch (const SqliteError& err) {
    Logger::info(err.what());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AbstractKeyValueRepositorySqlite::remove(const string& key)
{
  try {
    SqliteStatement& stmt{stmtDelete(key)};

    stmt.step();
    stmt.reset();
  }
  catch (const SqliteError& err) {
    Logger::info(err.what());
  }
}
