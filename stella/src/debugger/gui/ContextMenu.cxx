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
// $Id: ContextMenu.cxx,v 1.6 2006-02-22 17:38:04 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "Dialog.hxx"
#include "DialogContainer.hxx"
#include "ContextMenu.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ContextMenu::ContextMenu(GuiObject* boss, const GUI::Font& font)
  : Dialog(boss->instance(), boss->parent(), 0, 0, 16, 16),
    CommandSender(boss),
    _selectedItem(-1),
    _rowHeight(font.getLineHeight()),
    _font(&font)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ContextMenu::~ContextMenu()
{
  _entries.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::show()
{
  _selectedItem = -1;
  parent()->addDialog(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::setList(const StringList& list)
{
  _entries = list;

  // Resize to largest string
  int maxwidth = 0;
  for(unsigned int i = 0; i < _entries.size(); ++i)
  {
    int length = _font->getStringWidth(_entries[i]);
    if(length > maxwidth)
      maxwidth = length;
  }

  _w = maxwidth + 8;
  _h = _rowHeight * _entries.size() + 4;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& ContextMenu::getSelectedString() const
{
  return (_selectedItem >= 0) ? _entries[_selectedItem] : EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::handleMouseDown(int x, int y, int button, int clickCount)
{
  // Only do a selection when the left button is in the dialog
  if(button == 1)
  {
    x += getAbsX(); y += getAbsY();
    if(x >= _x && x <= _x+_w && y >= _y && y <= _y+_h)
      sendSelection();
    else
      parent()->removeDialog();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::handleMouseWheel(int x, int y, int direction)
{
  if(direction < 0)
    moveUp();
  else if(direction > 0)
    moveDown();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::handleMouseMoved(int x, int y, int button)
{
  // Compute over which item the mouse is...
  int item = findItem(x, y);
  if(item == -1)
    return;

  // ...and update the selection accordingly
  setSelection(item);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::handleKeyDown(int ascii, int keycode, int modifiers)
{
  switch(ascii)
  {
    case 27:        // escape
      parent()->removeDialog();
      break;
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
      setSelection(_entries.size()-1);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int ContextMenu::findItem(int x, int y) const
{
  if(x >= 0 && x < _w && y >= 0 && y < _h)
    return (y-4) / _rowHeight;

  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::setSelection(int item)
{
  if(item != _selectedItem)
  {
    // Change selection
    _selectedItem = item;
    setDirty(); draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::sendSelection()
{
  // We remove the dialog when the user has selected an item
  parent()->removeDialog();

  sendCommand(kCMenuItemSelectedCmd, _selectedItem, -1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::moveUp()
{
  int item = _selectedItem;
  if(item > 0)
    setSelection(--item);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::moveDown()
{
  int item = _selectedItem;
  if(item < (int)_entries.size() - 1)
    setSelection(++item);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::drawDialog()
{
  // Normally we add widgets and let Dialog::draw() take care of this
  // logic.  But for some reason, this Dialog was written differently
  // by the ScummVM guys, so I'm not going to mess with it.
  if(_dirty)
  {
    FrameBuffer& fb = instance()->frameBuffer();

    fb.fillRect(_x+1, _y+1, _w-2, _h-2, kBGColor);
    fb.box(_x, _y, _w, _h, kColor, kShadowColor);

    // Draw the entries
    int count = _entries.size();
    for(int i = 0; i < count; i++)
    {
      bool hilite = i == _selectedItem;
      int x = _x + 2;
      int y = _y + 2 + i * _rowHeight;
      int w = _w - 4;
      string& name = _entries[i];

      fb.fillRect(x, y, w, _rowHeight, hilite ? kTextColorHi : kBGColor);

      fb.drawString(_font, name, x + 1, y + 2, w - 2,
                    hilite ? kBGColor : kTextColor);
    }
    _dirty = false;
    fb.addDirtyRect(_x, _y, _w, _h);
  }
}
