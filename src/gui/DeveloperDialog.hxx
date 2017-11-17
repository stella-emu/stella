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
  #if 0
    void handleKeyDown(StellaKey key, StellaMod mod) override;
    void handleJoyDown(int stick, int button) override;
    void handleJoyAxis(int stick, int axis, int value) override;
    bool handleJoyHat(int stick, int hat, int value) override;
  #endif
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

    void loadConfig() override;
    void saveConfig() override;
    void setDefaults() override;

  private:
    enum
    {
      kDevSettings0     = 'DVs0',
      kRandRAMID        = 'DVrm',
      kRandCPUID        = 'DVcp',
      kTVJitter         = 'DVjt',
      kTVJitterChanged  = 'DVjr',
      kPPinCmd          = 'DVpn',
      kDevSettings1     = 'DVs1',
  #ifdef DEBUGGER_SUPPORT
      kDWidthChanged    = 'UIdw',
      kDHeightChanged   = 'UIdh',
      kDSmallSize       = 'UIds',
      kDMediumSize      = 'UIdm',
      kDLargeSize       = 'UIdl'
  #endif
    };

    TabWidget* myTab;

    CheckboxWidget*   myDevSettings0;
    CheckboxWidget*   myDevSettings1;

    StaticTextWidget* myLoadingROMLabel;
    CheckboxWidget*   myRandomBank;
    CheckboxWidget*   myRandomizeRAM;
    StaticTextWidget* myRandomizeCPULabel;
    CheckboxWidget*   myRandomizeCPU[5];
    //CheckboxWidget*   myThumbException;

    CheckboxWidget*   myColorLoss;
    CheckboxWidget*   myTVJitter;
    SliderWidget*     myTVJitterRec;
    StaticTextWidget* myTVJitterRecLabel;
    CheckboxWidget*   myDebugColors;
    CheckboxWidget*   myUndrivenPins;

#ifdef DEBUGGER_SUPPORT
    // Debugger options
    SliderWidget*     myDebuggerWidthSlider;
    StaticTextWidget* myDebuggerWidthLabel;
    SliderWidget*     myDebuggerHeightSlider;
    StaticTextWidget* myDebuggerHeightLabel;
    PopUpWidget*      myDebuggerFontStyle;
#endif

    // Maximum width and height for this dialog
    int myMaxWidth, myMaxHeight;

  private:
    void addEmulationTab(const GUI::Font& font);
    //void addVideoTab(const GUI::Font& font);
    void addDebuggerTab(const GUI::Font& font);
    //void addUITab(const GUI::Font& font);
    void addStatesTab(const GUI::Font& font);
    // Add Defaults, OK and Cancel buttons
    void addDefaultOKCancelButtons(const GUI::Font& font);

    void enableOptions();
    void handleTVJitterChange(bool enable);
    void handleDebugColors();

    // Following constructors and assignment operators not supported
    DeveloperDialog() = delete;
    DeveloperDialog(const DeveloperDialog&) = delete;
    DeveloperDialog(DeveloperDialog&&) = delete;
    DeveloperDialog& operator=(const DeveloperDialog&) = delete;
    DeveloperDialog& operator=(DeveloperDialog&&) = delete;
};

#endif
