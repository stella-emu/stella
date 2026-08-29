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

#ifndef CHEAT_CODE_DIALOG_HXX
#define CHEAT_CODE_DIALOG_HXX

class DialogContainer;
class CommandSender;
class Widget;
class ButtonWidget;
class LabelWidget;
class CheckListWidget;
class EditTextWidget;
class OptionsDialog;
class InputTextDialog;
class OSystem;

#include "Dialog.hxx"

/**
  Dialog for viewing, adding, editing, and removing cheats, backed
  by CheatManager.

  @author  Stephen Anthony
*/
class CheatCodeDialog : public Dialog
{
  public:
    // Builds all child widgets at placeholder geometry; layout() sizes them
    CheatCodeDialog(OSystem& osystem, DialogContainer& parent,
                    const GUI::Font& font);
    // Out-of-line: myCheatInput needs InputTextDialog's complete type here
    ~CheatCodeDialog() override;

    // Populates the list from CheatManager and enables/disables buttons
    void loadConfig() override;
    // Applies checkbox states back to the CheatManager
    void saveConfig() override;

    // The cheat input box is a separate Dialog; forward explicitly rather
    // than assume it will always be freshly reconstructed before it goes stale
    void refreshFont() override;

  protected:
    // Positions the list, action-button column, and OK/Cancel row
    void layout() override;
    // Dispatches button/menu commands to the add/edit/remove helpers
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:
    // Opens the input dialog to create a new cheat
    void addCheat();
    // Opens the input dialog pre-filled with the selected cheat
    void editCheat();
    // Deletes the selected cheat and refreshes the list
    void removeCheat();
    // Opens the input dialog to create a one-shot cheat
    void addOneShotCheat();

  private:
    // Checklist of cheat names with enable/disable checkboxes
    CheckListWidget* myCheatList{nullptr};
    // Popup dialog for entering/editing a cheat's name and code
    unique_ptr<InputTextDialog> myCheatInput;

    // Opens the input dialog to add a cheat
    ButtonWidget* myAddButton{nullptr};
    // Opens the input dialog to edit the selected cheat
    ButtonWidget* myEditButton{nullptr};
    // Removes the selected cheat
    ButtonWidget* myRemoveButton{nullptr};
    // Opens the input dialog to add a one-shot cheat
    ButtonWidget* myOneShotButton{nullptr};

    // Command IDs used with handleCommand()
    struct Cmd {
      static constexpr GuiCmd::Code
        AddCheat          = GuiCmd::of("CheatCodeDialog.AddCheat"),
        EditCheat         = GuiCmd::of("CheatCodeDialog.EditCheat"),
        AddOneShot        = GuiCmd::of("CheatCodeDialog.AddOneShot"),
        CheatAdded        = GuiCmd::of("CheatCodeDialog.CheatAdded"),
        CheatEdited       = GuiCmd::of("CheatCodeDialog.CheatEdited"),
        OneShotCheatAdded = GuiCmd::of("CheatCodeDialog.OneShotCheatAdded"),
        RemoveCheat       = GuiCmd::of("CheatCodeDialog.RemoveCheat");
    };

  private:
    // Following constructors and assignment operators not supported
    CheatCodeDialog() = delete;
    CheatCodeDialog(const CheatCodeDialog&) = delete;
    CheatCodeDialog(CheatCodeDialog&&) = delete;
    CheatCodeDialog& operator=(const CheatCodeDialog&) = delete;
    CheatCodeDialog& operator=(CheatCodeDialog&&) = delete;
};

#endif  // CHEAT_CODE_DIALOG_HXX
