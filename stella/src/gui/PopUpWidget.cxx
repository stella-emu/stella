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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: PopUpWidget.cxx,v 1.1 2005-03-14 04:08:15 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "bspf.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "Stack.hxx"
#include "Menu.hxx"
#include "Dialog.hxx"
#include "GuiUtils.hxx"
#include "PopUpWidget.hxx"

#define UP_DOWN_BOX_HEIGHT	10

// Little up/down arrow
static uInt32 up_down_arrows[8] = {
  0x00000000,
  0x00001000,
  0x00011100,
  0x00111110,
  0x00000000,
  0x00111110,
  0x00011100,
  0x00001000,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PopUpDialog::PopUpDialog(PopUpWidget* boss, Int32 clickX, Int32 clickY)
    : Dialog(boss->instance(), 0, 0, 16, 16),
      _popUpBoss(boss)
{
  // Copy the selection index
  _selection = _popUpBoss->_selectedItem;

  // Calculate real popup dimensions
  _x = _popUpBoss->getAbsX() + _popUpBoss->_labelWidth;
  _y = _popUpBoss->getAbsY() - _popUpBoss->_selectedItem * kLineHeight;
  _h = _popUpBoss->_entries.size() * kLineHeight + 2;
  _w = _popUpBoss->_w - 10 - _popUpBoss->_labelWidth;
	
  // Perform clipping / switch to scrolling mode if we don't fit on the screen
  // FIXME - hard coded screen height 200. We really need an API in OSystem to query the
  // screen height, and also OSystem should send out notification messages when the screen
  // resolution changes... we could generalize CommandReceiver and CommandSender.
  if(_h >= 200) // FIXME - change this to actual height of the window
    _h = 199;
  if(_y < 0)
    _y = 0;
  else if(_y + _h >= 200)
    _y = 199 - _h;

  // TODO - implement scrolling if we had to move the menu, or if there are too many entries

  // Remember original mouse position
  _clickX = clickX - _x;
  _clickY = clickY - _y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::drawDialog()
{
  FrameBuffer& fb = _popUpBoss->instance()->frameBuffer();

  // Draw the menu border
  fb.hLine(_x, _y, _x+_w - 1, kColor);
  fb.hLine(_x, _y + _h - 1, _x + _w - 1, kShadowColor);
  fb.vLine(_x, _y, _y+_h - 1, kColor);
  fb.vLine(_x + _w - 1, _y, _y + _h - 1, kShadowColor);

  // Draw the entries
  Int32 count = _popUpBoss->_entries.size();
  for(Int32 i = 0; i < count; i++)
    drawMenuEntry(i, i == _selection);

  fb.addDirtyRect(_x, _y, _w, _h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::handleMouseDown(Int32 x, Int32 y, Int32 button, Int32 clickCount)
{
  _clickX = -1;
  _clickY = -1;
  _openTime = (uInt32)-1;

  // We remove the dialog and delete the dialog when the user has selected an item
  _popUpBoss->instance()->menu().removeDialog();
  delete this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::handleMouseWheel(Int32 x, Int32 y, Int32 direction)
{
  if(direction < 0)
    moveUp();
  else if(direction > 0)
    moveDown();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::handleMouseMoved(Int32 x, Int32 y, Int32 button)
{
  // Compute over which item the mouse is...
  Int32 item = findItem(x, y);

  if(item >= 0 && _popUpBoss->_entries[item].name.size() == 0)
    item = -1;

  if(item == -1 && !isMouseDown())
    return;

  // ...and update the selection accordingly
  setSelection(item);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::handleKeyDown(uInt16 ascii, Int32 keycode, Int32 modifiers)
{
  if(isMouseDown())
    return;

  switch(keycode)
  {
    case '\n':      // enter/return
    case '\r':
      setResult(_selection);
      close();
      break;
    case 256+17:    // up arrow
      moveUp();
      break;
    case 256+18:    // down arrow
      moveDown();
      break;
    case 256+22:    // home
      setSelection(0);
      break;
    case 256+23:    // end
      setSelection(_popUpBoss->_entries.size()-1);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 PopUpDialog::findItem(Int32 x, Int32 y) const
{
  if(x >= 0 && x < _w && y >= 0 && y < _h)
    return (y-2) / kLineHeight;

  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::setSelection(Int32 item)
{
  if(item != _selection)
  {
    // Undraw old selection
    if(_selection >= 0)
      drawMenuEntry(_selection, false);

    // Change selection
    _selection = item;
    _popUpBoss->_selectedItem = item;

    // Draw new selection
    if(item >= 0)
      drawMenuEntry(item, true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PopUpDialog::isMouseDown()
{
  // TODO/FIXME - need a way to determine whether any mouse buttons are pressed or not.
  // Sure, we could just count mouse button up/down events, but that is cumbersome and
  // error prone. Would be much nicer to add an API to OSystem for this...

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::moveUp()
{
  if(_selection < 0)
  {
    setSelection(_popUpBoss->_entries.size() - 1);
  }
  else if(_selection > 0)
  {
    Int32 item = _selection;
    do {
      item--;
    } while (item >= 0 && _popUpBoss->_entries[item].name.size() == 0);
    if(item >= 0)
      setSelection(item);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::moveDown()
{
  Int32 lastItem = _popUpBoss->_entries.size() - 1;

  if(_selection < 0)
  {
    setSelection(0);
  }
  else if(_selection < lastItem)
  {
    Int32 item = _selection;
    do {
      item++;
    } while (item <= lastItem && _popUpBoss->_entries[item].name.size() == 0);
    if(item <= lastItem)
      setSelection(item);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::drawMenuEntry(Int32 entry, bool hilite)
{
  // Draw one entry of the popup menu, including selection
  assert(entry >= 0);
  Int32 x = _x + 1;
  Int32 y = _y + 1 + kLineHeight * entry;
  Int32 w = _w - 2;
  string& name = _popUpBoss->_entries[entry].name;

  FrameBuffer& fb = _popUpBoss->instance()->frameBuffer();
  fb.fillRect(x, y, w, kLineHeight, hilite ? kTextColorHi : kBGColor);
  if(name.size() == 0)
  {
    // Draw a separator
    fb.hLine(x - 1, y + kLineHeight / 2, x + w, kShadowColor);
    fb.hLine(x, y + 1 + kLineHeight / 2, x + w, kColor);
  }
  else
    fb.font().drawString(name, x + 1, y + 2, w - 2, hilite ? kBGColor : kTextColor);

  fb.addDirtyRect(x, y, w, kLineHeight);
}

//
// PopUpWidget
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PopUpWidget::PopUpWidget(GuiObject* boss, Int32 x, Int32 y, Int32 w, Int32 h,
                         const string& label, uInt32 labelWidth)
    : Widget(boss, x, y - 1, w, h + 2),
      CommandSender(boss),
      _label(label),
      _labelWidth(labelWidth)
{
  _flags = WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS;
  _type = kPopUpWidget;

  _selectedItem = -1;

  if(!_label.empty() && _labelWidth == 0)
    _labelWidth = instance()->frameBuffer().font().getStringWidth(_label);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::handleMouseDown(Int32 x, Int32 y, Int32 button, Int32 clickCount)
{
  if(isEnabled())
  {
    myPopUpDialog = new PopUpDialog(this, x + getAbsX(), y + getAbsY());
    instance()->menu().addDialog(myPopUpDialog);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::appendEntry(const string& entry, uInt32 tag)
{
  Entry e;
  e.name = entry;
  e.tag = tag;
  _entries.push_back(e);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::clearEntries()
{
  _entries.clear();
  _selectedItem = -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::setSelected(Int32 item)
{
  if(item != _selectedItem)
  {
    if(item >= 0 && item < (int)_entries.size())
      _selectedItem = item;
    else
      _selectedItem = -1;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::setSelectedTag(uInt32 tag)
{
  for(uInt32 item = 0; item < _entries.size(); ++item)
  {
    if(_entries[item].tag == tag)
    {
      setSelected(item);
      return;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::drawWidget(bool hilite)
{
  FrameBuffer& fb = instance()->frameBuffer();

  Int32 x = _x + _labelWidth;
  Int32 w = _w - _labelWidth;

  // Draw the label, if any
  if (_labelWidth > 0)
    fb.font().drawString(_label, _x, _y + 3, _labelWidth,
                         isEnabled() ? kTextColor : kColor, kTextAlignRight);

  // Draw a thin frame around us.
  fb.hLine(x, _y, x + w - 1, kColor);
  fb.hLine(x, _y +_h-1, x + w - 1, kShadowColor);
  fb.vLine(x, _y, _y+_h-1, kColor);
  fb.vLine(x + w - 1, _y, _y +_h - 1, kShadowColor);

  // Draw an arrow pointing down at the right end to signal this is a dropdown/popup
//  fb.drawBitmap(up_down_arrows, x+w - 10, _y+2,
// FIXME                !isEnabled() ? kColor : hilite ? kTextColorHi : kTextColor);

  // Draw the selected entry, if any
  if(_selectedItem >= 0)
  {
    TextAlignment align = (fb.font().getStringWidth(_entries[_selectedItem].name) > w-6) ?
                           kTextAlignRight : kTextAlignLeft;
    fb.font().drawString(_entries[_selectedItem].name, x+2, _y+3, w-6,
                         !isEnabled() ? kColor : kTextColor, align);
  }
}
