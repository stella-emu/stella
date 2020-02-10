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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
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
#include "HighScoresManager.hxx"

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
    // load the properties for the 'High Scores' tab
    void loadHighScoresProperties(const Properties& props);
    // load the properties of the 'High Scores' tab
    void saveHighScoresProperties();

    void updateControllerStates();
    void eraseEEPROM();
    void handleHighScoresWidgets();
    void setAddressVal(const EditTextWidget* address, EditTextWidget* val, bool isBCD = true, uInt8 incVal = 0);

  private:
    TabWidget* myTab{nullptr};

    // Emulation properties
    PopUpWidget*      myBSType{nullptr};
    StaticTextWidget* myTypeDetected{nullptr};
    StaticTextWidget* myStartBankLabel{nullptr};
    PopUpWidget*      myStartBank{nullptr};
    PopUpWidget*      myFormat{nullptr};
    StaticTextWidget* myFormatDetected{nullptr};
    SliderWidget*     myVCenter{nullptr};
    CheckboxWidget*   myPhosphor{nullptr};
    SliderWidget*     myPPBlend{nullptr};
    CheckboxWidget*   mySound{nullptr};

    // Console properties
    RadioButtonGroup* myLeftDiffGroup{nullptr};
    RadioButtonGroup* myRightDiffGroup{nullptr};
    RadioButtonGroup* myTVTypeGroup{nullptr};

    // Controller properties
    StaticTextWidget* myLeftPortLabel{nullptr};
    StaticTextWidget* myRightPortLabel{nullptr};
    PopUpWidget*      myLeftPort{nullptr};
    StaticTextWidget* myLeftPortDetected{nullptr};
    PopUpWidget*      myRightPort{nullptr};
    StaticTextWidget* myRightPortDetected{nullptr};
    CheckboxWidget*   mySwapPorts{nullptr};
    CheckboxWidget*   mySwapPaddles{nullptr};
    StaticTextWidget* myEraseEEPROMLabel{nullptr};
    ButtonWidget*     myEraseEEPROMButton{nullptr};
    StaticTextWidget* myEraseEEPROMInfo{nullptr};
    CheckboxWidget*   myMouseControl{nullptr};
    PopUpWidget*      myMouseX{nullptr};
    PopUpWidget*      myMouseY{nullptr};
    SliderWidget*     myMouseRange{nullptr};

    // Cartridge properties
    EditTextWidget*   myName{nullptr};
    EditTextWidget*   myMD5{nullptr};
    EditTextWidget*   myManufacturer{nullptr};
    EditTextWidget*   myModelNo{nullptr};
    EditTextWidget*   myRarity{nullptr};
    EditTextWidget*   myNote{nullptr};

    // High Scores properties
    CheckboxWidget*   myHighScores{nullptr};
    StaticTextWidget* myPlayersLabel{ nullptr };
    PopUpWidget*      myPlayers{nullptr};
    StaticTextWidget* myPlayersAddressLabel{ nullptr };
    EditTextWidget*   myPlayersAddress{ nullptr };
    EditTextWidget*   myPlayersAddressVal{ nullptr };

    StaticTextWidget* myVariationsLabel{ nullptr };
    EditTextWidget*   myVariations{nullptr};
    StaticTextWidget* myVarAddressLabel{ nullptr };
    EditTextWidget*   myVarAddress{ nullptr };
    EditTextWidget*   myVarAddressVal{ nullptr };
    CheckboxWidget*   myVarBCD{ nullptr };
    CheckboxWidget*   myVarZeroBased{ nullptr };

    StaticTextWidget* myScoresLabel{ nullptr };
    StaticTextWidget* myScoreDigitsLabel{ nullptr };
    PopUpWidget*      myScoreDigits{nullptr};
    StaticTextWidget* myTrailingZeroesLabel{ nullptr };
    PopUpWidget*      myTrailingZeroes{nullptr};
    CheckboxWidget*   myScoreBCD{nullptr};

    StaticTextWidget* myScoreAddressesLabel[HighScoresManager::MAX_PLAYERS]{ nullptr };
    EditTextWidget*   myScoreAddress[HighScoresManager::MAX_PLAYERS][HighScoresManager::MAX_SCORE_ADDR]{nullptr};
    EditTextWidget*   myScoreAddressVal[HighScoresManager::MAX_PLAYERS][HighScoresManager::MAX_SCORE_ADDR]{nullptr};
    StaticTextWidget* myCurrentScoreLabel;
    StaticTextWidget* myCurrentScore[HighScoresManager::MAX_PLAYERS];

    enum {
      kVCenterChanged      = 'Vcch',
      kPhosphorChanged     = 'PPch',
      kPPBlendChanged      = 'PBch',
      kLeftCChanged        = 'LCch',
      kRightCChanged       = 'RCch',
      kMCtrlChanged        = 'MCch',
      kEEButtonPressed     = 'EEgb',
      kHiScoresChanged     = 'HSch',
      kPlayersChanged      = 'Plch',
      kVarZeroBasedChanged = 'VZch',
      kVarBcdChanged       = 'VBch',
      kScoreDigitsChanged  = 'SDch',
      kScoreZeroesChanged  = 'SZch',
      kScoreBcdChanged     = 'SBch',
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
