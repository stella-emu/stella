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

#ifndef GAME_INFO_DIALOG_HXX
#define GAME_INFO_DIALOG_HXX

class OSystem;
class GuiObject;
class EditTextWidget;
class PopUpWidget;
class StaticTextWidget;
class RadioButtonGroup;
class TabWidget;
class SliderWidget;

#include "Dialog.hxx"
#include "Command.hxx"
#include "Props.hxx"

class GameInfoDialog : public Dialog, public CommandSender
{
  public:
    GameInfoDialog(OSystem& osystem, DialogContainer& parent,
                   const GUI::Font& font, GuiObject* boss, int max_w, int max_h);
    virtual ~GameInfoDialog() = default;

  private:
    void loadConfig() override;
    void saveConfig() override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

    void setDefaults() override;

    // load the properties for the 'Emulation' tab
    void loadEmulationProperties(const Properties& props);
    // load the properties for the 'Console' tab
    void loadConsoleProperties(const Properties& props);
    // load the properties for the 'Controller' tab
    void loadControllerProperties(const Properties& props);
    // load the properties for the 'Cartridge' tab
    void loadCartridgeProperties(const Properties& props);

    void updateControllerStates();
    void eraseEEPROM();

  private:
    TabWidget* myTab;

    // Emulation properties
    PopUpWidget*      myBSType;
    StaticTextWidget* myTypeDetected;
    StaticTextWidget* myStartBankLabel;
    PopUpWidget*      myStartBank;
    PopUpWidget*      myFormat;
    StaticTextWidget* myFormatDetected;
    SliderWidget*     myVCenter;
    CheckboxWidget*   myPhosphor;
    SliderWidget*     myPPBlend;
    CheckboxWidget*   mySound;

    // Console properties
    RadioButtonGroup* myLeftDiffGroup;
    RadioButtonGroup* myRightDiffGroup;
    RadioButtonGroup* myTVTypeGroup;

    // Controller properties
    StaticTextWidget* myLeftPortLabel;
    StaticTextWidget* myRightPortLabel;
    PopUpWidget*      myLeftPort;
    StaticTextWidget* myLeftPortDetected;
    PopUpWidget*      myRightPort;
    StaticTextWidget* myRightPortDetected;
    CheckboxWidget*   mySwapPorts;
    CheckboxWidget*   mySwapPaddles;
    StaticTextWidget* myEraseEEPROMLabel;
    ButtonWidget*     myEraseEEPROMButton;
    StaticTextWidget* myEraseEEPROMInfo;
    CheckboxWidget*   myMouseControl;
    PopUpWidget*      myMouseX;
    PopUpWidget*      myMouseY;
    SliderWidget*     myMouseRange;

    // Cartridge properties
    EditTextWidget*   myName;
    EditTextWidget*   myMD5;
    EditTextWidget*   myManufacturer;
    EditTextWidget*   myModelNo;
    EditTextWidget*   myRarity;
    EditTextWidget*   myNote;

    enum {
      kVCenterChanged  = 'Vcch',
      kPhosphorChanged = 'PPch',
      kPPBlendChanged  = 'PBch',
      kLeftCChanged    = 'LCch',
      kRightCChanged   = 'RCch',
      kMCtrlChanged    = 'MCch',
      kEEButtonPressed = 'EEgb',
    };

    // Game properties for currently loaded ROM
    Properties myGameProperties;

  private:
    // Following constructors and assignment operators not supported
    GameInfoDialog() = delete;
    GameInfoDialog(const GameInfoDialog&) = delete;
    GameInfoDialog(GameInfoDialog&&) = delete;
    GameInfoDialog& operator=(const GameInfoDialog&) = delete;
    GameInfoDialog& operator=(GameInfoDialog&&) = delete;
};

#endif
