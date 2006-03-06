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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: SettingsMACOSX.cxx,v 1.9 2006-03-06 02:26:16 stephena Exp $
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
  setInternal("gl_lib", "libGL.so");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsMACOSX::~SettingsMACOSX()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsMACOSX::loadConfig()
{
  string key, value;
  char cvalue[1024];
  
  // Write out each of the key and value pairs
  for(uInt32 i = 0; i < mySize; ++i)
  {
    prefsGetString((char *) mySettings[i].key.c_str(), cvalue);
    if(cvalue[0] != 0)
    {
      mySettings[i].value.assign(cvalue);
      mySettings[i].save = true;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsMACOSX::saveConfig()
{
  // Write out each of the key and value pairs
  for(uInt32 i = 0; i < mySize; ++i)
  {
    if(mySettings[i].save)
    {
      prefsSetString((char *) mySettings[i].key.c_str(), 
                     (char *) mySettings[i].value.c_str());
    }
  }
  prefsSave();
}
