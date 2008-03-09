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
// $Id: OSystemWin32.cxx,v 1.22 2008-03-09 17:52:40 stephena Exp $
//============================================================================

#include <sstream>
#include <fstream>
#include <windows.h>

#include "bspf.hxx"
#include "FSNode.hxx"
#include "OSystem.hxx"
#include "OSystemWin32.hxx"

/**
  Each derived class is responsible for calling the following methods
  in its constructor:

  setBaseDir()
  setConfigFile()

  See OSystem.hxx for a further explanation
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemWin32::OSystemWin32()
  : OSystem()
{
  string basedir = ".";

  if(!FilesystemNode::fileExists("disable_profiles.txt"))
  {
    OSVERSIONINFO win32OsVersion;
    ZeroMemory(&win32OsVersion, sizeof(OSVERSIONINFO));
    win32OsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&win32OsVersion);

    // Check for non-9X version of Windows; Win9x will use the app directory
    if(win32OsVersion.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS)
    {
      // If this doesn't exist, we just fall back to the default (same directory as app)
      char configFile[256];
      if(GetEnvironmentVariable("USERPROFILE", configFile, sizeof(configFile)))
      {
        strcat(configFile, "\\Stella");
        basedir = configFile;
      }
    }
  }

  setBaseDir(basedir);
  setConfigFile(basedir + "\\stella.ini");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemWin32::~OSystemWin32()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 OSystemWin32::getTicks()
{
  return (uInt32) SDL_GetTicks() * 1000;
}
