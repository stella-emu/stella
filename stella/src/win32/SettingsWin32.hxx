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
// $Id: SettingsWin32.hxx,v 1.6 2005-06-16 01:11:29 stephena Exp $
//============================================================================

#ifndef SETTINGS_WIN32_HXX
#define SETTINGS_WIN32_HXX

class OSystem;

#include "bspf.hxx"


class SettingsWin32 : public Settings
{
  public:
    /**
      Create a new UNIX settings object
    */
    SettingsWin32(OSystem* osystem);

    /**
      Destructor
    */
    virtual ~SettingsWin32();
};

#endif
