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
    _rowHeight(font.getLineHeight()),
    _firstEntry(0),
    _numEntries(0),
    _selectedOffset(0),
    _selectedItem(-1),
    _showScroll(false),
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

  // Resize to largest string
  int maxwidth = 0;
  for(unsigned int i = 0; i < _entries.size(); ++i)
  {
    int length = _font->getStringWidth(_entries[i].first);
    if(length > maxwidth)
      maxwidth = length;
  }

  _x = _y = 0;
  _w = maxwidth + 10;
  _h = 1;  // recalculate this in ::recalc()
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::show(uInt32 x, uInt32 y, int item)
{
  _xorig = x;
  _yorig = y;

  recalc(instance().frameBuffer().imageRect());
  parent().addDialog(this);
  setSelected(item);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::center()
{
  // Make sure the menu is exactly where it should be, in case the image
  // offset has changed
  const GUI::Rect& image = instance().frameBuffer().imageRect();
  recalc(image);
  uInt32 x = image.x() + _xorig;
  uInt32 y = image.y() + _yorig;
  uInt32 tx = image.x() + image.width();
  uInt32 ty = image.y() + image.height();
  if(x + _w > tx) x -= (x + _w - tx);
  if(y + _h > ty) y -= (y + _h - ty);

  surface().setPos(x, y);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::recalc(const GUI::Rect& image)
{
  // Now is the time to adjust the height
  // If it's higher than the screen, we need to scroll through
  int maxentries = (image.height() - 4) / _rowHeight;
  if((int)_entries.size() > maxentries)
  {
    // We show two less than the max, so we have room for two scroll buttons
    _numEntries = maxentries - 2;
    _h = maxentries * _rowHeight + 4;
    _showScroll = true;
  }
  else
  {
    _numEntries = _entries.size();
    _h = _entries.size() * _rowHeight + 4;
    _showScroll = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::setSelected(int item)
{
  if(item >= 0 && item < (int)_entries.size())
    _selectedItem = item;
  else
    _selectedItem = -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::setSelected(const string& tag, const string& defaultTag)
{
  for(unsigned int item = 0; item < _entries.size(); ++item)
  {
    if(BSPF_equalsIgnoreCase(_entries[item].second, tag) == 0)
    {
      setSelected(item);
      return;
    }
  }

  // If we get this far, the value wasn't found; use the default value
  for(unsigned int item = 0; item < _entries.size(); ++item)
  {
    if(BSPF_equalsIgnoreCase(_entries[item].second, defaultTag) == 0)
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
  _selectedItem = -1;
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
    return (y - 4) / _rowHeight;

  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::drawCurrentSelection(int item)
{
  // Change selection
  _selectedOffset = item;
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::sendSelection()
{
  // Select the correct item when scrolling; we have to take into account
  // that the viewable items are no longer 1-to-1 with the entries
  int item = _firstEntry + _selectedOffset;

  if(_showScroll)
  {
    if(_selectedOffset == 0)  // scroll up
      return scrollUp();
    else if(_selectedOffset == _numEntries+1) // scroll down
      return scrollDown();
    else
      item--;
  }

  // We remove the dialog when the user has selected an item
  // Make sure the dialog is removed before sending any commands,
  // since one consequence of sending a command may be to add another
  // dialog/menu
  close();

  // Send any command associated with the selection
  _selectedItem = item;
  sendCommand(_cmd ? _cmd : kCMenuItemSelectedCmd, _selectedItem, -1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::moveUp()
{
  if(_showScroll)
  {
    // Reaching the top of the list means we have to scroll up, but keep the
    // current item offset
    // Otherwise, the offset should decrease by 1
    if(_selectedOffset == 1)
      scrollUp();
    else if(_selectedOffset > 1)
      drawCurrentSelection(_selectedOffset-1);
  }
  else
  {
    if(_selectedOffset > 0)
      drawCurrentSelection(_selectedOffset-1);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::moveDown()
{
  if(_showScroll)
  {
    // Reaching the bottom of the list means we have to scroll down, but keep the
    // current item offset
    // Otherwise, the offset should increase by 1
    if(_selectedOffset == _numEntries)
      scrollDown();
    else if(_selectedOffset < (int)_entries.size())
      drawCurrentSelection(_selectedOffset+1);    
  }
  else
  {
    if(_selectedOffset < (int)_entries.size() - 1)
      drawCurrentSelection(_selectedOffset+1);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::scrollUp()
{
  if(_firstEntry > 0)
  {
    _firstEntry--;
    setDirty();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::scrollDown()
{
  if(_firstEntry + _numEntries < (int)_entries.size())
  {
    _firstEntry++;
    setDirty();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::drawDialog()
{
  static uInt32 up_arrow[8] = {
    0x00011000,
    0x00011000,
    0x00111100,
    0x00111100,
    0x01111110,
    0x01111110,
    0x11111111,
    0x11111111
  };
  static uInt32 down_arrow[8] = {
    0x11111111,
    0x11111111,
    0x01111110,
    0x01111110,
    0x00111100,
    0x00111100,
    0x00011000,
    0x00011000
  };

  // Normally we add widgets and let Dialog::draw() take care of this
  // logic.  But for some reason, this Dialog was written differently
  // by the ScummVM guys, so I'm not going to mess with it.
  FBSurface& s = surface();

  if(_dirty)
  {
    // Draw menu border and background
    s.fillRect(_x+1, _y+1, _w-2, _h-2, kWidColor);
    s.box(_x, _y, _w, _h, kColor, kShadowColor);

    // Draw the entries, taking scroll buttons into account
    int x = _x + 2, y = _y + 2, w = _w - 4;

    // Show top scroll area
    int offset = _selectedOffset;
    if(_showScroll)
    {
      s.hLine(x, y+_rowHeight-1, w+2, kShadowColor);
      s.drawBitmap(up_arrow, ((_w-_x)>>1)-4, (_rowHeight>>1)+y-4, kScrollColor, 8);
      y += _rowHeight;
      offset--;
    }

    for(int i = _firstEntry, current = 0; i < _firstEntry + _numEntries; ++i, ++current)
    {
      bool hilite = offset == current;
      if(hilite) s.fillRect(x, y, w, _rowHeight, kTextColorHi);
      s.drawString(_font, _entries[i].first, x + 1, y + 2, w,
                   !hilite ? kTextColor : kWidColor);
      y += _rowHeight;
    }

    // Show bottom scroll area
    if(_showScroll)
    {
      s.hLine(x, y, w+2, kShadowColor);
      s.drawBitmap(down_arrow, ((_w-_x)>>1)-4, (_rowHeight>>1)+y-4, kScrollColor, 8);
    }

    s.addDirtyRect(_x, _y, _w, _h);
    _dirty = false;
  }

  // Commit surface changes to screen
  s.update();
}
