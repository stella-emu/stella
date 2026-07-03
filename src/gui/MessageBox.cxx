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
#include "StringParser.hxx"
#include "Layout.hxx"
#include "MessageBox.hxx"

namespace GUI {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MessageBox::MessageBox(GuiObject* boss, const GUI::Font& font,
                       const StringList& text, int max_w, int max_h, int okCmd,
                       int cancelCmd, string_view okText, string_view cancelText,
                       string_view title, bool focusOKButton)
  : Dialog(boss->instance(), boss->parent(), font, title, 0, 0, max_w, max_h),
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
void MessageBox::createText(const GUI::Font& font, const StringList& text)
{
  const int fontHeight = Dialog::fontHeight();

  myText = text;
  for(const auto& s: text)
    myTextWidgets.push_back(new StaticTextWidget(this, font, 0, 0, 1,
                            fontHeight, s, TextAlign::Left));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MessageBox::layout()
{
  const int fontWidth  = Dialog::fontWidth(),
            fontHeight = Dialog::fontHeight(),
            VBORDER    = Dialog::vBorder(),
            HBORDER    = Dialog::hBorder();

  // Size to the longest line, clamped to the max, but never narrower than the
  // button group (mirrors addOKCancelBGroup's width floor)
  int str_w = 0;
  for(const auto& s: myText)
    str_w = std::max(static_cast<int>(s.length()), str_w);
  _w = std::min(str_w * fontWidth + HBORDER * 2, myMaxW);
  _w = std::max(_w, HBORDER * 2 + _okWidget->getWidth()
                    + _cancelWidget->getWidth() + Dialog::buttonGap());
  _h = std::min((static_cast<int>(myText.size()) + 2) * fontHeight + VBORDER * 2 + _th,
                myMaxH);

  // Stack the text lines below the title bar
  int ypos = VBORDER + _th;
  for(auto* w: myTextWidgets)
  {
    w->setPos(HBORDER, ypos);
    w->setWidth(_w - HBORDER * 2);
    ypos += fontHeight;
  }

  // Standard OK/Cancel button group along the bottom edge
  layoutButtonGroup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MessageBox::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  if(cmd == GuiObject::kOKCmd)
  {
    close();

    // Send a signal to the calling class that 'OK' has been selected
    // Since we aren't derived from a widget, we don't have a 'data' or 'id'
    if(myOkCmd)
      sendCommand(myOkCmd, 0, 0);
  }
  else if(cmd == GuiObject::kCloseCmd)
  {
    close();

    // Send a signal to the calling class that 'Cancel' has been selected
    if(myCancelCmd)
      sendCommand(myCancelCmd, 0, 0);
  }
  else
    Dialog::handleCommand(sender, cmd, data, id);
}

} // namespace GUI
