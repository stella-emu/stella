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

#ifndef ROM_AUDIT_DIALOG_HXX
#define ROM_AUDIT_DIALOG_HXX

class OSystem;
class GuiObject;
class DialogContainer;
class ButtonWidget;
class EditTextWidget;
class LabelWidget;

#include "Dialog.hxx"
#include "Command.hxx"
#include "FSNode.hxx"

/**
  Dialog for auditing a ROM directory: renames files matching a known
  MD5 to their properties name, reporting the results.

  @author  Stephen Anthony and Thomas Jentzsch
*/
class RomAuditDialog : public Dialog
{
  public:
    RomAuditDialog(OSystem& osystem, DialogContainer& parent, const GUI::Font& font);
    ~RomAuditDialog() override;

    // Populates the audit path from the launcher's current directory (or
    // 'romdir'), and clears the result fields
    void loadConfig() override;

  protected:
    // Audit ("OK") confirms, then runs auditRoms() and reloads the launcher;
    // the path button opens a directory browser
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;
    void layout() override;

  private:
    // Renames every ROM under the audit path that matches a known MD5 to its
    // properties name, showing progress and reporting renamed/skipped counts
    void auditRoms();

  private:
    struct Cmd {
      static constexpr GuiCmd::Code
        ChooseAuditDir = GuiCmd::of("RomAuditDialog.ChooseAuditDir");  // audit dir select
    };

    // ROM audit path
    ButtonWidget*   myRomButton{nullptr};
    EditTextWidget* myRomPath{nullptr};

    // Show the results of the ROM audit
    LabelWidget*    myRenamedLbl{nullptr};
    EditTextWidget* myResults1{nullptr};
    LabelWidget*    mySkippedLbl{nullptr};
    EditTextWidget* myResults2{nullptr};

    // Inline warning about the dangers of using this function
    LabelWidget* myWarningLbl{nullptr};

  private:
    // Following constructors and assignment operators not supported
    RomAuditDialog() = delete;
    RomAuditDialog(const RomAuditDialog&) = delete;
    RomAuditDialog(RomAuditDialog&&) = delete;
    RomAuditDialog& operator=(const RomAuditDialog&) = delete;
    RomAuditDialog& operator=(RomAuditDialog&&) = delete;
};

#endif  // ROM_AUDIT_DIALOG_HXX
