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
// $Id: SettingsUNIX.cxx,v 1.7 2004-08-17 01:17:08 stephena Exp $
//============================================================================

#include <cstdlib>
#include <sstream>
#include <fstream>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "bspf.hxx"
#include "Settings.hxx"
#include "SettingsUNIX.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsUNIX::SettingsUNIX()
{
  // First set variables that the parent class needs
  myBaseDir = getenv("HOME");
  string stelladir = myBaseDir + "/.stella";

  if(!fileExists(stelladir))
    mkdir(stelladir.c_str(), 0777);

  myStateDir = stelladir + "/state/";
  if(!fileExists(myStateDir))
    mkdir(myStateDir.c_str(), 0777);

  string userPropertiesFile   = stelladir + "/stella.pro";
  string systemPropertiesFile = "/etc/stella.pro";
  string userConfigFile       = stelladir + "/stellarc";
  string systemConfigFile     = "/etc/stellarc";

  // Set up the names of the input and output config files
  myConfigOutputFile = userConfigFile;
  if(fileExists(userConfigFile))
    myConfigInputFile = userConfigFile;
  else if(fileExists(systemConfigFile))
    myConfigInputFile = systemConfigFile;
  else
    myConfigInputFile = "";

  // Set up the input and output properties files
  myPropertiesOutputFile = userPropertiesFile;
  if(fileExists(userPropertiesFile))
    myPropertiesInputFile = userPropertiesFile;
  else if(fileExists(systemPropertiesFile))
    myPropertiesInputFile = systemPropertiesFile;
  else
    myPropertiesInputFile = "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsUNIX::~SettingsUNIX()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string SettingsUNIX::stateFilename(const string& md5, uInt32 state)
{
  ostringstream buf;
  buf << myStateDir << md5 << ".st" << state;

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsUNIX::fileExists(const string& filename)
{
  return (access(filename.c_str(), F_OK) == 0);
}
