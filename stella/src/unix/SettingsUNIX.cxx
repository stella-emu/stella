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
// $Id: SettingsUNIX.cxx,v 1.10 2005-02-22 18:41:16 stephena Exp $
//============================================================================

#include <cstdlib>

#include "bspf.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "SettingsUNIX.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsUNIX::SettingsUNIX(OSystem* osystem)
    : Settings(osystem)
{
  // First set variables that the OSystem needs
  string basedir = getenv("HOME");
  myOSystem->setBaseDir(basedir);

  string stelladir = basedir + "/.stella";
  if(!myOSystem->fileExists(stelladir))
    myOSystem->makeDir(stelladir);

  string statedir = stelladir + "/state/";
  if(!myOSystem->fileExists(statedir))
    myOSystem->makeDir(statedir);
  myOSystem->setStateDir(statedir);

  string userPropertiesFile   = stelladir + "/stella.pro";
  string systemPropertiesFile = "/etc/stella.pro";
  myOSystem->setPropertiesFiles(userPropertiesFile, systemPropertiesFile);

  string userConfigFile   = stelladir + "/stellarc";
  string systemConfigFile = "/etc/stellarc";
  myOSystem->setConfigFiles(userConfigFile, systemConfigFile);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsUNIX::~SettingsUNIX()
{
}
