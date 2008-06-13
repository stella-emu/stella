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
// $Id: ContextMenu.cxx,v 1.1 2008-06-13 13:14:51 stephena Exp $
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
ContextMenu::ContextMenu(GuiObject* boss, const GUI::Font& font,
                         const StringList& items, int cmd)
  : Dialog(&boss->instance(), &boss->parent(), 0, 0, 16, 16),
    CommandSender(boss),
    _entries(items),
    _currentItem(-1),
    _selectedItem(-1),
    _rowHeight(font.getLineHeight()),
    _font(&font),
    _cmd(cmd)
{
  // Resize to largest string
  int maxwidth = 0;
  for(unsigned int i = 0; i < _entries.size(); ++i)
  {
    int length = _font->getStringWidth(_entries[i]);
    if(length > maxwidth)
      maxwidth = length;
  }

  _x = _y = 0;
  _w = maxwidth + 8;
  _h = _rowHeight * _entries.size() + 4;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ContextMenu::~ContextMenu()
{
  _entries.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::show(uInt32 x, uInt32 y, int item)
{
  // Make sure position is set *after* the dialog is added, since the surface
  // may not exist before then
  parent().addDialog(this);
  surface().setPos(x, y);
  setSelected(item);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::setSelected(int item)
{
  if(item >= 0 && item < (int)_entries.size())
    _selectedItem = _currentItem = item;
  else
    _selectedItem = _currentItem = -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::setSelected(const string& name)
{
  for(unsigned int item = 0; item < _entries.size(); ++item)
  {
    if(_entries[item] == name)
    {
      setSelected(item);
      return;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::setSelectedMax()
{
  setSelected(_entries.size() - 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::clearSelection()
{
  _selectedItem = _currentItem = -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int ContextMenu::getSelected() const
{
  return _selectedItem;
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
      parent().removeDialog();
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
  drawCurrentSelection(item);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::handleKeyDown(int ascii, int keycode, int modifiers)
{
  handleEvent(instance().eventHandler().eventForKey(keycode, kMenuMode));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::handleJoyDown(int stick, int button)
{
  handleEvent(instance().eventHandler().eventForJoyButton(stick, button, kMenuMode));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::handleJoyAxis(int stick, int axis, int value)
{
  if(value != 0)  // we don't care about 'axis up' events
    handleEvent(instance().eventHandler().eventForJoyAxis(stick, axis, value, kMenuMode));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ContextMenu::handleJoyHat(int stick, int hat, int value)
{
  handleEvent(instance().eventHandler().eventForJoyHat(stick, hat, value, kMenuMode));
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::handleEvent(Event::Type e)
{
  switch(e)
  {
    case Event::UISelect:
      sendSelection();
      break;
    case Event::UIUp:
    case Event::UILeft:
      moveUp();
      break;
    case Event::UIDown:
    case Event::UIRight:
      moveDown();
      break;
    case Event::UIHome:
      drawCurrentSelection(0);
      break;
    case Event::UIEnd:
      drawCurrentSelection(_entries.size()-1);
      break;
    case Event::UICancel:
      parent().removeDialog();
      break;
    default:
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
void ContextMenu::drawCurrentSelection(int item)
{
  if(item != _currentItem)
  {
    // Change selection
    _currentItem = item;
    setDirty(); draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::sendSelection()
{
  // Send any command associated with the selection
  _selectedItem = _currentItem;
  sendCommand(_cmd ? _cmd : kCMenuItemSelectedCmd, _selectedItem, -1);

  // We remove the dialog when the user has selected an item
  parent().removeDialog();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::moveUp()
{
  int item = _currentItem;
  if(item > 0)
    drawCurrentSelection(--item);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::moveDown()
{
  int item = _currentItem;
  if(item < (int)_entries.size() - 1)
    drawCurrentSelection(++item);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::center()
{
  // Adjust dialog, making sure it doesn't fall outside screen area

  // FIXME - add code to do this ...
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::drawDialog()
{
  // Normally we add widgets and let Dialog::draw() take care of this
  // logic.  But for some reason, this Dialog was written differently
  // by the ScummVM guys, so I'm not going to mess with it.
  FBSurface& s = surface();

  if(_dirty)
  {
    FBSurface& s = surface();

    s.fillRect(_x+1, _y+1, _w-2, _h-2, kWidColor);
    s.box(_x, _y, _w, _h, kColor, kShadowColor);

    // Draw the entries
    int count = _entries.size();
    for(int i = 0; i < count; i++)
    {
      bool hilite = i == _currentItem;
      int x = _x + 2;
      int y = _y + 2 + i * _rowHeight;
      int w = _w - 4;

      if(hilite) s.fillRect(x, y, w, _rowHeight, kTextColorHi);
      s.drawString(_font, _entries[i], x + 1, y + 2, w - 2,
                   hilite ? kWidColor : kTextColor);
    }
    s.addDirtyRect(_x, _y, _w, _h);
    _dirty = false;
  }

  // Commit surface changes to screen
  s.update();
}
