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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: InputTextDialog.cxx,v 1.3 2005-08-10 12:23:42 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "Widget.hxx"
#include "EditNumWidget.hxx"
#include "Dialog.hxx"
#include "GuiObject.hxx"
#include "GuiUtils.hxx"
#include "InputTextDialog.hxx"

#include "bspf.hxx"

enum {
  kAcceptCmd = 'ACPT'
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputTextDialog::InputTextDialog(GuiObject* boss, const GUI::Font& font)
  : Dialog(boss->instance(), boss->parent(), 0, 0, 16, 16),
    CommandSender(boss)
{
  const int fontWidth  = font.getMaxCharWidth(),
            fontHeight = font.getFontHeight(),
            lineHeight = font.getLineHeight();
  int xpos, ypos;

  // Calculate real dimensions
  _w = fontWidth * 30;
  _h = lineHeight * 6;
  _x = (boss->getWidth() - _w) / 2;
  _y = (boss->getHeight() - _h) / 2;

  xpos = 10; ypos = lineHeight;
  int lwidth = font.getStringWidth("Enter Data:");
  StaticTextWidget* t = 
  new StaticTextWidget(this, xpos, ypos,
                       lwidth, fontHeight,
                       "Enter Data:", kTextAlignLeft);
  t->setFont(font);

  xpos += lwidth + fontWidth;
  _input = new EditNumWidget(this, xpos, ypos,
                             _w - xpos - 10, lineHeight, "");
  _input->setFont(font);
  addFocusWidget(_input);

  xpos = 10; ypos = 2*lineHeight;
  _title = new StaticTextWidget(this, xpos, ypos, _w - 2*xpos, fontHeight,
                                "", kTextAlignCenter);

#ifndef MAC_OSX
  addButton(_w - 2 * (kButtonWidth + 10), _h - 24, "OK", kAcceptCmd, 0);
  addButton(_w - (kButtonWidth+10), _h - 24, "Cancel", kCloseCmd, 0);
#else
  addButton(_w - 2 * (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd, 0);
  addButton(_w - (kButtonWidth+10), _h - 24, "OK", kAcceptCmd, 0);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::handleCommand(CommandSender* sender, int cmd,
                                    int data, int id)
{
  switch (cmd)
  {
    case kAcceptCmd:
    {
      // Send a signal to the calling class that a selection has been made
      // Since we aren't derived from a widget, we don't have a 'data' or 'id'
      if(_cmd)
        sendCommand(_cmd, 0, 0);

      // We don't close, but leave the parent to do it
      // If the data isn't valid, the parent may wait until it is
      break;
    }
    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
