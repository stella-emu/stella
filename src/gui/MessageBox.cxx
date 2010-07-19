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
// Copyright (c) 1995-2010 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "Dialog.hxx"
#include "OSystem.hxx"
#include "Version.hxx"
#include "Widget.hxx"

#include "MessageBox.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MessageBox::MessageBox(GuiObject* boss, const GUI::Font& font,
                       const StringList& text, int cmd)
  : Dialog(&boss->instance(), &boss->parent(), 0, 0, 16, 16),
    CommandSender(boss),
    myCmd(cmd)
{
  const int lineHeight = font.getLineHeight(),
            fontWidth  = font.getMaxCharWidth(),
            fontHeight = font.getFontHeight();
  int xpos, ypos;
  WidgetArray wid;

  // Set real dimensions
  _w = 50 * fontWidth + 8;
  _h = (text.size() + 2) * lineHeight + 20;

  xpos = 10;  ypos = 10;
  for(uInt32 i = 0; i < text.size(); ++i)
  {
    new StaticTextWidget(this, font, xpos, ypos, _w - 20,
                         fontHeight, text[i], kTextAlignLeft);
    ypos += fontHeight;
  }

  addOKCancelBGroup(wid, font);

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MessageBox::~MessageBox()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MessageBox::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case kOKCmd:
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
