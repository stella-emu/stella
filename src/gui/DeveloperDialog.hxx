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

#ifndef DEVELOPER_DIALOG_HXX
#define DEVELOPER_DIALOG_HXX

class OSystem;
class GuiObject;
class TabWidget;
class EventMappingWidget;
class CheckboxWidget;
class EditTextWidget;
class PopUpWidget;
class RadioButtonGroup;
class RadioButtonWidget;
class SliderWidget;
class StaticTextWidget;

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
      kPlrSettings      = 'DVpl',
      kDevSettings      = 'DVdv',
      kConsole          = 'DVco',
      kRandRAMID        = 'DVrm',
      kRandCPUID        = 'DVcp',
      kTVJitter         = 'DVjt',
      kTVJitterChanged  = 'DVjr',
      kPPinCmd          = 'DVpn',
      kRewind           = 'DSrw',
      kSizeChanged      = 'DSsz',
      kIntervalChanged  = 'DSin',
      kHorizonChanged   = 'DShz',
  #ifdef DEBUGGER_SUPPORT
      kDWidthChanged    = 'UIdw',
      kDHeightChanged   = 'UIdh',
      kDFontSizeChanged = 'UIfs',
  #endif
    };
    enum SettingsSet
    {
      player,
      developer
    };

    static const int NUM_INTERVALS = 6;
    // TODO: check for intervals shorter than 1 frame (adjust horizon too!)
    const string INTERVALS[NUM_INTERVALS] = { "1 scanline", "50 scanlines", "1 frame", "10 frames",
      "1 second", "10 seconds" };
    const uInt32 INTERVAL_CYCLES[NUM_INTERVALS] = { 76, 76 * 50, 76 * 262, 76 * 262 * 10,
      76 * 262 * 60, 76 * 262 * 60 * 10 };
    static const int NUM_HORIZONS = 7;
    const string HORIZONS[NUM_HORIZONS] = { "~1 frame", "~10 frames", "~1 second", "~10 seconds",
      "~1 minute", "~10 minutes", "~60 minutes" };
    const uInt64 HORIZON_CYCLES[NUM_HORIZONS] = { 76 * 262, 76 * 262 * 10, 76 * 262 * 60, 76 * 262 * 60 * 10,
      76 * 262 * 60 * 60, 76 * 262 * 60 * 60 * 10, (uInt64)76 * 262 * 60 * 60 * 60 };

    TabWidget* myTab;
    // Emulator
    RadioButtonGroup*   mySettingsGroup;
    CheckboxWidget*     myFrameStatsWidget;
    PopUpWidget*        myConsoleWidget;
    StaticTextWidget*   myLoadingROMLabel;
    CheckboxWidget*     myRandomBankWidget;
    CheckboxWidget*     myRandomizeRAMWidget;
    StaticTextWidget*   myRandomizeCPULabel;
    CheckboxWidget*     myRandomizeCPUWidget[5];
    CheckboxWidget*     myColorLossWidget;
    CheckboxWidget*     myTVJitterWidget;
    SliderWidget*       myTVJitterRecWidget;
    StaticTextWidget*   myTVJitterRecLabelWidget;
    CheckboxWidget*     myDebugColorsWidget;
    CheckboxWidget*     myUndrivenPinsWidget;
    // States
    CheckboxWidget*     myContinuousRewind;
    SliderWidget*       myStateSize;
    StaticTextWidget*   myStateSizeLabel;
    SliderWidget*       myStateInterval;
    StaticTextWidget*   myStateIntervalLabel;
    SliderWidget*       myStateHorizon;
    StaticTextWidget*   myStateHorizonLabel;

#ifdef DEBUGGER_SUPPORT
    // Debugger UI
    SliderWidget*       myDebuggerWidthSlider;
    StaticTextWidget*   myDebuggerWidthLabel;
    SliderWidget*       myDebuggerHeightSlider;
    StaticTextWidget*   myDebuggerHeightLabel;
    PopUpWidget*        myDebuggerFontSize;
    PopUpWidget*        myDebuggerFontStyle;
#endif

    // Maximum width and height for this dialog
    int myMaxWidth, myMaxHeight;

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

  private:
    void addEmulationTab(const GUI::Font& font);
    void addDebuggerTab(const GUI::Font& font);
    void addStatesTab(const GUI::Font& font);
    // Add Defaults, OK and Cancel buttons
    void addDefaultOKCancelButtons(const GUI::Font& font);

    void copySettingsToSet(SettingsSet set);
    void copySetToSettings(SettingsSet set);
    void copyStateToSet(SettingsSet set);
    void copySetToState(SettingsSet set);

    void handleSettings(bool devSettings);
    void handleTVJitterChange(bool enable);
    void handleDebugColors();
    void handleConsole();
    void handleRewind();
    void handleSize();
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
