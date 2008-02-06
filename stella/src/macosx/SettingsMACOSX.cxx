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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: SettingsMACOSX.cxx,v 1.20 2008-02-06 13:45:24 stephena Exp $
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
  setInternal("video", "gl");        // Use opengl mode by default
  setInternal("gl_lib", "libGL.so"); // Try this one first, then let the system decide
  setInternal("gl_vsync", "true");   // OSX almost always supports vsync; let's use it
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
  // Write out each of the key and value pairs
  const SettingsArray& settings = getInternalSettings();
  for(unsigned int i = 0; i < settings.size(); ++i)
  {
    prefsSetString((char *) settings[i].key.c_str(),
                   (char *) settings[i].value.c_str());
  }
  prefsSave();
}
