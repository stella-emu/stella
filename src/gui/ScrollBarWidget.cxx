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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
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
#include "Dialog.hxx"
#include "FrameBuffer.hxx"
#include "ScrollBarWidget.hxx"
#include "bspf.hxx"

/*
 * TODO:
 * - Allow for a horizontal scrollbar, too?
 * - If there are less items than fit on one pages, no scrolling can be done
 *   and we thus should not highlight the arrows/slider.
 */

#define UP_DOWN_BOX_HEIGHT	18

// Up arrow
static unsigned int up_arrow[8] = {
  0x00011000,
  0x00011000,
  0x00111100,
  0x00111100,
  0x01111110,
  0x01111110,
  0x11111111,
  0x11111111
};

// Down arrow
static unsigned int down_arrow[8] = {
    0x11111111,
    0x11111111,
    0x01111110,
    0x01111110,
    0x00111100,
    0x00111100,
    0x00011000,
    0x00011000
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ScrollBarWidget::ScrollBarWidget(GuiObject* boss, const GUI::Font& font,
                                 int x, int y, int w, int h)
  : Widget(boss, font, x, y, w, h), CommandSender(boss),
    _numEntries(0),
    _entriesPerPage(0),
    _currentPos(0),
    _wheel_lines(0),
    _part(kNoPart),
    _draggingPart(kNoPart),
    _sliderHeight(0),
    _sliderPos(0),
    _sliderDeltaMouseDownPos(0)
{
  _flags = WIDGET_ENABLED | WIDGET_TRACK_MOUSE | WIDGET_CLEARBG;
  _type = kScrollBarWidget;
  _bgcolor = kWidColor;
  _bgcolorhi = kWidColor;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::handleMouseDown(int x, int y, int button,
                                      int clickCount)
{
  // Ignore subsequent mouse clicks when the slider is being moved
  if(_draggingPart == kSliderPart)
    return;

  int old_pos = _currentPos;

  // Do nothing if there are less items than fit on one page
  if(_numEntries <= _entriesPerPage)
    return;

  if (y <= UP_DOWN_BOX_HEIGHT)
  {
    // Up arrow
    _currentPos--;
    _draggingPart = kUpArrowPart;
  }
  else if(y >= _h - UP_DOWN_BOX_HEIGHT)
  {
    // Down arrow
    _currentPos++;
    _draggingPart = kDownArrowPart;
  }
  else if(y < _sliderPos)
  {
    _currentPos -= _entriesPerPage;
  }
  else if(y >= _sliderPos + _sliderHeight)
  {
    _currentPos += _entriesPerPage;
  }
  else
  {
    _draggingPart = kSliderPart;
    _sliderDeltaMouseDownPos = y - _sliderPos;
  }

  // Make sure that _currentPos is still inside the bounds
  checkBounds(old_pos);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::handleMouseUp(int x, int y, int button,
                                    int clickCount)
{
  _draggingPart = kNoPart;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::handleMouseWheel(int x, int y, int direction)
{
  int old_pos = _currentPos;

  if(_numEntries < _entriesPerPage)
    return;

  if(direction < 0)
    _currentPos -= _wheel_lines ? _wheel_lines : _WHEEL_LINES;
  else
    _currentPos += _wheel_lines ? _wheel_lines : _WHEEL_LINES;

  // Make sure that _currentPos is still inside the bounds
  checkBounds(old_pos);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::handleMouseMoved(int x, int y, int button)
{
  // Do nothing if there are less items than fit on one page
  if(_numEntries <= _entriesPerPage)
    return;

  if(_draggingPart == kSliderPart)
  {
    int old_pos = _currentPos;
    _sliderPos = y - _sliderDeltaMouseDownPos;

    if(_sliderPos < UP_DOWN_BOX_HEIGHT)
      _sliderPos = UP_DOWN_BOX_HEIGHT;

    if(_sliderPos > _h - UP_DOWN_BOX_HEIGHT - _sliderHeight)
      _sliderPos = _h - UP_DOWN_BOX_HEIGHT - _sliderHeight;

    _currentPos = (_sliderPos - UP_DOWN_BOX_HEIGHT) * (_numEntries - _entriesPerPage) /
                  (_h - 2 * UP_DOWN_BOX_HEIGHT - _sliderHeight);
    checkBounds(old_pos);
  }
  else
  {
    int old_part = _part;

    if(y <= UP_DOWN_BOX_HEIGHT)   // Up arrow
      _part = kUpArrowPart;
    else if(y >= _h - UP_DOWN_BOX_HEIGHT)	// Down arrow
      _part = kDownArrowPart;
    else if(y < _sliderPos)
      _part = kPageUpPart;
    else if(y >= _sliderPos + _sliderHeight)
      _part = kPageDownPart;
    else
      _part = kSliderPart;

    if (old_part != _part)
    {
      setDirty(); draw();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ScrollBarWidget::handleMouseClicks(int x, int y, int button)
{
  // Let continuous mouse clicks come through, as the scroll buttons need them
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::checkBounds(int old_pos)
{
  if(_numEntries <= _entriesPerPage || _currentPos < 0)
    _currentPos = 0;
  else if(_currentPos > _numEntries - _entriesPerPage)
    _currentPos = _numEntries - _entriesPerPage;

  if (old_pos != _currentPos)
  {
    recalc();  // This takes care of the required refresh
    setDirty(); draw();
    sendCommand(kSetPositionCmd, _currentPos, _id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::handleMouseEntered(int button)
{
  setFlags(WIDGET_HILITED);
  setDirty(); draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::handleMouseLeft(int button)
{
  _part = kNoPart;
  clearFlags(WIDGET_HILITED);
  setDirty(); draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::recalc()
{
//cerr << "ScrollBarWidget::recalc()\n";
  if(_numEntries > _entriesPerPage)
  {
    _sliderHeight = (_h - 2 * UP_DOWN_BOX_HEIGHT) * _entriesPerPage / _numEntries;
    if(_sliderHeight < UP_DOWN_BOX_HEIGHT)
      _sliderHeight = UP_DOWN_BOX_HEIGHT;

    _sliderPos = UP_DOWN_BOX_HEIGHT + (_h - 2 * UP_DOWN_BOX_HEIGHT - _sliderHeight) *
                 _currentPos / (_numEntries - _entriesPerPage);
    if(_sliderPos < 0)
      _sliderPos = 0;
  }
  else
  {
    _sliderHeight = _h - 2 * UP_DOWN_BOX_HEIGHT;
    _sliderPos = UP_DOWN_BOX_HEIGHT;
  }

  setDirty(); draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::drawWidget(bool hilite)
{
//cerr << "ScrollBarWidget::drawWidget\n";
  FBSurface& s = _boss->dialog().surface();
  int bottomY = _y + _h;
  bool isSinglePage = (_numEntries <= _entriesPerPage);

  s.frameRect(_x, _y, _w, _h, kShadowColor);

  if(_draggingPart != kNoPart)
    _part = _draggingPart;

  // Up arrow
  s.frameRect(_x, _y, _w, UP_DOWN_BOX_HEIGHT, kColor);
  s.drawBitmap(up_arrow, _x+3, _y+5, isSinglePage ? kColor :
               (hilite && _part == kUpArrowPart) ? kScrollColorHi : kScrollColor, 8);

  // Down arrow
  s.frameRect(_x, bottomY - UP_DOWN_BOX_HEIGHT, _w, UP_DOWN_BOX_HEIGHT, kColor);
  s.drawBitmap(down_arrow, _x+3, bottomY - UP_DOWN_BOX_HEIGHT + 5, isSinglePage ? kColor :
               (hilite && _part == kDownArrowPart) ? kScrollColorHi : kScrollColor, 8);

  // Slider
  if(!isSinglePage)
  {
    s.fillRect(_x, _y + _sliderPos, _w, _sliderHeight,
              (hilite && _part == kSliderPart) ? kScrollColorHi : kScrollColor);
    s.frameRect(_x, _y + _sliderPos, _w, _sliderHeight, kColor);
    int y = _y + _sliderPos + _sliderHeight / 2;
    s.hLine(_x + 2, y - 2, _x + _w - 3, kWidColor);
    s.hLine(_x + 2, y,     _x + _w - 3, kWidColor);
    s.hLine(_x + 2, y + 2, _x + _w - 3, kWidColor);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int ScrollBarWidget::_WHEEL_LINES = 4;
