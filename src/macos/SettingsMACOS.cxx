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

// TODO - Fix this code so that we don't need access to getPermanentSettings()
//        The code should parse the plist file and call setValue on each
//        option; it shouldn't need to query the base class for which options
//        are valid.

#include "SettingsMACOS.hxx"

extern "C" {
  void prefsSetString(const char* key, const char* value);
  void prefsGetString(const char* key, char* value, int size);
  void prefsSave(void);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsMACOS::SettingsMACOS()
  : Settings()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsMACOS::loadConfigFile(const string&)
{
  string key, value;
  char cvalue[4096];

  // Read key/value pairs from the plist file
  for(const auto& s: getPermanentSettings())
  {
    prefsGetString(s.first.c_str(), cvalue, 4090);
    if(cvalue[0] != 0)
      setValue(s.first, cvalue);
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsMACOS::saveConfigFile(const string&) const
{
  // Write out each of the key and value pairs
  for(const auto& s: getPermanentSettings())
    prefsSetString(s.first.c_str(), s.second.toCString());

  prefsSave();

  return true;
}
