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
// Copyright (c) 1995-1999 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: SettingsMACOSX.cxx,v 1.1.1.1 2004-06-16 02:30:30 markgrebe Exp $
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
#include "StellaEvent.hxx"

#include "Settings.hxx"
#include "SettingsMACOSX.hxx"

extern "C" {
void prefsSetString(char *key, char *value);
void prefsGetString(char *key, char *value);
void prefsSave(void);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsMACOSX::SettingsMACOSX()
{
  // First set variables that the parent class needs
  myBaseDir = "./";
  string stelladir = myBaseDir;

  myStateDir = stelladir + "state/";
  if(access(myStateDir.c_str(), R_OK|W_OK|X_OK) != 0 )
    mkdir(myStateDir.c_str(), 0777);

  myUserPropertiesFile   = stelladir + "stella.pro";
  mySystemPropertiesFile = stelladir + "stella.pro";
  myUserConfigFile       = "";
  mySystemConfigFile     = "";

  // Set up the names of the input and output config files
  mySettingsOutputFilename = myUserConfigFile;
  mySettingsInputFilename = myUserConfigFile;

  mySnapshotFile = "";
  myStateFile    = "";

  // Now create MacOSX specific settings
#ifdef SNAPSHOT_SUPPORT
  set("ssdir", "./");
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsMACOSX::~SettingsMACOSX()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsMACOSX::usage(string& message)
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
	prefsGetString((char *) mySettings[i].key.c_str(),cvalue);
	if (cvalue[0] != 0)
		mySettings[i].value.assign(cvalue);
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsMACOSX::saveConfig()
{
  // Write out each of the key and value pairs
  for(uInt32 i = 0; i < mySize; ++i)
    if(mySettings[i].save)
		{
		prefsSetString((char *) mySettings[i].key.c_str(), 
					   (char *) mySettings[i].value.c_str());
		}
   prefsSave();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string SettingsMACOSX::stateFilename(const string& md5, uInt32 state)
{
  ostringstream buf;
  buf << myStateDir << md5 << ".st" << state;

  myStateFile = buf.str();
  return myStateFile;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsMACOSX::fileExists(const string& filename)
{
  return (access(filename.c_str(), F_OK|W_OK) == 0);
}
