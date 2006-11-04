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
// $Id: PopUpWidget.cxx,v 1.27 2006-11-04 19:38:25 stephena Exp $
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
#include "StringListWidget.hxx"
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
  _y = _popUpBoss->getAbsY() - _popUpBoss->_selectedItem * _popUpBoss->_fontHeight;
  _w = _popUpBoss->_w - _popUpBoss->_labelWidth - 10;
  _h = 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::drawDialog()
{
  // Normally we add widgets and let Dialog::draw() take care of this
  // logic.  But for some reason, this Dialog was written differently
  // by the ScummVM guys, so I'm not going to mess with it.
  if(_dirty)
  {
    FrameBuffer& fb = instance()->frameBuffer();

//cerr << "PopUpDialog::drawDialog()\n";
    // Draw the menu border
    fb.hLine(_x, _y, _x + _w - 1, kColor);
    fb.hLine(_x, _y + _h - 1, _x + _w - 1, kShadowColor);
    fb.vLine(_x, _y, _y + _h - 1, kColor);
    fb.vLine(_x + _w - 1, _y, _y + _h - 1, kShadowColor);

    // If necessary, draw dividing line
    if(_twoColumns)
      fb.vLine(_x + _w / 2, _y, _y + _h - 2, kColor);

    // Draw the entries
    int count = _popUpBoss->_entries.size();
    for(int i = 0; i < count; i++)
      drawMenuEntry(i, i == _selection);

    // The last entry may be empty. Fill it with black.
    if(_twoColumns && (count & 1))
      fb.fillRect(_x + 1 + _w / 2, _y + 1 + _popUpBoss->_fontHeight * (_entriesPerColumn - 1),
                  _w / 2 - 1, _popUpBoss->_fontHeight, kBGColor);

    _dirty = false;
    fb.addDirtyRect(_x, _y, _w, _h);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::handleMouseDown(int x, int y, int button, int clickCount)
{
  // Only make a selection if we're in the dialog area
  if(x >= 0 && x < _w && y >= 0 && y < _h)
    sendSelection();
  else
    cancelSelection();
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

  switch (ascii)
  {
    case 27:        // escape
      cancelSelection();
      break;
    default:
      handleEvent(instance()->eventHandler().eventForKey(ascii, kMenuMode));
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::handleEvent(Event::Type e)
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
      setSelection(0);
      break;
    case Event::UIEnd:
      setSelection(_popUpBoss->_entries.size()-1);
      break;
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::drawMenuEntry(int entry, bool hilite)
{
//cerr << "PopUpDialog::drawMenuEntry\n";
  FrameBuffer& fb = instance()->frameBuffer();

  // Draw one entry of the popup menu, including selection
  int x, y, w;

  if(_twoColumns)
  {
    int n = _popUpBoss->_entries.size() / 2;

    if(_popUpBoss->_entries.size() & 1)
      n++;

    if (entry >= n)
    {
      x = _x + 1 + _w / 2;
      y = _y + 1 + _popUpBoss->_fontHeight * (entry - n);
    }
    else
    {
      x = _x + 1;
      y = _y + 1 + _popUpBoss->_fontHeight * entry;
    }

    w = _w / 2 - 1;
  }
  else
  {
    x = _x + 1;
    y = _y + 1 + _popUpBoss->_fontHeight * entry;
    w = _w - 2;
  }

  string& name = _popUpBoss->_entries[entry].name;
  fb.fillRect(x, y, w, _popUpBoss->_fontHeight, hilite ? kTextColorHi : kBGColor);

  if(name.size() == 0)
  {
    // Draw a separator
    fb.hLine(x - 1, y + _popUpBoss->_fontHeight / 2, x + w, kShadowColor);
    fb.hLine(x, y + 1 + _popUpBoss->_fontHeight / 2, x + w, kColor);
  }
  else
    fb.drawString(_popUpBoss->font(), name, x + 1, y + 2, w - 2,
                  hilite ? kBGColor : kTextColor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::recalc()
{
  // Perform clipping / switch to scrolling mode if we don't fit on the screen
  const int height = instance()->frameBuffer().baseHeight();

  _h = _popUpBoss->_entries.size() * _popUpBoss->_fontHeight + 2;

  // HACK: For now, we do not do scrolling. Instead, we draw the dialog
  // in two columns if it's too tall.
  if(_h >= height)
  {
    const int width = instance()->frameBuffer().baseWidth();

    _twoColumns = true;
    _entriesPerColumn = _popUpBoss->_entries.size() / 2;

    if(_popUpBoss->_entries.size() & 1)
      _entriesPerColumn++;

    _h = _entriesPerColumn * _popUpBoss->_fontHeight + 2;
    _w = 0;

    // Find width of largest item
    for(unsigned int i = 0; i < _popUpBoss->_entries.size(); i++)
    {
      int width = _popUpBoss->_font->getStringWidth(_popUpBoss->_entries[i].name);

      if(width > _w)
      _w = width;
    }

    _w = 2 * _w + 10;

    if (!(_w & 1))
      _w++;

    if(_popUpBoss->_selectedItem >= _entriesPerColumn)
    {
      _x -= _w / 2;
      _y = _popUpBoss->getAbsY() - (_popUpBoss->_selectedItem - _entriesPerColumn) *
           _popUpBoss->_fontHeight;
    }

    if(_w >= width)
      _w = width - 1;
    if(_x < 0)
      _x = 0;
    if(_x + _w >= width)
      _x = width - 1 - _w;
  }
  else
    _twoColumns = false;

  if(_h >= height)
    _h = height - 1;
  if(_y < 0)
    _y = 0;
  else if(_y + _h >= height)
    _y = height - 1 - _h;

  // TODO - implement scrolling if we had to move the menu, or if there are too many entries
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int PopUpDialog::findItem(int x, int y) const
{
  if(x >= 0 && x < _w && y >= 0 && y < _h)
  {
    if(_twoColumns)
    {
      unsigned int entry = (y - 2) / _popUpBoss->_fontHeight;
      if(x > _w / 2)
      {
        entry += _entriesPerColumn;

        if(entry >= _popUpBoss->_entries.size())
          return -1;
      }
      return entry;
    }
    return (y - 2) / _popUpBoss->_fontHeight;
  }

  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::setSelection(int item)
{
  if(item != _selection)
  {
    // Change selection
    _selection = item;
    _popUpBoss->_selectedItem = item;

    setDirty(); _popUpBoss->setDirty(); _popUpBoss->draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::sendSelection()
{
  if(_popUpBoss->_cmd)
    _popUpBoss->sendCommand(_popUpBoss->_cmd,
                            _popUpBoss->_entries[_selection].tag,
                            _popUpBoss->_id);

  // We remove the dialog when the user has selected an item
  parent()->removeDialog();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpDialog::cancelSelection()
{
  setSelection(_oldSelection);
  parent()->removeDialog();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PopUpDialog::isMouseDown()
{
  // TODO - need a way to determine whether any mouse buttons are pressed or not.
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

//
// PopUpWidget
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PopUpWidget::PopUpWidget(GuiObject* boss, const GUI::Font& font,
                         int x, int y, int w, int h,
                         const string& label, int labelWidth, int cmd)
  : Widget(boss, font, x, y - 1, w, h + 2),
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

  _w = w + _labelWidth + 15;

  // vertically center the arrows and text
  myTextY   = (_h - _font->getFontHeight()) / 2;
  myArrowsY = (_h - 8) / 2;

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
  {
    myPopUpDialog->_oldSelection = _selectedItem;
    parent()->addDialog(myPopUpDialog);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PopUpWidget::handleEvent(Event::Type e)
{
  if(!isEnabled())
    return false;

  switch(e)
  {
    case Event::UISelect:
      handleMouseDown(0, 0, 1, 0);
      return true;
    default:
      return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::appendEntry(const string& entry, int tag)
{
  Entry e;
  e.name = entry;
  e.tag = tag;
  _entries.push_back(e);

  // Each time an entry is added, the popup dialog gets larger
  // This isn't as efficient as it could be, since it's called
  // each time an entry is added (which can be dozens of times).
  myPopUpDialog->recalc();
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
//cerr << "PopUpWidget::drawWidget\n";
  FrameBuffer& fb = instance()->frameBuffer();

  int x = _x + _labelWidth;
  int w = _w - _labelWidth;

  // Draw the label, if any
  if (_labelWidth > 0)
    fb.drawString(_font, _label, _x, _y + myTextY, _labelWidth,
                  isEnabled() ? kTextColor : kColor, kTextAlignRight);

  // Draw a thin frame around us.
  fb.hLine(x, _y, x + w - 1, kColor);
  fb.hLine(x, _y +_h-1, x + w - 1, kShadowColor);
  fb.vLine(x, _y, _y+_h-1, kColor);
  fb.vLine(x + w - 1, _y, _y +_h - 1, kShadowColor);

  // Draw an arrow pointing down at the right end to signal this is a dropdown/popup
  fb.drawBitmap(up_down_arrows, x+w - 10, _y + myArrowsY,
                !isEnabled() ? kColor : hilite ? kTextColorHi : kTextColor);

  // Draw the selected entry, if any
  if(_selectedItem >= 0)
  {
    TextAlignment align = (_font->getStringWidth(_entries[_selectedItem].name) > w-6) ?
                           kTextAlignRight : kTextAlignLeft;
    fb.drawString(_font, _entries[_selectedItem].name, x+2, _y+myTextY, w-6,
                  !isEnabled() ? kColor : kTextColor, align);
  }
}
