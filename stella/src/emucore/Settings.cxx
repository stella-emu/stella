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
// $Id: Settings.cxx,v 1.4 2003-09-12 18:08:53 stephena Exp $
//============================================================================

#include "bspf.hxx"
#include "Settings.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::Settings()
{
  theKeymapList = "";
  theJoymapList = "";
  theSnapshotDir = "";
  theSnapshotName = "romname";
  theMultipleSnapshotFlag = true;
  theZoomLevel = 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::Settings(const string& infile, const string& outfile)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::~Settings()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::Settings(const Settings&)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings& Settings::operator = (const Settings&)
{
  assert(false);

  return *this;
}
