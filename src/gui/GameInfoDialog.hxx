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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
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
class LabelWidget;
class RadioButtonGroup;
class RadioButtonWidget;
class TabWidget;
class SliderWidget;
class QuadTariDialog;

#include "Dialog.hxx"
#include "Command.hxx"
#include "Props.hxx"
#include "HighScoresManager.hxx"

class GameInfoDialog : public Dialog, public CommandSender
{
  public:
    GameInfoDialog(OSystem& osystem, DialogContainer& parent,
                   const GUI::Font& font, GuiObject* boss);
    ~GameInfoDialog() override;

    void loadConfig() override;
    void saveConfig() override;
    void setDefaults() override;

    // The QuadTari dialog is a separate Dialog; only allocated on first use,
    // so it must forward explicitly rather than assume it will always be
    // freshly reconstructed before it goes stale
    void refreshFont() override;

  protected:
    void layout() override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    void addEmulationTab();
    void addConsoleTab();
    void addControllersTab();
    void addCartridgeTab();
    void addHighScoresTab();

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
    // save properties from all tabs into the local properties object
    void saveProperties();

    // en/disable tabs and widgets depending on multicart bankswitch type selected
    void updateMultiCart();
    // update 'BS Type' list
    void updateBSTypes();
    // update 'Controller' tab widgets
    void updateControllerStates();
    // erase SaveKey/AtariVox pages for current game
    void eraseEEPROM();
    // update link button
    void updateLink();
    // update 'High Scores' tab widgets
    void updateHighScoresWidgets();
    // set formatted memory value for given address field
    void setAddressVal(const EditTextWidget* address, EditTextWidget* val,
                       bool isBCD = true, bool zeroBased = false, uInt8 maxVal = 255);
    void exportCurrentPropertiesToDisk(const FSNode& node);

  private:
    TabWidget* myTab{nullptr};

    // Emulation properties
    LabelWidget*    myBSTypeLbl{nullptr};
    PopUpWidget*    myBSType{nullptr};
    CheckboxWidget* myBSFilter{nullptr};
    LabelWidget*    myTypeDetected{nullptr};
    LabelWidget*    myStartBankLbl{nullptr};
    PopUpWidget*    myStartBank{nullptr};
    LabelWidget*    myFormatLbl{nullptr};
    PopUpWidget*    myFormat{nullptr};
    LabelWidget*    myFormatDetected{nullptr};
    LabelWidget*    myVCenterLbl{nullptr};
    SliderWidget*   myVCenter{nullptr};
    CheckboxWidget* myPhosphor{nullptr};
    LabelWidget*    myPPBlendLbl{nullptr};
    SliderWidget*   myPPBlend{nullptr};
    CheckboxWidget* mySound{nullptr};
    LabelWidget*    myEmulInfo{nullptr};

    // Console properties
    unique_ptr<RadioButtonGroup> myLeftDiffGroup;
    unique_ptr<RadioButtonGroup> myRightDiffGroup;
    unique_ptr<RadioButtonGroup> myTVTypeGroup;
    LabelWidget* myTVTypeLbl{nullptr};
    LabelWidget* myLeftDiffLbl{nullptr};
    LabelWidget* myRightDiffLbl{nullptr};
    std::array<RadioButtonWidget*, 2> myTVType{nullptr};
    std::array<RadioButtonWidget*, 2> myLeftDiff{nullptr};
    std::array<RadioButtonWidget*, 2> myRightDiff{nullptr};

    // Controller properties
    LabelWidget*    myLeftPortLbl{nullptr};
    LabelWidget*    myRightPortLbl{nullptr};
    PopUpWidget*    myLeftPort{nullptr};
    LabelWidget*    myLeftPortDetected{nullptr};
    PopUpWidget*    myRightPort{nullptr};
    LabelWidget*    myRightPortDetected{nullptr};
    ButtonWidget*   myQuadTariButton{nullptr};
    CheckboxWidget* mySwapPorts{nullptr};
    CheckboxWidget* mySwapPaddles{nullptr};
    LabelWidget*    myEraseEEPROMLbl{nullptr};
    ButtonWidget*   myEraseEEPROMButton{nullptr};
    LabelWidget*    myEraseEEPROMInfo{nullptr};
    LabelWidget*    myPaddlesCenter{nullptr};
    LabelWidget*    myPaddleXCenterLbl{nullptr};
    SliderWidget*   myPaddleXCenter{nullptr};
    LabelWidget*    myPaddleYCenterLbl{nullptr};
    SliderWidget*   myPaddleYCenter{nullptr};
    CheckboxWidget* myMouseControl{nullptr};
    LabelWidget*    myMouseXLbl{nullptr};
    PopUpWidget*    myMouseX{nullptr};
    LabelWidget*    myMouseYLbl{nullptr};
    PopUpWidget*    myMouseY{nullptr};
    LabelWidget*    myMouseRangeLbl{nullptr};
    SliderWidget*   myMouseRange{nullptr};

    // Allow assigning the four QuadTari controllers
    unique_ptr<QuadTariDialog> myQuadTariDialog;

    // Cartridge properties
    // Row labels: Name/MD5/Manufacturer/Model/Rarity/Note/Link/Bezelname
    std::array<LabelWidget*, 8> myCartLabels{nullptr};
    EditTextWidget* myName{nullptr};
    EditTextWidget* myMD5{nullptr};
    EditTextWidget* myManufacturer{nullptr};
    EditTextWidget* myModelNo{nullptr};
    EditTextWidget* myRarity{nullptr};
    EditTextWidget* myNote{nullptr};
    EditTextWidget* myUrl{nullptr};
    ButtonWidget*   myUrlButton{nullptr};
    EditTextWidget* myBezelName{nullptr};
    ButtonWidget*   myBezelButton{nullptr};
    LabelWidget*    myBezelDetected{nullptr};

    // High Scores properties
    CheckboxWidget* myHighScores{nullptr};

    LabelWidget*    myVariationsLbl{nullptr};
    EditTextWidget* myVariations{nullptr};
    LabelWidget*    myVarAddressLbl{nullptr};
    EditTextWidget* myVarAddress{nullptr};
    EditTextWidget* myVarAddressVal{nullptr};
    CheckboxWidget* myVarsBCD{nullptr};
    CheckboxWidget* myVarsZeroBased{nullptr};

    LabelWidget*    myScoreLbl{nullptr};
    LabelWidget*    myScoreDigitsLbl{nullptr};
    PopUpWidget*    myScoreDigits{nullptr};
    LabelWidget*    myTrailingZeroesLbl{nullptr};
    PopUpWidget*    myTrailingZeroes{nullptr};
    CheckboxWidget* myScoreBCD{nullptr};
    CheckboxWidget* myScoreInvert{nullptr};

    LabelWidget*    myScoreAddressesLbl{nullptr};
    EditTextWidget* myScoreAddress[HSM::MAX_SCORE_ADDR]{nullptr};
    EditTextWidget* myScoreAddressVal[HSM::MAX_SCORE_ADDR]{nullptr};
    LabelWidget*    myCurrentScoreLbl{nullptr};
    LabelWidget*    myCurrentScore{nullptr};

    LabelWidget*    mySpecialLbl{nullptr};
    EditTextWidget* mySpecialName{nullptr};
    LabelWidget*    mySpecialAddressLbl{nullptr};
    EditTextWidget* mySpecialAddress{nullptr};
    EditTextWidget* mySpecialAddressVal{nullptr};
    CheckboxWidget* mySpecialBCD{nullptr};
    CheckboxWidget* mySpecialZeroBased{nullptr};

    LabelWidget*    myHighScoreNotesLbl{nullptr};
    EditTextWidget* myHighScoreNotes{nullptr};

    enum {
      kBSTypeChanged    = 'Btch',
      kBSFilterChanged  = 'Bfch',
      kVCenterChanged   = 'Vcch',
      kPhosphorChanged  = 'PPch',
      kPPBlendChanged   = 'PBch',
      kLeftCChanged     = 'LCch',
      kRightCChanged    = 'RCch',
      kQuadTariPressed  = 'QTpr',
      kMCtrlChanged     = 'MCch',
      kEEButtonPressed  = 'EEgb',
      kHiScoresChanged  = 'HSch',
      kPXCenterChanged  = 'Pxch',
      kPYCenterChanged  = 'Pych',
      kExportPressed    = 'Expr',
      kLinkPressed      = 'Lkpr',
      kBezelFilePressed = 'BFpr'
    };

    enum: uInt8 { kLinkId };

    // Game properties for currently loaded ROM
    Properties myGameProperties;
    // Filename of the currently loaded ROM
    FSNode myGameFile;

  private:
    // Following constructors and assignment operators not supported
    GameInfoDialog() = delete;
    GameInfoDialog(const GameInfoDialog&) = delete;
    GameInfoDialog(GameInfoDialog&&) = delete;
    GameInfoDialog& operator=(const GameInfoDialog&) = delete;
    GameInfoDialog& operator=(GameInfoDialog&&) = delete;
};

#endif  // GAME_INFO_DIALOG_HXX
