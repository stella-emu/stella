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
// $Id: FrontendUNIX.cxx,v 1.3 2003-09-12 18:08:54 stephena Exp $
//============================================================================

#include <cstdlib>
#include <sstream>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "bspf.hxx"
#include "Console.hxx"
#include "FrontendUNIX.hxx"
#include "Settings.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrontendUNIX::FrontendUNIX()
    : myPauseIndicator(false),
      myQuitIndicator(false),
      myConsole(0)
{
  myHomeDir = getenv("HOME");
  string path = myHomeDir + "/.stella";

  if(access(path.c_str(), R_OK|W_OK|X_OK) != 0 )
    mkdir(path.c_str(), 0777);

  myStateDir = myHomeDir + "/.stella/state/";
  if(access(myStateDir.c_str(), R_OK|W_OK|X_OK) != 0 )
    mkdir(myStateDir.c_str(), 0777);

  myHomePropertiesFile   = myHomeDir + "/.stella/stella.pro";
  mySystemPropertiesFile = "/etc/stella.pro";
  myHomeRCFile           = myHomeDir + "/.stella/stellarc";
  mySystemRCFile         = "/etc/stellarc";

  mySnapshotFile = "";
  myStateFile    = "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrontendUNIX::~FrontendUNIX()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrontendUNIX::setConsole(Console* console)
{
  myConsole = console;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrontendUNIX::setQuitEvent()
{
  myQuitIndicator = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrontendUNIX::quit()
{
  return myQuitIndicator;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrontendUNIX::setPauseEvent()
{
  myPauseIndicator = !myPauseIndicator;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrontendUNIX::pause()
{
  return myPauseIndicator;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FrontendUNIX::stateFilename(string& md5, uInt32 state)
{
  ostringstream buf;
  buf << myStateDir << md5 << ".st" << state;

  myStateFile = buf.str();
  return myStateFile;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FrontendUNIX::snapshotFilename()
{
  if(!myConsole)
    return "";

  string path = myConsole->settings().theSnapshotDir;
  string filename;

  if(myConsole->settings().theSnapshotName == "romname")
    path = path + "/" + myConsole->properties().get("Cartridge.Name");
  else if(myConsole->settings().theSnapshotName == "md5sum")
    path = path + "/" + myConsole->properties().get("Cartridge.MD5");

  // Replace all spaces in name with underscores
  replace(path.begin(), path.end(), ' ', '_');

  // Check whether we want multiple snapshots created
  if(myConsole->settings().theMultipleSnapshotFlag)
  {
    // Determine if the file already exists, checking each successive filename
    // until one doesn't exist
    filename = path + ".png";
    if(access(filename.c_str(), F_OK) == 0 )
    {
      ostringstream buf;
      for(uInt32 i = 1; ;++i)
      {
        buf.str("");
        buf << path << "_" << i << ".png";
        if(access(buf.str().c_str(), F_OK) == -1 )
          break;
      }
      filename = buf.str();
    }
  }
  else
    filename = path + ".png";

  mySnapshotFile = filename;
  return mySnapshotFile;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FrontendUNIX::userPropertiesFilename()
{
  return myHomePropertiesFile;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FrontendUNIX::systemPropertiesFilename()
{
  return mySystemPropertiesFile;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FrontendUNIX::userConfigFilename()
{
  return myHomeRCFile;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FrontendUNIX::systemConfigFilename()
{
  return mySystemRCFile;
}
