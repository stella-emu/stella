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
// $Id: SettingsGP2X.cxx,v 1.6 2006-02-06 01:38:50 azaballa Exp $
// Modified on 2006/02/05 by Alex Zaballa for use on GP2X
//============================================================================

#include "bspf.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "SettingsGP2X.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsGP2X::SettingsGP2X(OSystem* osystem)
  : Settings(osystem)
{
  // Some of these settings might be redundant, but are crucial for GP2X
  set("center", "true");
  set("volume", "50");
  set("sound", "true");
  set("zoom", "1");
  set("fragsize", "512");
  set("tiafreq", "22050");
  set("clipvol", "false");
  set("joymouse", "true");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsGP2X::~SettingsGP2X()
{
}
