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
// $Id: PopUpWidget.cxx,v 1.13 2005-06-23 14:33:11 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "bspf.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "Stack.hxx"
#include "Dialog.hxx"
#include "DialogContainer.hxx"
#include "GuiUtils.hxx"
#include "PopUpWidget.hxx"

#define UP_DOWN_BOX_HEIGHT	10

// Little up/down arrow
static unsigned int up_down_arrows[8] = {
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
PopUpDialog::PopUpDialog(PopUpWidget* boss, int clickX, int clickY)
    : Dialog(boss->instance(), boss->parent(), 0, 0, 16, 16),
      _popUpBoss(boss)
{
  // Copy the selection index
  _selection = _popUpBoss->_selectedItem;

  // Calculate real popup dimensions
  _x = _popUpBoss->getAbsX() + _popUpBoss->_labelWidth;
  _y = _popUpBoss->getAbsY() - _popUpBoss->_selectedItem * kLineHeight;
  _w = _popUpBoss->_w - 10 - _popUpBoss->_labelWidth;
  _h = 2;  // this will increase as more items are added
	
  // Perform clipping / switch to scrolling mode if we don't fit on the screen
  int height = instance()->frameBuffer().baseHeight();
  if(_h >= height)
    _h = height - 1;
  if(_y < 0)
    _y = 0;
  else if(_y + _h >= height)
    _y = height - _h - 1;

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
  int count = _popUpBoss->_entries.size();
  for(int i = 0; i < count; i++)
    drawMenuEntry(i, i == _selection);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::handleMouseDown(int x, int y, int button, int clickCount)
{
  sendSelection();

  _clickX = -1;
  _clickY = -1;
  _openTime = (unsigned int)-1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::handleMouseWheel(int x, int y, int direction)
{
  if(direction < 0)
    moveUp();
  else if(direction > 0)
    moveDown();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::handleMouseMoved(int x, int y, int button)
{
  // Compute over which item the mouse is...
  int item = findItem(x, y);

  if(item >= 0 && _popUpBoss->_entries[item].name.size() == 0)
    item = -1;

  if(item == -1 && !isMouseDown())
    return;

  // ...and update the selection accordingly
  setSelection(item);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::handleKeyDown(int ascii, int keycode, int modifiers)
{
  if(isMouseDown())
    return;

  switch(keycode)
  {
    case '\n':      // enter/return
    case '\r':
      sendSelection();
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
int PopUpDialog::findItem(int x, int y) const
{
  if(x >= 0 && x < _w && y >= 0 && y < _h)
    return (y-2) / kLineHeight;

  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::setSelection(int item)
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

    // TODO - dirty rectangle
    _popUpBoss->instance()->frameBuffer().refreshOverlay();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::sendSelection()
{
  if(_popUpBoss->_cmd)
    _popUpBoss->sendCommand(_popUpBoss->_cmd, _selection);

  // We remove the dialog when the user has selected an item
  parent()->removeDialog();
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
    int item = _selection;
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
  int lastItem = _popUpBoss->_entries.size() - 1;

  if(_selection < 0)
  {
    setSelection(0);
  }
  else if(_selection < lastItem)
  {
    int item = _selection;
    do {
      item++;
    } while (item <= lastItem && _popUpBoss->_entries[item].name.size() == 0);
    if(item <= lastItem)
      setSelection(item);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::drawMenuEntry(int entry, bool hilite)
{
  // Draw one entry of the popup menu, including selection
  assert(entry >= 0);
  int x = _x + 1;
  int y = _y + 1 + kLineHeight * entry;
  int w = _w - 2;
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
    fb.drawString(_popUpBoss->font(), name, x + 1, y + 2, w - 2,
                  hilite ? kBGColor : kTextColor);
}

//
// PopUpWidget
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PopUpWidget::PopUpWidget(GuiObject* boss, int x, int y, int w, int h,
                         const string& label, int labelWidth, int cmd)
    : Widget(boss, x, y - 1, w, h + 2),
      CommandSender(boss),
      _label(label),
      _labelWidth(labelWidth),
      _cmd(cmd)
{
  _flags = WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS;
  _type = kPopUpWidget;

  _selectedItem = -1;

  if(!_label.empty() && _labelWidth == 0)
    _labelWidth = _font->getStringWidth(_label);

  myPopUpDialog = new PopUpDialog(this, x + getAbsX(), y + getAbsY());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PopUpWidget::~PopUpWidget()
{
  delete myPopUpDialog;
  myPopUpDialog = NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
  if(isEnabled())
    parent()->addDialog(myPopUpDialog);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::appendEntry(const string& entry, int tag)
{
  Entry e;
  e.name = entry;
  e.tag = tag;
  _entries.push_back(e);

  // Each time an entry is added, the popup dialog gets larger
  myPopUpDialog->setHeight(myPopUpDialog->getHeight() + kLineHeight);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::clearEntries()
{
  _entries.clear();
  _selectedItem = -1;

  // Reset the height of the popup dialog to be empty
  myPopUpDialog->setHeight(2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::setSelected(int item)
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
void PopUpWidget::setSelectedTag(int tag)
{
  for(unsigned int item = 0; item < _entries.size(); ++item)
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

  int x = _x + _labelWidth;
  int w = _w - _labelWidth;

  // Draw the label, if any
  if (_labelWidth > 0)
    fb.drawString(_font, _label, _x, _y + 3, _labelWidth,
                  isEnabled() ? kTextColor : kColor, kTextAlignRight);

  // Draw a thin frame around us.
  fb.hLine(x, _y, x + w - 1, kColor);
  fb.hLine(x, _y +_h-1, x + w - 1, kShadowColor);
  fb.vLine(x, _y, _y+_h-1, kColor);
  fb.vLine(x + w - 1, _y, _y +_h - 1, kShadowColor);

  // Draw an arrow pointing down at the right end to signal this is a dropdown/popup
  fb.drawBitmap(up_down_arrows, x+w - 10, _y+2,
                !isEnabled() ? kColor : hilite ? kTextColorHi : kTextColor);

  // Draw the selected entry, if any
  if(_selectedItem >= 0)
  {
    TextAlignment align = (_font->getStringWidth(_entries[_selectedItem].name) > w-6) ?
                           kTextAlignRight : kTextAlignLeft;
    fb.drawString(_font, _entries[_selectedItem].name, x+2, _y+3, w-6,
                  !isEnabled() ? kColor : kTextColor, align);
  }
}
