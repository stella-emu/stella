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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Dialog.hxx"
#include "OSystem.hxx"
#include "Version.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "StringParser.hxx"
#include "MessageBox.hxx"

namespace GUI {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MessageBox::MessageBox(GuiObject* boss, const GUI::Font& font,
                       const StringList& text, int max_w, int max_h, int cmd,
                       const string& okText, const string& cancelText,
                       const string& title,
                       bool focusOKButton)
  : Dialog(boss->instance(), boss->parent(), font, title, 0, 0, max_w, max_h),
    CommandSender(boss),
    myCmd(cmd)
{
  addText(font, text);

  WidgetArray wid;
  addOKCancelBGroup(wid, font, okText, cancelText, focusOKButton);
  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MessageBox::MessageBox(GuiObject* boss, const GUI::Font& font,
                       const string& text, int max_w, int max_h, int cmd,
                       const string& okText, const string& cancelText,
                       const string& title,
                       bool focusOKButton)
  : MessageBox(boss, font, StringParser(text).stringList(), max_w, max_h,
               cmd, okText, cancelText, title, focusOKButton)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MessageBox::addText(const GUI::Font& font, const StringList& text)
{
  const int lineHeight = font.getLineHeight(),
            fontWidth  = font.getMaxCharWidth(),
            fontHeight = font.getFontHeight();
  int xpos, ypos;

  // Set real dimensions
  int str_w = 0;
  for(const auto& s: text)
    str_w = std::max(int(s.length()), str_w);
  _w = std::min(str_w * fontWidth + 20, _w);
  _h = std::min(uInt32((text.size() + 2) * lineHeight + 20 + _th), uInt32(_h));

  xpos = 10;  ypos = 10 + _th;
  for(const auto& s: text)
  {
    new StaticTextWidget(this, font, xpos, ypos, _w - 20,
                         fontHeight, s, TextAlign::Left);
    ypos += fontHeight;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MessageBox::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case GuiObject::kOKCmd:
    {
      close();

      // Send a signal to the calling class that 'OK' has been selected
      // Since we aren't derived from a widget, we don't have a 'data' or 'id'
      if(myCmd)
        sendCommand(myCmd, 0, 0);

      break;
    }

    default:
      Dialog::handleCommand(sender, cmd, data, id);
      break;
  }
}

}  // namespace GUI
