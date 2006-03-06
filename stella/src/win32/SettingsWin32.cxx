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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: SettingsWin32.cxx,v 1.22 2006-03-06 21:41:58 stephena Exp $
//============================================================================

#include <sstream>
#include <fstream>
#include <direct.h>

#include "bspf.hxx"
#include "Settings.hxx"
#include "SettingsWin32.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsWin32::SettingsWin32(OSystem* osystem)
  : Settings(osystem)
{
  setInternal("fragsize", "2048");     // Anything less than this usually causes sound skipping
  setInternal("video", "hard");        // Use software mode with hardware surface
  setInternal("dirtyrects", "false");  // Most Windows systems work better without this
  setInternal("gl_lib", "opengl32.dll");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsWin32::~SettingsWin32()
{
}
