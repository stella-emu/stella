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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
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
    virtual ~DeveloperDialog() = default;

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
      kGhostReads           = 'Dbgh'
  #endif
    };
    enum SettingsSet
    {
      player,
      developer
    };

    // MUST be aligned with RewindManager!
    static const int NUM_INTERVALS = 7;
    static const int NUM_HORIZONS = 8;

    static const int DEBUG_COLORS = 6;

    TabWidget* myTab;
    // Emulator widgets
    RadioButtonGroup*   mySettingsGroup0;
    CheckboxWidget*     myFrameStatsWidget;
    PopUpWidget*        myConsoleWidget;
    StaticTextWidget*   myLoadingROMLabel;
    CheckboxWidget*     myRandomBankWidget;
    CheckboxWidget*     myRandomizeRAMWidget;
    StaticTextWidget*   myRandomizeCPULabel;
    CheckboxWidget*     myRandomizeCPUWidget[5];
    CheckboxWidget*     myUndrivenPinsWidget;
    CheckboxWidget*     myThumbExceptionWidget;
    CheckboxWidget*     myEEPROMAccessWidget;

    // Video widgets
    RadioButtonGroup*   mySettingsGroup1;
    CheckboxWidget*     myTVJitterWidget;
    SliderWidget*       myTVJitterRecWidget;
    StaticTextWidget*   myTVJitterRecLabelWidget;
    CheckboxWidget*     myColorLossWidget;
    CheckboxWidget*     myDebugColorsWidget;
    PopUpWidget*        myDbgColour[DEBUG_COLORS];
    ColorWidget*        myDbgColourSwatch[DEBUG_COLORS];

    // States widgets
    RadioButtonGroup*   mySettingsGroup2;
    CheckboxWidget*     myTimeMachineWidget;
    SliderWidget*       myStateSizeWidget;
    SliderWidget*       myUncompressedWidget;
    PopUpWidget*        myStateIntervalWidget;
    PopUpWidget*        myStateHorizonWidget;

#ifdef DEBUGGER_SUPPORT
    // Debugger UI widgets
    SliderWidget*       myDebuggerWidthSlider;
    SliderWidget*       myDebuggerHeightSlider;
    PopUpWidget*        myDebuggerFontSize;
    PopUpWidget*        myDebuggerFontStyle;
    CheckboxWidget*     myGhostReadsTrapWidget;
#endif

    bool    mySettings;
    // Emulator sets
    bool    myFrameStats[2];
    int     myConsole[2];
    bool    myRandomBank[2];
    bool    myRandomizeRAM[2];
    string  myRandomizeCPU[2];
    bool    myColorLoss[2];
    bool    myTVJitter[2];
    int     myTVJitterRec[2];
    bool    myDebugColors[2];
    bool    myUndrivenPins[2];
    bool    myThumbException[2];
    bool    myEEPROMAccess[2];
    // States sets
    bool    myTimeMachine[2];
    int     myStateSize[2];
    int     myUncompressed[2];
    string  myStateInterval[2];
    string  myStateHorizon[2];

  private:
    void addEmulationTab(const GUI::Font& font);
    void addTimeMachineTab(const GUI::Font& font);
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
