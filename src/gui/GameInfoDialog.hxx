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

/**
  Dialog for editing a ROM's properties (bankswitch type, TV format,
  controllers, cartridge metadata, high-score definitions) across
  five tabs.

  @author  Stephen Anthony and Thomas Jentzsch
*/
class GameInfoDialog : public Dialog, public CommandSender
{
  public:
    GameInfoDialog(OSystem& osystem, DialogContainer& parent,
                   const GUI::Font& font, GuiObject* boss);
    // Out-of-line: myQuadTariDialog, myLeftDiffGroup, myRightDiffGroup and
    // myTVTypeGroup (unique_ptr<QuadTariDialog>/<RadioButtonGroup>) need
    // their complete types here
    ~GameInfoDialog() override;

    // Populates every tab from the current game properties, then sets the
    // dialog's title
    void loadConfig() override;
    // Writes every tab back into myGameProperties and the properties set,
    // applying changes to a running console immediately
    void saveConfig() override;
    // Resets the currently active tab to this ROM's default properties
    void setDefaults() override;

  protected:
    void layout() override;
    // OK saves and exits; Defaults resets the active tab to this ROM's
    // defaults; Export writes properties to disk; the remaining ids refresh
    // detected-controller/link/high-scores state or open the QuadTari/bezel
    // browsers
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:
    // Builds the 'Emulation' tab: bankswitch type, TV format, phosphor/
    // blend, V-Center, sound
    void addEmulationTab();
    // Builds the 'Console' tab: TV type and left/right difficulty switches
    void addConsoleTab();
    // Builds the 'Controllers' tab: port selection, paddle/mouse settings,
    // EEPROM erase
    void addControllersTab();
    // Builds the 'Cartridge' tab: name/MD5/manufacturer/model/rarity/note/
    // link/bezel fields
    void addCartridgeTab();
    // Builds the 'High Scores' tab: variation/score/special-value
    // definitions and their live readouts
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
    // save the properties of the 'High Scores' tab
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
    // Writes the current properties (from all tabs) to a stand-alone .pro
    // file at 'node'
    void exportCurrentPropertiesToDisk(const FSNode& node);

  private:
    // Hosts the five property tabs (Emulation, Console, Controllers,
    // Cartridge, High Scores)
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

    // Number of game variations, and where that count is read from memory
    LabelWidget*    myVariationsLbl{nullptr};
    EditTextWidget* myVariations{nullptr};
    LabelWidget*    myVarAddressLbl{nullptr};
    EditTextWidget* myVarAddress{nullptr};
    EditTextWidget* myVarAddressVal{nullptr};
    CheckboxWidget* myVarsBCD{nullptr};
    CheckboxWidget* myVarsZeroBased{nullptr};

    // Score format: digit count, trailing zero count, BCD encoding, and
    // whether a lower score is better
    LabelWidget*    myScoreLbl{nullptr};
    LabelWidget*    myScoreDigitsLbl{nullptr};
    PopUpWidget*    myScoreDigits{nullptr};
    LabelWidget*    myTrailingZeroesLbl{nullptr};
    PopUpWidget*    myTrailingZeroes{nullptr};
    CheckboxWidget* myScoreBCD{nullptr};
    CheckboxWidget* myScoreInvert{nullptr};

    // Memory addresses the score is read from, and the score they
    // currently resolve to
    LabelWidget*    myScoreAddressesLbl{nullptr};
    EditTextWidget* myScoreAddress[HSM::MAX_SCORE_ADDR]{nullptr};
    EditTextWidget* myScoreAddressVal[HSM::MAX_SCORE_ADDR]{nullptr};
    LabelWidget*    myCurrentScoreLbl{nullptr};
    LabelWidget*    myCurrentScore{nullptr};

    // The optional "special" value (the game's own word, e.g. Level/Wave/
    // Round) and where it's read from memory
    LabelWidget*    mySpecialLbl{nullptr};
    EditTextWidget* mySpecialName{nullptr};
    LabelWidget*    mySpecialAddressLbl{nullptr};
    EditTextWidget* mySpecialAddress{nullptr};
    EditTextWidget* mySpecialAddressVal{nullptr};
    CheckboxWidget* mySpecialBCD{nullptr};
    CheckboxWidget* mySpecialZeroBased{nullptr};

    // Free-text notes about this game's high-score properties
    LabelWidget*    myHighScoreNotesLbl{nullptr};
    EditTextWidget* myHighScoreNotes{nullptr};

    // Command ids dispatched in handleCommand()
    struct Cmd {
      static constexpr GuiCmd::Code
        BankswitchTypeChanged   = GuiCmd::of("GameInfoDialog.BankswitchTypeChanged"),
        BankswitchFilterChanged = GuiCmd::of("GameInfoDialog.BankswitchFilterChanged"),
        VCenterChanged          = GuiCmd::of("GameInfoDialog.VCenterChanged"),
        PhosphorChanged         = GuiCmd::of("GameInfoDialog.PhosphorChanged"),
        PhosphorBlendChanged    = GuiCmd::of("GameInfoDialog.PhosphorBlendChanged"),
        LeftControllerChanged   = GuiCmd::of("GameInfoDialog.LeftControllerChanged"),
        RightControllerChanged  = GuiCmd::of("GameInfoDialog.RightControllerChanged"),
        QuadTariPressed         = GuiCmd::of("GameInfoDialog.QuadTariPressed"),
        MouseControlChanged     = GuiCmd::of("GameInfoDialog.MouseControlChanged"),
        EraseEeprom             = GuiCmd::of("GameInfoDialog.EraseEeprom"),
        HighScoresChanged       = GuiCmd::of("GameInfoDialog.HighScoresChanged"),
        PaddleXCenterChanged    = GuiCmd::of("GameInfoDialog.PaddleXCenterChanged"),
        PaddleYCenterChanged    = GuiCmd::of("GameInfoDialog.PaddleYCenterChanged"),
        Export                  = GuiCmd::of("GameInfoDialog.Export"),
        Link                    = GuiCmd::of("GameInfoDialog.Link"),
        BezelFile               = GuiCmd::of("GameInfoDialog.BezelFile");
    };

    // Widget id for myUrl, checked in handleCommand() to route its
    // EditableWidget::Cmd::Changed to updateLink()
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
