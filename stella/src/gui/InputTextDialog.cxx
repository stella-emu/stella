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
// $Id: InputTextDialog.cxx,v 1.7 2005-10-14 13:50:00 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "Widget.hxx"
#include "EditTextWidget.hxx"
#include "Dialog.hxx"
#include "GuiObject.hxx"
#include "GuiUtils.hxx"
#include "InputTextDialog.hxx"

#include "bspf.hxx"

enum {
  kAcceptCmd = 'ACPT'
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputTextDialog::InputTextDialog(GuiObject* boss, const GUI::Font& font,
                                 int x, int y)
  : Dialog(boss->instance(), boss->parent(), x, y, 16, 16),
    CommandSender(boss),
    _errorFlag(false)
{
  const int fontWidth  = font.getMaxCharWidth(),
            fontHeight = font.getFontHeight(),
            lineHeight = font.getLineHeight();
  int xpos, ypos;

  // Calculate real dimensions
  _w = fontWidth * 30;
  _h = lineHeight * 5;

  xpos = 10; ypos = lineHeight;
  int lwidth = font.getStringWidth("Enter Data:");
  StaticTextWidget* t = 
  new StaticTextWidget(this, xpos, ypos,
                       lwidth, fontHeight,
                       "Enter Data:", kTextAlignLeft);
  t->setFont(font);

  xpos += lwidth + fontWidth;
  _input = new EditTextWidget(this, xpos, ypos,
                              _w - xpos - 10, lineHeight, "");
  _input->setFont(font);
  addFocusWidget(_input);

  xpos = 10; ypos = 2*lineHeight + 5;
  _title = new StaticTextWidget(this, xpos, ypos, _w - 2*xpos, fontHeight,
                                "", kTextAlignCenter);
  _title->setColor(kTextColorEm);

#ifndef MAC_OSX
  addButton(_w - 2 * (kButtonWidth + 10), _h - 24, "OK", kAcceptCmd, 0);
  addButton(_w - (kButtonWidth+10), _h - 24, "Cancel", kCloseCmd, 0);
#else
  addButton(_w - 2 * (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd, 0);
  addButton(_w - (kButtonWidth+10), _h - 24, "OK", kAcceptCmd, 0);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::setTitle(const string& title)
{
  _title->setLabel(title);
  _errorFlag = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::handleCommand(CommandSender* sender, int cmd,
                                    int data, int id)
{
  switch (cmd)
  {
    case kAcceptCmd:
    case kEditAcceptCmd:
    {
      // Send a signal to the calling class that a selection has been made
      // Since we aren't derived from a widget, we don't have a 'data' or 'id'
      if(_cmd)
        sendCommand(_cmd, 0, 0);

      // We don't close, but leave the parent to do it
      // If the data isn't valid, the parent may wait until it is
      break;
    }

    case kEditChangedCmd:
      // Erase the invalid message once editing is restarted
      if(_errorFlag)
      {
        _title->setLabel("");
        _errorFlag = false;
      }
      break;

    case kEditCancelCmd:
      Dialog::handleCommand(sender, kCloseCmd, data, id);
      break;


    default:
      Dialog::handleCommand(sender, cmd, data, id);
      break;
  }
}
