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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef SETTINGS_MAC_OSX_HXX
#define SETTINGS_MAC_OSX_HXX

class OSystem;

#include "Settings.hxx"

/**
  This class defines Macintosh OSX system specific settings.

  @author  Mark Grebe
*/
class SettingsMACOSX : public Settings
{
  public:
    /**
      Create a new UNIX settings object
    */
    SettingsMACOSX(OSystem& osystem);
    virtual ~SettingsMACOSX() = default;

  public:
    /**
      This method should be called to load the current settings from the
      standard Mac preferences.
    */
    void loadConfig() override;

    /**
      This method should be called to save the current settings to the
      standard Mac preferences.
    */
    void saveConfig() override;

  private:
    // Following constructors and assignment operators not supported
    SettingsMACOSX() = delete;
    SettingsMACOSX(const SettingsMACOSX&) = delete;
    SettingsMACOSX(SettingsMACOSX&&) = delete;
    SettingsMACOSX& operator=(const SettingsMACOSX&) = delete;
    SettingsMACOSX& operator=(SettingsMACOSX&&) = delete;
};

#endif
