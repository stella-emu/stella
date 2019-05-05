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

#ifndef MIN_UI_HELP_DIALOG_HXX
#define MIN_UI_HELP_DIALOG_HXX

class DialogContainer;
class CommandSender;
class ButtonWidget;
class StaticTextWidget;
class OSystem;

#include "Dialog.hxx"
#include "bspf.hxx"

class R77HelpDialog : public Dialog
{
  public:
    R77HelpDialog(OSystem& osystem, DialogContainer& parent, const GUI::Font& font);
    virtual ~R77HelpDialog() = default;

  private:
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;
    void updateStrings(uInt8 page, uInt8 lines, string& title);
    void displayInfo();
    void loadConfig() override { displayInfo(); }

  private:
    static constexpr uInt32 LINES_PER_PAGE = 10;
    ButtonWidget* myNextButton;
    ButtonWidget* myPrevButton;

    StaticTextWidget* myTitle;
    StaticTextWidget* myJoy[LINES_PER_PAGE];
    StaticTextWidget* myBtn[LINES_PER_PAGE];
    StaticTextWidget* myDesc[LINES_PER_PAGE];
    string myJoyStr[LINES_PER_PAGE];
    string myBtnStr[LINES_PER_PAGE];
    string myDescStr[LINES_PER_PAGE];

    uInt8 myPage;
    uInt8 myNumPages;

  private:
    // Following constructors and assignment operators not supported
    R77HelpDialog() = delete;
    R77HelpDialog(const R77HelpDialog&) = delete;
    R77HelpDialog(R77HelpDialog&&) = delete;
    R77HelpDialog& operator=(const R77HelpDialog&) = delete;
    R77HelpDialog& operator=(R77HelpDialog&&) = delete;
};

#endif
