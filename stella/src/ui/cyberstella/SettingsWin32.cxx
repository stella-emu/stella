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
// Copyright (c) 1995-2002 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: SettingsWin32.cxx,v 1.4 2003-11-24 01:14:38 stephena Exp $
//============================================================================

#include <sstream>
#include <afxwin.h>

#include "bspf.hxx"
#include "Console.hxx"
#include "Settings.hxx"
#include "SettingsWin32.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsWin32::SettingsWin32()
{
  // First set variables that the parent class needs
  myBaseDir = ".\\"; // TODO - this should change to per-user location if using Windows XP

  myStateDir = myBaseDir + "state\\";
  CreateDirectory(myStateDir.c_str(), NULL);

  // TODO - these should reflect user vs. system files
  myUserPropertiesFile   = myBaseDir + "stella.pro";
  mySystemPropertiesFile = myBaseDir + "stella.pro";
  myUserConfigFile       = myBaseDir + "stellarc";
  mySystemConfigFile     = myBaseDir + "stellarc";

  // Set up the names of the input and output config files
  mySettingsOutputFilename = myUserConfigFile;
  mySettingsInputFilename  = mySystemConfigFile;

  mySnapshotFile = "";
  myStateFile    = "";

  // Now create Win32 specific settings
  set("autoselect_video", "false");
  set("disable_joystick", "true");
  set("rompath", "");
  set("sound", "win");
  set("volume", "-1");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsWin32::~SettingsWin32()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string SettingsWin32::stateFilename(uInt32 state)
{
  if(!myConsole)
    return "";

  ostringstream buf;
  buf << myStateDir << myConsole->properties().get("Cartridge.MD5")
      << ".st" << state;

  myStateFile = buf.str();
  return myStateFile; 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string SettingsWin32::snapshotFilename()
{
  return "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsWin32::usage(string& message)
{
}
