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
// $Id: FrontendUNIX.cxx,v 1.2 2003-09-06 21:17:48 stephena Exp $
//============================================================================

#include <cstdlib>
#include <sstream>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "bspf.hxx"
#include "Console.hxx"
#include "FrontendUNIX.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrontendUNIX::FrontendUNIX()
    : myPauseIndicator(false),
      myQuitIndicator(false)
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
string FrontendUNIX::snapshotFilename(string& md5, uInt32 state)
{
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
