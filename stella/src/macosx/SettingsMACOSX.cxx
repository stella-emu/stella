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
// Copyright (c) 1995-2006 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: SettingsMACOSX.cxx,v 1.16 2006-12-18 18:35:26 stephena Exp $
//============================================================================

#include <cassert>
#include <sstream>
#include <fstream>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "bspf.hxx"
#include "Console.hxx"
#include "EventHandler.hxx"
#include "Version.hxx"

#include "Settings.hxx"
#include "SettingsMACOSX.hxx"

extern "C" {
  void prefsSetString(char *key, char *value);
  void prefsGetString(char *key, char *value);
  void prefsSave(void);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsMACOSX::SettingsMACOSX(OSystem* osystem)
  : Settings(osystem)
{
  setInternal("video", "gl");  // Use opengl mode by default
  setInternal("gl_lib", "");   // Let the system decide which lib to use
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsMACOSX::~SettingsMACOSX()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsMACOSX::loadConfig()
{
  string key, value;
  char cvalue[2048];
  
  // Check if the settings plist file is valid
  prefsGetString("stella_version", cvalue);
  if(cvalue[0] == 0 || string(cvalue) < string(STELLA_SETTINGS_VERSION))
    return;

  // Read key/value pairs from the plist file
  const SettingsArray& settings = getInternalSettings();
  for(unsigned int i = 0; i < settings.size(); ++i)
  {
    prefsGetString((char *) settings[i].key.c_str(), cvalue);
    if(cvalue[0] != 0)
      setInternal(settings[i].key, cvalue, i, true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsMACOSX::saveConfig()
{
  // Write out plist version
  prefsSetString("stella_version", STELLA_VERSION);

  // Write out each of the key and value pairs
  const SettingsArray& settings = getInternalSettings();
  for(unsigned int i = 0; i < settings.size(); ++i)
  {
    prefsSetString((char *) settings[i].key.c_str(),
                   (char *) settings[i].value.c_str());
  }
  prefsSave();
}
