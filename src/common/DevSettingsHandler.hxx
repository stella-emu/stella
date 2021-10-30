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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef DEV_SETTINGS_HANDLER_HXX
#define DEV_SETTINGS_HANDLER_HXX

class OSystem;

#include <array>

/**
  This class takes care developer settings sets.

  @author  Thomas Jentzsch
*/
class DevSettingsHandler
{
  public:
    enum SettingsSet {
      player,
      developer,
      numSettings
    };

    DevSettingsHandler(OSystem& osystem);

    void loadSettings(SettingsSet set);
    void saveSettings(SettingsSet set);
    void applySettings(SettingsSet set);

  protected:
    OSystem& myOSystem;
    // Emulator sets
    std::array<bool, numSettings>   myFrameStats;
    std::array<bool, numSettings>   myDetectedInfo;
    std::array<bool, numSettings>   myExternAccess;
    std::array<int, numSettings>    myConsole;
    std::array<bool, numSettings>   myRandomBank;
    std::array<bool, numSettings>   myRandomizeTIA;
    std::array<bool, numSettings>   myRandomizeRAM;
    std::array<string, numSettings> myRandomizeCPU;
    std::array<bool, numSettings>   myColorLoss;
    std::array<bool, numSettings>   myTVJitter;
    std::array<int, numSettings>    myTVJitterRec;
    std::array<bool, numSettings>   myDebugColors;
    std::array<bool, numSettings>   myUndrivenPins;
  #ifdef DEBUGGER_SUPPORT
    std::array<bool, numSettings>   myRWPortBreak;
    std::array<bool, numSettings>   myWRPortBreak;
  #endif
    std::array<bool, numSettings>   myThumbException;
    // TIA sets
    std::array<string, numSettings> myTIAType;
    std::array<bool, numSettings>   myPlInvPhase;
    std::array<bool, numSettings>   myMsInvPhase;
    std::array<bool, numSettings>   myBlInvPhase;
    std::array<bool, numSettings>   myPFBits;
    std::array<bool, numSettings>   myPFColor;
    std::array<bool, numSettings>   myBKColor;
    std::array<bool, numSettings>   myPlSwap;
    std::array<bool, numSettings>   myBlSwap;
    // States sets
    std::array<bool, numSettings>   myTimeMachine;
    std::array<int, numSettings>    myStateSize;
    std::array<int, numSettings>    myUncompressed;
    std::array<string, numSettings> myStateInterval;
    std::array<string, numSettings> myStateHorizon;

  private:
    void handleEnableDebugColors(bool enable);

    // Following constructors and assignment operators not supported
    DevSettingsHandler() = delete;
    DevSettingsHandler(const DevSettingsHandler&) = delete;
    DevSettingsHandler(DevSettingsHandler&&) = delete;
    DevSettingsHandler& operator=(const DevSettingsHandler&) = delete;
    DevSettingsHandler& operator=(DevSettingsHandler&&) = delete;
};

#endif
