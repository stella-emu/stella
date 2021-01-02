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

#include <sstream>

#include "SettingsDb.hxx"
#include "Logger.hxx"
#include "SqliteError.hxx"
#include "repository/KeyValueRepositoryNoop.hxx"
#include "repository/CompositeKeyValueRepositoryNoop.hxx"
#include "repository/CompositeKVRJsonAdapter.hxx"
#include "repository/KeyValueRepositoryConfigfile.hxx"
#include "KeyValueRepositorySqlite.hxx"
#include "SqliteTransaction.hxx"
#include "FSNode.hxx"

namespace {
  constexpr Int32 CURRENT_VERSION = 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsDb::SettingsDb(const string& databaseDirectory, const string& databaseName)
  : myDatabaseDirectory{databaseDirectory},
    myDatabaseName{databaseName}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsDb::initialize()
{
  try {
    myDb = make_unique<SqliteDatabase>(myDatabaseDirectory, myDatabaseName);
    myDb->initialize();

    auto settingsRepository = make_unique<KeyValueRepositorySqlite>(*myDb, "settings", "setting", "value");
    settingsRepository->initialize();
    mySettingsRepository = std::move(settingsRepository);

    auto propertyRepositoryHost = make_unique<KeyValueRepositorySqlite>(*myDb, "properties", "md5", "properties");
    propertyRepositoryHost->initialize();
    myPropertyRepositoryHost = std::move(propertyRepositoryHost);

    myPropertyRepository = make_unique<CompositeKVRJsonAdapter>(*myPropertyRepositoryHost);

    if (myDb->getUserVersion() == 0) {
      initializeDb();
    } else {
      migrate();
    }
  }
  catch (const SqliteError& err) {
    Logger::error("sqlite DB " + databaseFileName() + " failed to initialize: " + err.what());

    mySettingsRepository = make_unique<KeyValueRepositoryNoop>();
    myPropertyRepository = make_unique<CompositeKeyValueRepositoryNoop>();

    myDb.reset();
    myPropertyRepositoryHost.reset();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsDb::initializeDb() {
  SqliteTransaction tx{*myDb};

  FilesystemNode legacyConfigFile{myDatabaseDirectory};
  legacyConfigFile /= "stellarc";

  FilesystemNode legacyConfigDatabase{myDatabaseDirectory};
  legacyConfigDatabase /= "settings.sqlite3";

  if (legacyConfigFile.exists() && legacyConfigFile.isFile()) {
    Logger::info("importing old settings from " + legacyConfigFile.getPath());

    mySettingsRepository->save(KeyValueRepositoryConfigfile{legacyConfigFile}.load());
  }

  myDb->setUserVersion(CURRENT_VERSION);

  tx.commit();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsDb::migrate() {
  Int32 version = myDb->getUserVersion();
  switch (version) {
    case 1:
      return;

    default: {
      stringstream ss;
      ss << "invalid database version " << version;

      throw new SqliteError(ss.str());
    }
  }
}
