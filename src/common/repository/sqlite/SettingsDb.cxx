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

#include "SettingsDb.hxx"
#include "Logger.hxx"
#include "SqliteError.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsDb::SettingsDb(const string& databaseDirectory, const string& databaseName)
  : myDatabaseDirectory{databaseDirectory},
    myDatabaseName{databaseName}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsDb::initialize()
{
  try {
    myDb = make_unique<SqliteDatabase>(myDatabaseDirectory, myDatabaseName);
    myDb->initialize();

    mySettingsRepository = make_unique<KeyValueRepositorySqlite>(*myDb, "settings");
    mySettingsRepository->initialize();

    myPropertyRepositoryHost = make_unique<KeyValueRepositorySqlite>(*myDb, "properties");
    myPropertyRepositoryHost->initialize();

    myPropertyRepository = make_unique<CompositeKVRJsonAdapter>(*myPropertyRepositoryHost);
  }
  catch (const SqliteError& err) {
    Logger::error("sqlite DB " + databaseFileName() + " failed to initialize: " + err.what());

    myDb.reset();
    mySettingsRepository.reset();
    myPropertyRepository.reset();

    return false;
  }

  return true;
}
