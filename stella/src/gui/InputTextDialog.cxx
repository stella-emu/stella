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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: InputTextDialog.cxx,v 1.22 2008-06-19 19:15:44 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "bspf.hxx"

#include "Dialog.hxx"
#include "DialogContainer.hxx"
#include "EditTextWidget.hxx"
#include "GuiObject.hxx"
#include "OSystem.hxx"
#include "Widget.hxx"

#include "InputTextDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputTextDialog::InputTextDialog(GuiObject* boss, const GUI::Font& font,
                                 const StringList& labels)
  : Dialog(&boss->instance(), &boss->parent(), 0, 0, 16, 16),
    CommandSender(boss),
    myErrorFlag(false)
{
  const int fontWidth  = font.getMaxCharWidth(),
            fontHeight = font.getFontHeight(),
            lineHeight = font.getLineHeight();
  unsigned int xpos, ypos, i, lwidth = 0, maxIdx = 0;
  WidgetArray wid;

  // Calculate real dimensions
  _w = fontWidth * 25;
  _h = lineHeight * 4 + labels.size() * (lineHeight + 5);

  // Determine longest label
  for(i = 0; i < labels.size(); ++i)
  {
    if(labels[i].length() > lwidth)
    {
      lwidth = labels[i].length();
      maxIdx = i;
    }
  }
  lwidth = font.getStringWidth(labels[maxIdx]);

  // Create editboxes for all labels
  ypos = lineHeight;
  for(i = 0; i < labels.size(); ++i)
  {
    xpos = 10;
    new StaticTextWidget(this, font, xpos, ypos,
                         lwidth, fontHeight,
                         labels[i], kTextAlignLeft);

    xpos += lwidth + fontWidth;
    EditTextWidget* w = new EditTextWidget(this, font, xpos, ypos,
                                           _w - xpos - 10, lineHeight, "");
    wid.push_back(w);

    myInput.push_back(w);
    ypos += lineHeight + 5;
  }

  xpos = 10;
  myTitle = new StaticTextWidget(this, font, xpos, ypos, _w - 2*xpos, fontHeight,
                                 "", kTextAlignCenter);
  myTitle->setTextColor(kTextColorEm);

  addToFocusList(wid);

  // Add OK and Cancel buttons
  wid.clear();
  addOKCancelBGroup(wid, font);
  addBGroupToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputTextDialog::~InputTextDialog()
{
  myInput.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::show(uInt32 x, uInt32 y)
{
  // Make sure position is set *after* the dialog is added, since the surface
  // may not exist before then
  parent().addDialog(this);

  // Are we in the current screen bounds?
  const GUI::Rect& image = instance().frameBuffer().imageRect();
  uInt32 dx = image.x() + image.width();
  uInt32 dy = image.y() + image.height();
  if(x + _w > dx) x -= (x + _w - dx);
  if(y + _h > dy) y -= (y + _h - dy);

  surface().setPos(x, y);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::setTitle(const string& title)
{
  myTitle->setLabel(title);
  myErrorFlag = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& InputTextDialog::getResult(int idx)
{
  if((unsigned int)idx < myInput.size())
    return myInput[idx]->getEditString();
  else
    return EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::setEditString(const string& str, int idx)
{
  if((unsigned int)idx < myInput.size())
    myInput[idx]->setEditString(str);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::setFocus(int idx)
{
  if((unsigned int)idx < myInput.size())
    Dialog::setFocus(getFocusList()[idx]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::handleCommand(CommandSender* sender, int cmd,
                                    int data, int id)
{
  switch(cmd)
  {
    case kOKCmd:
    case kEditAcceptCmd:
    {
      // Send a signal to the calling class that a selection has been made
      // Since we aren't derived from a widget, we don't have a 'data' or 'id'
      if(myCmd)
        sendCommand(myCmd, 0, 0);

      // We don't close, but leave the parent to do it
      // If the data isn't valid, the parent may wait until it is
      break;
    }

    case kEditChangedCmd:
      // Erase the invalid message once editing is restarted
      if(myErrorFlag)
      {
        myTitle->setLabel("");
        myErrorFlag = false;
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
