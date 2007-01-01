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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: OSystemWin32.cxx,v 1.17 2007-01-01 18:04:56 stephena Exp $
//============================================================================

#include <sstream>
#include <fstream>
#include <windows.h>

#include "bspf.hxx"
#include "OSystem.hxx"
#include "OSystemWin32.hxx"

/**
  Each derived class is responsible for calling the following methods
  in its constructor:

  setBaseDir()
  setStateDir()
  setPropertiesDir()
  setConfigFile()
  setCacheFile()

  See OSystem.hxx for a further explanation
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemWin32::OSystemWin32(const string& path) : OSystem()
{
  // TODO - there really should be code here to determine which version
  // of Windows is being used.
  // If using a version which supports multiple users (NT and above),
  // the relevant directories should be created in per-user locations.
  // For now, we just put it in the same directory as the executable.
  const string& basedir = (path.length() > 0) ? path : ".";

  setBaseDir(basedir);

  setStateDir(basedir + "\\state\\");

  setPropertiesDir(basedir);
  setConfigFile(basedir + "\\stella.ini");

  setCacheFile(basedir + "\\stella.cache");
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemWin32::getScreenDimensions(int& width, int& height)
{
  width  = (int) GetSystemMetrics(SM_CXSCREEN);
  height = (int) GetSystemMetrics(SM_CYSCREEN);
}
