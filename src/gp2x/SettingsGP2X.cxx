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
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
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
  setInternal("center", "true");
  setInternal("volume", "33");
  setInternal("sound", "true");
  setInternal("zoom", "1");
  setInternal("fragsize", "256");
  setInternal("freq", "15700");
  setInternal("tiafreq", "15700");
  setInternal("clipvol", "false");
  setInternal("rombrowse", "true");
  setInternal("romdir", "/mnt/sd/");
  setInternal("ssdir", "/mnt/sd/");
  setInternal("p0speed", "15");
  setInternal("p1speed", "15");
  setInternal("p2speed", "15");
  setInternal("p3speed", "15");
  setInternal("launchersize", "1");
  setInternal("uipalette", "2");
  setInternal("tv_scale_width", "1.125");
  setInternal("tv_scale_height", "1.2");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsGP2X::~SettingsGP2X()
{
}
