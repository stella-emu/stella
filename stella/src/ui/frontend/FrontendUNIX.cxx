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
// $Id: FrontendUNIX.cxx,v 1.1 2003-09-05 18:02:58 stephena Exp $
//============================================================================

#ifndef FRONTEND_UNIX_HXX
#define FRONTEND_UNIX_HXX

#include "bspf.hxx"
#include "Console.hxx"
#include "FrontendUNIX.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrontendUNIX::FrontendUNIX()
{
  myHomeDir = getenv("HOME");
  string path = homeDir + "/.stella";

  if(access(path.c_str(), R_OK|W_OK|X_OK) != 0 )
    mkdir(path.c_str(), 0777);

  myStateDir = myHomeDir + "/.stella/state/";
  if(access(myStateDir.c_str(), R_OK|W_OK|X_OK) != 0 )
    mkdir(myStateDir.c_str(), 0777);

  myHomePropertiesFile   = myHomeDir + "/.stella/stella.pro";
  mySystemPropertiesFile = "/etc/stella.pro";
  myHomeRCFile           = myHomeDir + "/.stella/stellarc";
  mySystemRCFile         = "/etc/stellarc";

  mySnapshotFilename = "";
  myStateFilename = "";
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
void FrontendUNIX::quit()
{
  theQuitIndicator = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrontendUNIX::pause(bool status)
{
  thePauseIndicator = status;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string& FrontendUNIX::stateFilename(string& md5, uInt32 state)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string& FrontendUNIX::snapshotFilename(string& md5, uInt32 state)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string& FrontendUNIX::userPropertiesFilename()
{
  return myHomePropertiesFile;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string& FrontendUNIX::systemPropertiesFilename()
{
  return mySystemPropertiesFile;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string& FrontendUNIX::userConfigFilename()
{
  return myHomeRCFile;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string& FrontendUNIX::systemConfigFilename()
{
  return mySystemRCFile;
}
