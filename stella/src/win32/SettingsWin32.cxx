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
// $Id: SettingsWin32.cxx,v 1.8 2004-07-05 00:53:48 stephena Exp $
//============================================================================

#include <sstream>
#include <fstream>
#include <direct.h>

#include "bspf.hxx"
#include "Settings.hxx"
#include "SettingsWin32.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsWin32::SettingsWin32()
{
  // FIXME - there really should be code here to determine which version
  // of Windows is being used.
  // If using a version which supports multiple users (NT and above),
  // the relevant directories should be created in per-user locations.
  // For now, we just put it in the same directory as the executable.

  // First set variables that the parent class needs
  myBaseDir = ".\\";
  string stelladir = myBaseDir;

  myStateDir = stelladir + "state\\";
  _mkdir(myStateDir.c_str());

  myUserPropertiesFile   = stelladir + "stella.pro";
  mySystemPropertiesFile = stelladir + "stella.pro";
  myUserConfigFile       = stelladir + "stella.ini";
  mySystemConfigFile     = stelladir + "stella.ini";

  // Set up the names of the input and output config files
  mySettingsOutputFilename = myUserConfigFile;
  if(fileExists(myUserConfigFile))
    mySettingsInputFilename = myUserConfigFile;
  else
    mySettingsInputFilename = mySystemConfigFile;

  mySnapshotFile = "";
  myStateFile    = "";

  // Now create Win32 specific settings
  set("romdir", "roms");
  set("accurate", "false");  // Don't change this, or the sound will skip
  set("fragsize", "2048");   // Anything less than this usually causes sound skipping
#ifdef SNAPSHOT_SUPPORT
  set("ssdir", ".\\");
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsWin32::~SettingsWin32()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string SettingsWin32::stateFilename(const string& md5, uInt32 state)
{
  ostringstream buf;
  buf << myStateDir << md5 << ".st" << state;

  myStateFile = buf.str();

  return myStateFile;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsWin32::fileExists(const string& filename)
{
  // FIXME - Since I don't have time to figure out the correct
  // and fast 'Win32' way of doing this, I'll cheat a bit
  ifstream in(filename.c_str());
  if(in)
  {
    in.close();
    return true;
  }

  return false;
}
