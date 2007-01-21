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
// Windows CE Port by Kostas Nakos
// $Id: SettingsWinCE.cpp,v 1.7 2007-01-21 20:10:50 knakos Exp $
//============================================================================

#include <sstream>
#include <fstream>

#include "bspf.hxx"
#include "Settings.hxx"
#include "SettingsWinCE.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsWinCE::SettingsWinCE(OSystem* osystem) : Settings(osystem) 
{
  setInternal("romdir", (string) getcwd() + "\\Roms\\");
}

SettingsWinCE::~SettingsWinCE()
{
}
