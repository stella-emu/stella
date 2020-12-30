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

#include "CompositeKeyValueRepositorySqlite.hxx"
#include "Logger.hxx"
#include "SqliteError.hxx"
#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeKeyValueRepositorySqlite::CompositeKeyValueRepositorySqlite(SqliteDatabase& db, const string& tableName)
  : myTableName{tableName}, myDb{db}
{}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
shared_ptr<KeyValueRepository> CompositeKeyValueRepositorySqlite::get(const string& key)
{
  return make_shared<ProxyRepository>(*this, key);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CompositeKeyValueRepositorySqlite::has(const string& key)
{
  try {
    myStmtCount->reset();
    myStmtCount->bind(1, key.c_str());

    if (!myStmtCount->step())
      throw new SqliteError("count failed");

    Int32 rowCount = myStmtCount->columnInt(0);

    myStmtCount->reset();

    return rowCount > 0;
  } catch (const SqliteError& err) {
    Logger::info(err.what());

    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeKeyValueRepositorySqlite::remove(const string& key)
{
  try {
    myStmtDeleteSet->reset();

    (*myStmtDeleteSet)
      .bind(1, key.c_str())
      .step();
  }
  catch (const SqliteError& err) {
    Logger::info(err.what());
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeKeyValueRepositorySqlite::initialize()
{
  myDb.exec(
    "CREATE TABLE IF NOT EXISTS `" + myTableName + "` (`key1` TEXT, `key2` TEXT, `value` TEXT, PRIMARY KEY (`key1`, `key2`)) WITHOUT ROWID"
  );

  myStmtInsert = make_unique<SqliteStatement>(myDb, "INSERT OR REPLACE INTO `" + myTableName + "` VALUES (?, ?, ?)");
  myStmtSelect = make_unique<SqliteStatement>(myDb, "SELECT `key2`, `VALUE` FROM `" + myTableName + "` WHERE `key1` = ?");
  myStmtCount = make_unique<SqliteStatement>(myDb, "SELECT COUNT(*) FROM `" + myTableName + "` WHERE `key1` = ?");
  myStmtDelete = make_unique<SqliteStatement>(myDb, "DELETE FROM `" + myTableName + "` WHERE `key1` = ? AND `key2` = ?");
  myStmtDeleteSet = make_unique<SqliteStatement>(myDb, "DELETE FROM `" + myTableName + "` WHERE `key1` = ?");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeKeyValueRepositorySqlite::ProxyRepository::ProxyRepository(
  const CompositeKeyValueRepositorySqlite& repo,
  const string& key
) : myRepo(repo), myKey(key)
{}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SqliteStatement& CompositeKeyValueRepositorySqlite::ProxyRepository::stmtInsert(
  const string& key, const string& value
) {
  myRepo.myStmtInsert->reset();

  return (*myRepo.myStmtInsert)
    .bind(1, myKey.c_str())
    .bind(2, key.c_str())
    .bind(3, value.c_str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SqliteStatement& CompositeKeyValueRepositorySqlite::ProxyRepository::stmtSelect()
{
  myRepo.myStmtSelect->reset();

  return (*myRepo.myStmtSelect)
    .bind(1, myKey.c_str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SqliteStatement& CompositeKeyValueRepositorySqlite::ProxyRepository::stmtDelete(const string& key)
{
  myRepo.myStmtDelete->reset();

  return (*myRepo.myStmtDelete)
    .bind(1, myKey.c_str())
    .bind(2, key.c_str());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SqliteDatabase& CompositeKeyValueRepositorySqlite::ProxyRepository::database()
{
  return myRepo.myDb;
}
