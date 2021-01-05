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

#ifndef DEVELOPER_DIALOG_HXX
#define DEVELOPER_DIALOG_HXX

class OSystem;
class GuiObject;
class TabWidget;
class CheckboxWidget;
class EditTextWidget;
class PopUpWidget;
class RadioButtonGroup;
class RadioButtonWidget;
class SliderWidget;
class StaticTextWidget;
class ColorWidget;

namespace GUI {
  class Font;
}

#include "bspf.hxx"
#include "Dialog.hxx"

class DeveloperDialog : public Dialog
{
  public:
    DeveloperDialog(OSystem& osystem, DialogContainer& parent,
                const GUI::Font& font, int max_w, int max_h);
    ~DeveloperDialog() override = default;

  private:
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

    void loadConfig() override;
    void saveConfig() override;
    void setDefaults() override;

  private:
    enum
    {
      kPlrSettings          = 'DVpl',
      kDevSettings          = 'DVdv',
      kConsole              = 'DVco',
      kRandRAMID            = 'DVrm',
      kRandCPUID            = 'DVcp',
      kTIAType              = 'DVtt',
      kTVJitter             = 'DVjt',
      kTVJitterChanged      = 'DVjr',
      kPPinCmd              = 'DVpn',
      kTimeMachine          = 'DTtm',
      kSizeChanged          = 'DTsz',
      kUncompressedChanged  = 'DTuc',
      kIntervalChanged      = 'DTin',
      kHorizonChanged       = 'DThz',
      kP0ColourChangedCmd   = 'GOp0',
      kM0ColourChangedCmd   = 'GOm0',
      kP1ColourChangedCmd   = 'GOp1',
      kM1ColourChangedCmd   = 'GOm1',
      kPFColourChangedCmd   = 'GOpf',
      kBLColourChangedCmd   = 'GObl',
  #ifdef DEBUGGER_SUPPORT
      kDFontSizeChanged     = 'UIfs',
  #endif
    };
    enum SettingsSet { player = 0, developer = 1 };

    // MUST be aligned with RewindManager!
    static constexpr int NUM_INTERVALS = 7;
    static constexpr int NUM_HORIZONS = 8;

    static constexpr int DEBUG_COLORS = 6;

    TabWidget* myTab{nullptr};
    // Emulator widgets
    RadioButtonGroup*   mySettingsGroupEmulation{nullptr};
    CheckboxWidget*     myFrameStatsWidget{nullptr};
    CheckboxWidget*     myDetectedInfoWidget{nullptr};
    PopUpWidget*        myConsoleWidget{nullptr};
    StaticTextWidget*   myLoadingROMLabel{nullptr};
    CheckboxWidget*     myRandomBankWidget{nullptr};
    CheckboxWidget*     myRandomizeRAMWidget{nullptr};
    StaticTextWidget*   myRandomizeCPULabel{nullptr};
    CheckboxWidget*     myUndrivenPinsWidget{nullptr};
    std::array<CheckboxWidget*, 5> myRandomizeCPUWidget{nullptr};
#ifdef DEBUGGER_SUPPORT
    CheckboxWidget*     myRWPortBreakWidget{nullptr};
    CheckboxWidget*     myWRPortBreakWidget{nullptr};
#endif
    CheckboxWidget*     myThumbExceptionWidget{nullptr};
    CheckboxWidget*     myEEPROMAccessWidget{nullptr};

    // TIA widgets
    RadioButtonGroup*   mySettingsGroupTia{nullptr};
    PopUpWidget*        myTIATypeWidget{nullptr};
    StaticTextWidget*   myInvPhaseLabel{nullptr};
    CheckboxWidget*     myPlInvPhaseWidget{nullptr};
    CheckboxWidget*     myMsInvPhaseWidget{nullptr};
    CheckboxWidget*     myBlInvPhaseWidget{nullptr};
    StaticTextWidget*   myPlayfieldLabel{nullptr};
    CheckboxWidget*     myPFBitsWidget{nullptr};
    CheckboxWidget*     myPFColorWidget{nullptr};
    StaticTextWidget*   myBackgroundLabel{nullptr};
    CheckboxWidget*     myBKColorWidget{nullptr};
    StaticTextWidget*   mySwapLabel{nullptr};
    CheckboxWidget*     myPlSwapWidget{nullptr};
    CheckboxWidget*     myBlSwapWidget{nullptr};

    // Video widgets
    RadioButtonGroup*   mySettingsGroupVideo{nullptr};
    CheckboxWidget*     myTVJitterWidget{nullptr};
    SliderWidget*       myTVJitterRecWidget{nullptr};
    StaticTextWidget*   myTVJitterRecLabelWidget{nullptr};
    CheckboxWidget*     myColorLossWidget{nullptr};
    CheckboxWidget*     myDebugColorsWidget{nullptr};
    std::array<PopUpWidget*, DEBUG_COLORS> myDbgColour{nullptr};
    std::array<ColorWidget*, DEBUG_COLORS> myDbgColourSwatch{nullptr};

    // States widgets
    RadioButtonGroup*   mySettingsGroupTM{nullptr};
    CheckboxWidget*     myTimeMachineWidget{nullptr};
    SliderWidget*       myStateSizeWidget{nullptr};
    SliderWidget*       myUncompressedWidget{nullptr};
    PopUpWidget*        myStateIntervalWidget{nullptr};
    PopUpWidget*        myStateHorizonWidget{nullptr};

#ifdef DEBUGGER_SUPPORT
    // Debugger UI widgets
    SliderWidget*       myDebuggerWidthSlider{nullptr};
    SliderWidget*       myDebuggerHeightSlider{nullptr};
    PopUpWidget*        myDebuggerFontSize{nullptr};
    PopUpWidget*        myDebuggerFontStyle{nullptr};
    CheckboxWidget*     myGhostReadsTrapWidget{nullptr};
#endif

    bool    mySettings{false};
    // Emulator sets
    std::array<bool, 2>   myFrameStats;
    std::array<bool, 2>   myDetectedInfo;
    std::array<int, 2>    myConsole;
    std::array<bool, 2>   myRandomBank;
    std::array<bool, 2>   myRandomizeRAM;
    std::array<string, 2> myRandomizeCPU;
    std::array<bool, 2>   myColorLoss;
    std::array<bool, 2>   myTVJitter;
    std::array<int, 2>    myTVJitterRec;
    std::array<bool, 2>   myDebugColors;
    std::array<bool, 2>   myUndrivenPins;
#ifdef DEBUGGER_SUPPORT
    std::array<bool, 2>   myRWPortBreak;
    std::array<bool, 2>   myWRPortBreak;
#endif
    std::array<bool, 2>   myThumbException;
    std::array<bool, 2>   myEEPROMAccess;
    // TIA sets
    std::array<string, 2> myTIAType;
    std::array<bool, 2>   myPlInvPhase;
    std::array<bool, 2>   myMsInvPhase;
    std::array<bool, 2>   myBlInvPhase;
    std::array<bool, 2>   myPFBits;
    std::array<bool, 2>   myPFColor;
    std::array<bool, 2>   myBKColor;
    std::array<bool, 2>   myPlSwap;
    std::array<bool, 2>   myBlSwap;
    // States sets
    std::array<bool, 2>   myTimeMachine;
    std::array<int, 2>    myStateSize;
    std::array<int, 2>    myUncompressed;
    std::array<string, 2> myStateInterval;
    std::array<string, 2> myStateHorizon;

  private:
    void addEmulationTab(const GUI::Font& font);
    void addTimeMachineTab(const GUI::Font& font);
    void addTiaTab(const GUI::Font& font);
    void addVideoTab(const GUI::Font& font);
    void addDebuggerTab(const GUI::Font& font);

    void loadSettings(SettingsSet set);
    void saveSettings(SettingsSet set);
    void getWidgetStates(SettingsSet set);
    void setWidgetStates(SettingsSet set);

    void handleSettings(bool devSettings);
    void handleTVJitterChange(bool enable);
    void handleEnableDebugColors();
    void handleConsole();

    void handleTia();

    void handleDebugColours(int cmd, int color);
    void handleDebugColours(const string& colors);

    void handleTimeMachine();
    void handleSize();
    void handleUncompressed();
    void handleInterval();
    void handleHorizon();
    void handleFontSize();

    // Following constructors and assignment operators not supported
    DeveloperDialog() = delete;
    DeveloperDialog(const DeveloperDialog&) = delete;
    DeveloperDialog(DeveloperDialog&&) = delete;
    DeveloperDialog& operator=(const DeveloperDialog&) = delete;
    DeveloperDialog& operator=(DeveloperDialog&&) = delete;
};

#endif
