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

#include "Dialog.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "OSystem.hxx"
#include "EventHandler.hxx"
#include "StringParser.hxx"
#include "Layout.hxx"
#include "MessageBox.hxx"

namespace GUI {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MessageBox::MessageBox(GuiObject* boss, const GUI::Font& font,
                       const StringList& text, int max_w, int max_h, int okCmd,
                       int cancelCmd, string_view okText, string_view cancelText,
                       string_view title, bool focusOKButton)
  : Dialog(boss->instance(), boss->parent(), font, title, max_w, max_h),
    CommandSender(boss),
    myOkCmd{okCmd},
    myCancelCmd{cancelCmd},
    myMaxW{max_w},
    myMaxH{max_h}
{
  createText(font, text);

  WidgetArray wid;
  addOKCancelBGroup(wid, font, okText, cancelText, focusOKButton);
  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MessageBox::MessageBox(GuiObject* boss, const GUI::Font& font,
                       const StringList& text, int max_w, int max_h, int okCmd,
                       string_view okText, string_view cancelText,
                       string_view title, bool focusOKButton)
  : MessageBox(boss, font, text, max_w, max_h,
               okCmd, 0, okText, cancelText, title, focusOKButton)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MessageBox::MessageBox(GuiObject* boss, const GUI::Font& font,
                       string_view text, int max_w, int max_h, int okCmd,
                       string_view okText, string_view cancelText,
                       string_view title, bool focusOKButton)
  : MessageBox(boss, font, StringParser(text).stringList(), max_w, max_h,
               okCmd, okText, cancelText, title, focusOKButton)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MessageBox::MessageBox(GuiObject* boss, const GUI::Font& font,
                       string_view text, int max_w, int max_h, int okCmd,
                       int cancelCmd, string_view okText, string_view cancelText,
                       string_view title, bool focusOKButton)
  : MessageBox(boss, font, StringParser(text).stringList(), max_w, max_h,
               okCmd, cancelCmd, okText, cancelText, title, focusOKButton)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MessageBox::MessageBox(OSystem& osystem, DialogContainer& parent,
                       const GUI::Font& font, const StringList& text,
                       int max_w, int max_h,
                       const std::function<void(bool ok)>& callback,
                       string_view okText, string_view cancelText,
                       string_view title, bool focusOKButton)
  : Dialog(osystem, parent, font, title, max_w, max_h),
    myMaxW{max_w},
    myMaxH{max_h},
    myCallback{callback}
{
  createText(font, text);

  WidgetArray wid;
  addOKCancelBGroup(wid, font, okText, cancelText, focusOKButton);
  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MessageBox::createText(const GUI::Font& font, const StringList& text)
{

  myText = text;
  for(const auto& s: text)
    myTextWidgets.push_back(new StaticTextWidget(this, font, s,
                                                 TextAlign::Left));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MessageBox::layout()
{
  using GUI::BoxLayout;
  using GUI::stretchedItem;
  using Dir = BoxLayout::Dir;

  const int fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();

  // Vertical stack of the text lines below the title bar; the button group sits
  // below it, positioned by layoutButtonGroup()
  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  for(auto* w: myTextWidgets)
    root->addAuto(stretchedItem(w));
  root->addSpace(VGAP * 2);

  // As wide as the longest line, and as tall as the lines need -- but never
  // narrower than the button group below them, and never bigger than the caller
  // said we may be
  int str_w = 0;
  for(const auto& s: myText)
    str_w = std::max(static_cast<int>(s.length()), str_w);

  _w = std::min(std::max(str_w * fontWidth + HBORDER * 2,
                         Dialog::buttonGroupWidth()), myMaxW);
  _h = std::min(_th + static_cast<int>(root->naturalSize().h)
                + buttonHeight + VBORDER, myMaxH);

  root->doLayout(0, _th, _w, _h - _th);

  // Standard OK/Cancel button group along the bottom edge
  layoutButtonGroup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MessageBox::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  if(cmd == GuiObject::kOKCmd || cmd == GuiObject::kCloseCmd)
  {
    const bool ok = cmd == GuiObject::kOKCmd;

    if(myCallback)
    {
      // As a transient dialog over TIA mode, any answer leaves the menu mode;
      // the result is reported through the callback
      instance().eventHandler().leaveMenuMode();
      myCallback(ok);
    }
    else
    {
      close();

      // Send a signal to the calling class which button was selected
      // Since we aren't derived from a widget, we don't have a 'data' or 'id'
      const int replyCmd = ok ? myOkCmd : myCancelCmd;
      if(replyCmd)
        sendCommand(replyCmd, 0, 0);
    }
  }
  else
    Dialog::handleCommand(sender, cmd, data, id);
}

} // namespace GUI
