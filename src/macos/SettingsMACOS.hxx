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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef SETTINGS_MACOS_HXX
#define SETTINGS_MACOS_HXX

class OSystem;

#include "Settings.hxx"

/**
  This class defines macOS system-specific settings.

  @author  Mark Grebe, Stephen Anthony
*/
class SettingsMACOS : public Settings
{
  public:
    /**
      Create a new UNIX settings object
    */
    explicit SettingsMACOS(OSystem& osystem);
    virtual ~SettingsMACOS() = default;

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
    SettingsMACOS() = delete;
    SettingsMACOS(const SettingsMACOS&) = delete;
    SettingsMACOS(SettingsMACOS&&) = delete;
    SettingsMACOS& operator=(const SettingsMACOS&) = delete;
    SettingsMACOS& operator=(SettingsMACOS&&) = delete;
};

#endif
