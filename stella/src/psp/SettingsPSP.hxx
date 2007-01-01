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
// $Id: SettingsPSP.hxx,v 1.3 2007-01-01 18:04:55 stephena Exp $
//============================================================================

#ifndef SETTINGS_PSP_HXX
#define SETTINGS_PSP_HXX

class OSystem;

#include "bspf.hxx"

/**
  This class defines PSP-like OS's (Linux) system specific settings.

  @author  Stephen Anthony
  @version $Id: SettingsPSP.hxx,v 1.3 2007-01-01 18:04:55 stephena Exp $
*/
class SettingsPSP : public Settings
{
  public:
    /**
      Create a new PSP settings object
    */
    SettingsPSP(OSystem* osystem);

    /**
      Destructor
    */
    virtual ~SettingsPSP();
};

#endif
