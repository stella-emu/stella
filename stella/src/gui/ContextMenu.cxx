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
// $Id: ContextMenu.cxx,v 1.6 2008-08-04 20:12:23 stephena Exp $
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
                         const StringMap& items, int cmd)
  : Dialog(&boss->instance(), &boss->parent(), 0, 0, 16, 16),
    CommandSender(boss),
    _currentItem(-1),
    _selectedItem(-1),
    _rowHeight(font.getLineHeight()),
    _twoColumns(false),
    _font(&font),
    _cmd(cmd),
    _xorig(0),
    _yorig(0)
{
  addItems(items);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ContextMenu::~ContextMenu()
{
  _entries.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::addItems(const StringMap& items)
{
  _entries.clear();
  _entries = items;

  // Create two columns of entries if there are more than 10 items
  if(_entries.size() > 10)
  {
    _twoColumns = true;
    _entriesPerColumn = _entries.size() / 2;
  
    if(_entries.size() & 1)
      _entriesPerColumn++;
  }
  else
  {
    _twoColumns = false;
    _entriesPerColumn = _entries.size();
  }

  // Resize to largest string
  int maxwidth = 0;
  for(unsigned int i = 0; i < _entries.size(); ++i)
  {
    int length = _font->getStringWidth(_entries[i].first);
    if(length > maxwidth)
      maxwidth = length;
  }

  _x = _y = 0;
  _w = maxwidth * (_twoColumns ? 2 : 1) + 10;
  _h = _entriesPerColumn * _rowHeight + 4;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::show(uInt32 x, uInt32 y, int item)
{
  // Make sure position is set *after* the dialog is added, since the surface
  // may not exist before then
  parent().addDialog(this);
  _xorig = x;
  _yorig = y;
  center();
  setSelected(item);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::center()
{
cerr << " ==> ContextMenu::center()" << endl;

  // Make sure the menu is exactly where it should be, in case the image
  // offset has changed
  uInt32 x = _xorig, y = _yorig;
  const GUI::Rect& image = instance().frameBuffer().imageRect();
  uInt32 tx = image.x() + image.width();
  uInt32 ty = image.y() + image.height();
  if(x + _w > tx) x -= (x + _w - tx);
  if(y + _h > ty) y -= (y + _h - ty);

  surface().setPos(x, y);


/*
  uInt32 tx, ty;
  const GUI::Rect& image = instance().frameBuffer().imageRect();
  dialog().surface().getPos(tx, ty);
  tx += image.x();
  ty += image.y();
  surface().setPos(tx, ty);
*/
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
void ContextMenu::setSelected(const string& tag, const string& defaultTag)
{
  for(unsigned int item = 0; item < _entries.size(); ++item)
  {
    if(BSPF_strcasecmp(_entries[item].second.c_str(), tag.c_str()) == 0)
    {
      setSelected(item);
      return;
    }
  }

  // If we get this far, the value wasn't found; use the default value
  for(unsigned int item = 0; item < _entries.size(); ++item)
  {
    if(BSPF_strcasecmp(_entries[item].second.c_str(), defaultTag.c_str()) == 0)
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
const string& ContextMenu::getSelectedName() const
{
  return (_selectedItem >= 0) ? _entries[_selectedItem].first : EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& ContextMenu::getSelectedTag() const
{
  return (_selectedItem >= 0) ? _entries[_selectedItem].second : EmptyString;
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
      close();
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
      close();
      break;
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int ContextMenu::findItem(int x, int y) const
{
  if(x >= 0 && x < _w && y >= 0 && y < _h)
  {
    if(_twoColumns)
    {
      unsigned int entry = (y - 4) / _rowHeight;
      if(x > _w / 2)
      {
        entry += _entriesPerColumn;
  
        if(entry >= _entries.size())
          return -1;
      }
      return entry;
    }
    return (y - 4) / _rowHeight;
  }
  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::drawCurrentSelection(int item)
{
  if(item != _currentItem)
  {
    // Change selection
    _currentItem = item;
    setDirty();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::sendSelection()
{
  // We remove the dialog when the user has selected an item
  // Make sure the dialog is removed before sending any commands,
  // since one consequence of sending a command may be to add another
  // dialog/menu
  close();

  // Send any command associated with the selection
  _selectedItem = _currentItem;
  sendCommand(_cmd ? _cmd : kCMenuItemSelectedCmd, _selectedItem, -1);
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
void ContextMenu::drawDialog()
{
  // Normally we add widgets and let Dialog::draw() take care of this
  // logic.  But for some reason, this Dialog was written differently
  // by the ScummVM guys, so I'm not going to mess with it.
  FBSurface& s = surface();

  if(_dirty)
  {
    // Draw menu border and background
    s.fillRect(_x+1, _y+1, _w-2, _h-2, kWidColor);
    s.box(_x, _y, _w, _h, kColor, kShadowColor);

    // If necessary, draw dividing line
    if(_twoColumns)
      s.vLine(_x + _w / 2, _y, _y + _h - 2, kColor);

    // Draw the entries
    int x, y, w;
    int count = _entries.size();
    for(int i = 0; i < count; i++)
    {
      bool hilite = i == _currentItem;
      if(_twoColumns)
      {
        int n = _entries.size() / 2;
        if(_entries.size() & 1) n++;
        if(i >= n)
        {
          x = _x + 1 + _w / 2;
          y = _y + 2 + _rowHeight * (i - n);
        }
        else
        {
          x = _x + 2;
          y = _y + 2 + _rowHeight * i;
        }
        w = _w / 2 - 3;
      }
      else
      {
        x = _x + 2;
        y = _y + 2 + i * _rowHeight;
        w = _w - 4;
      }
      if(hilite) s.fillRect(x, y, w, _rowHeight, kTextColorHi);
      s.drawString(_font, _entries[i].first, x + 1, y + 2, w - 2,
                   hilite ? kWidColor : kTextColor);
    }
    s.addDirtyRect(_x, _y, _w, _h);
    _dirty = false;
  }

  // Commit surface changes to screen
  s.update();
}
