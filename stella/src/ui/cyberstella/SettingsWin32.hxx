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
// $Id: SettingsWin32.hxx,v 1.2 2003-09-23 19:41:16 stephena Exp $
//============================================================================

#ifndef SETTINGS_WIN32_HXX
#define SETTINGS_WIN32_HXX

#include "bspf.hxx"
#include "Settings.hxx"

class SettingsWin32 : public Settings
{
  public:
    SettingsWin32();
    virtual ~SettingsWin32();

  public:
    virtual string stateFilename(uInt32 state);
    virtual string snapshotFilename();
	virtual void usage(string& message);
};

#endif
