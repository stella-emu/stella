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

#ifndef ABOUT_DIALOG_HXX
#define ABOUT_DIALOG_HXX

class OSystem;
class DialogContainer;
class CommandSender;
class ButtonWidget;
class LabelWidget;
class WhatsNewDialog;

#include "Dialog.hxx"

/**
  Shows paged information about Stella (credits, description, links),
  and opens the What's New dialog on request.

  @author  Stephen Anthony and Thomas Jentzsch
*/
class AboutDialog : public Dialog
{
  public:
    AboutDialog(OSystem& osystem, DialogContainer& parent,
                const GUI::Font& font);
    // Out-of-line: myWhatsNewDialog (unique_ptr<WhatsNewDialog>) needs its
    // complete type here
    ~AboutDialog() override;

    void loadConfig() override { displayInfo(); }

  protected:
    void layout() override;
    // Pages via Prev/Next, opens the What's New dialog, or opens a clicked link
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:
    // Fills myDescStr (and 'title') with the given page's text; each line may
    // start with \C/\L/\R (alignment) and/or \c0-\c5 (color) prefixes, stripped
    // and applied by displayInfo()
    void updateStrings(int page, int lines, string& title);
    // Applies updateStrings()'s formatted lines to myTitle/myDesc, including
    // auto-linking a few recognized names/phrases to their URLs
    void displayInfo();

  private:
    ButtonWidget* myWhatsNewButton{nullptr};
    ButtonWidget* myNextButton{nullptr};
    ButtonWidget* myPrevButton{nullptr};

    LabelWidget* myTitle{nullptr};
    // One LabelWidget per line, reused across pages
    vector<LabelWidget*> myDesc;
    // This page's raw, format-prefixed text (see updateStrings())
    vector<string> myDescStr;

    int myPage{1};
    static constexpr int myNumPages{4};
    static constexpr int myLinesPerPage{13};

    // Created lazily on first "What's New" click
    unique_ptr<WhatsNewDialog> myWhatsNewDialog;

    struct Cmd {
      static constexpr GuiCmd::Code
        WhatsNew = GuiCmd::of("AboutDialog.WhatsNew");
    };

  private:
    // Following constructors and assignment operators not supported
    AboutDialog() = delete;
    AboutDialog(const AboutDialog&) = delete;
    AboutDialog(AboutDialog&&) = delete;
    AboutDialog& operator=(const AboutDialog&) = delete;
    AboutDialog& operator=(AboutDialog&&) = delete;
};

#endif  // ABOUT_DIALOG_HXX
