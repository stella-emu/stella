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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef SETTINGS_WINDOWS_HXX
#define SETTINGS_WINDOWS_HXX

class OSystem;

#include "Settings.hxx"

class SettingsWINDOWS : public Settings
{
  public:
    /**
      Create a new UNIX settings object
    */
    SettingsWINDOWS(OSystem& osystem);

    /**
      Destructor
    */
    virtual ~SettingsWINDOWS();
};

#endif
