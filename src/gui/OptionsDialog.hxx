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

#ifndef OPTIONS_DIALOG_HXX
#define OPTIONS_DIALOG_HXX

class CommandSender;
class DialogContainer;
class GuiObject;
class OSystem;

#include "Dialog.hxx"

class OptionsDialog : public Dialog
{
  public:
    // Builds the two columns of category buttons plus the Close button; some
    // are disabled/enabled depending on 'mode' (launcher vs. in-game)
    OptionsDialog(OSystem& osystem, DialogContainer& parent, GuiObject* boss,
                  AppMode mode);
    // Out-of-line: myDialog (unique_ptr<Dialog>) needs its complete type here
    ~OptionsDialog() override;

    // Enables/disables the Game Properties button per the current ROM/state
    void loadConfig() override;

  protected:
    // Opens the sub-dialog for the clicked category button; Close exits
    // menu mode (emulator) or closes this dialog (launcher)
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

    void layout() override;

  private:
    // The currently open category sub-dialog, if any
    unique_ptr<Dialog>           myDialog;

    // All buttons in grid order: the two columns top-to-bottom, then Close
    vector<ButtonWidget*> myButtons;

    // Kept to enable/disable per-mode (see the ctor/loadConfig())
    ButtonWidget* myRomAuditButton{nullptr};
    ButtonWidget* myGameInfoButton{nullptr};
    ButtonWidget* myCheatCodeButton{nullptr};

    // The dialog that opened us, passed through to sub-dialogs that need it
    // (e.g. Game Properties)
    GuiObject* myBoss{nullptr};
    // Indicates if this dialog is used for global (vs. in-game) settings
    AppMode myMode{AppMode::emulator};

    // Command ids for the category buttons, dispatched in handleCommand()
    enum {
      kVidCmd      = 'VIDO',
      kEmuCmd      = 'EMUO',
      kInptCmd     = 'INPT',
      kUsrIfaceCmd = 'URIF',
      kSnapCmd     = 'SNAP',
      kAuditCmd    = 'RAUD',
      kInfoCmd     = 'INFO',
      kCheatCmd    = 'CHET',
      kLoggerCmd   = 'LOGG',
      kDevelopCmd  = 'DEVL',
      kHelpCmd     = 'HELP',
      kAboutCmd    = 'ABOU',
      kExitCmd     = 'EXIM'
    };

  private:
    // Following constructors and assignment operators not supported
    OptionsDialog() = delete;
    OptionsDialog(const OptionsDialog&) = delete;
    OptionsDialog(OptionsDialog&&) = delete;
    OptionsDialog& operator=(const OptionsDialog&) = delete;
    OptionsDialog& operator=(OptionsDialog&&) = delete;
};

#endif  // OPTIONS_DIALOG_HXX
