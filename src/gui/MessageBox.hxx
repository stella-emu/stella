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

#ifndef MESSAGE_BOX_HXX
#define MESSAGE_BOX_HXX

class GuiObject;
class StaticTextWidget;

#include <functional>

#include "Dialog.hxx"

namespace GUI {

/**
 * Show a simple message box containing the given text, with buttons
 * prompting the user to accept or reject. The answer is reported through
 * a callback rather than a command id, so a caller needs no member to
 * hold the box, no command enum, and no handleCommand() case of its own.
 *
 * The c'tor is private: there is no point calling it directly, since the
 * result can't be gotten back that way. Use confirm() (from a dialog) or
 * create() (the transient TIA-overlay case, see EventHandler) instead —
 * both funnel into the one c'tor; they differ only in how they get an
 * (OSystem&, DialogContainer&) and in what happens to the box afterwards.
 */
class MessageBox : public Dialog
{
  public:
    ~MessageBox() override = default;

    /**
      Ask a simple OK/Cancel question, sized to its own content. Owns its
      own lifetime (one shared instance, recreated as needed — see
      BrowserDialog::show() for the same idea), so the caller keeps
      nothing: no member, no command id, no handleCommand() case. The
      answer comes back through 'callback' instead.

      @param boss          The dialog asking the question
      @param text          The message; split into lines on '\n'
      @param callback      Run with 'true' if OK was clicked, 'false' for Cancel
      @param title         The dialog's title bar text
      @param okText        Text of the OK button
      @param cancelText    Text of the Cancel button
      @param focusOKButton Whether OK (true) or Cancel (false) is focused initially
    */
    static void confirm(GuiObject* boss, string_view text,
                         const std::function<void(bool ok)>& callback,
                         string_view title = "", string_view okText = "OK",
                         string_view cancelText = "Cancel",
                         bool focusOKButton = false);

    /**
      Variant for transient use over TIA mode (via EventHandler::openDialog):
      there is no boss to send commands to — answering leaves the menu mode
      instead of closing normally. Returns ownership to the caller, which is
      expected to hand it straight to EventHandler::openDialog().
    */
    static unique_ptr<Dialog> create(OSystem& osystem, DialogContainer& parent,
               const GUI::Font& font, string_view text,
               const std::function<void(bool ok)>& callback,
               string_view okText = "OK", string_view cancelText = "Cancel",
               string_view title = "");

    /**
      Since confirm() allocates a static MessageBox, at some point we need
      to manually de-allocate it. This method must be called from one of
      the lowest-level destructors to do that. Currently this is called
      from the OSystem destructor (see BrowserDialog::hide()).
    */
    static void hide();

  protected:
    void layout() override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    // 'transient' is true only for the boss-less TIA-overlay case: there is
    // no dialog to return to, so answering must leave menu mode instead of
    // just closing this box.
    MessageBox(OSystem& osystem, DialogContainer& parent, const GUI::Font& font,
               string_view text,
               const std::function<void(bool ok)>& callback,
               string_view okText, string_view cancelText,
               string_view title, bool focusOKButton, bool transient);

    void createText(const GUI::Font& font, string_view text);

  private:
    StringList myText;
    std::vector<StaticTextWidget*> myTextWidgets;
    std::function<void(bool ok)> myCallback;
    bool myTransient{false};

    static unique_ptr<MessageBox> ourBox;

  private:
    // Following constructors and assignment operators not supported
    MessageBox() = delete;
    MessageBox(const MessageBox&) = delete;
    MessageBox(MessageBox&&) = delete;
    MessageBox& operator=(const MessageBox&) = delete;
    MessageBox& operator=(MessageBox&&) = delete;
};

}  // namespace GUI

#endif  // MESSAGE_BOX_HXX
