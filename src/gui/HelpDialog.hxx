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

#ifndef HELP_DIALOG_HXX
#define HELP_DIALOG_HXX

class DialogContainer;
class CommandSender;
class ButtonWidget;
class LabelWidget;
class OSystem;

#include "Dialog.hxx"
#include "bspf.hxx"

/**
  Shows the paged list of keyboard/controller shortcuts and their
  current mappings.

  @author  Stephen Anthony and Thomas Jentzsch
*/
class HelpDialog : public Dialog
{
  public:
    HelpDialog(OSystem& osystem, DialogContainer& parent, const GUI::Font& font);
    ~HelpDialog() override = default;

    void loadConfig() override { displayInfo(); }

  protected:
    void layout() override;
    // Pages via Prev/Next, opens the update-check URL, or opens a clicked link
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:
    // Fills myKeyStr/myDescStr (and 'title') with the given page's rows, pairing
    // each event's current key/button mapping with a description
    void updateStrings(int page, int lines, string& title);
    // Applies updateStrings()'s rows to myTitle/myKey/myDesc, auto-linking the
    // one row that points at the Input remapping dialog
    void displayInfo();

  private:
    static constexpr uInt32 LINES_PER_PAGE = 10;
    ButtonWidget* myNextButton{nullptr};
    ButtonWidget* myPrevButton{nullptr};
    ButtonWidget* myUpdateButton{nullptr};

    LabelWidget* myTitle;
    // Hotkey / description LabelWidgets, one row each, reused across pages
    std::array<LabelWidget*, LINES_PER_PAGE> myKey{nullptr};
    std::array<LabelWidget*, LINES_PER_PAGE> myDesc{nullptr};
    // This page's raw text (see updateStrings())
    std::array<string, LINES_PER_PAGE> myKeyStr;
    std::array<string, LINES_PER_PAGE> myDescStr;

    int myPage{1};
    static constexpr int myNumPages{5};

    // Sent by the "Check for Update" button
    struct Cmd {
      static constexpr GuiCmd::Code
        Update = GuiCmd::of("HelpDialog.Update");
    };

  private:
    // Following constructors and assignment operators not supported
    HelpDialog() = delete;
    HelpDialog(const HelpDialog&) = delete;
    HelpDialog(HelpDialog&&) = delete;
    HelpDialog& operator=(const HelpDialog&) = delete;
    HelpDialog& operator=(HelpDialog&&) = delete;
};

#endif  // HELP_DIALOG_HXX
