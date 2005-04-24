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
// $Id: ScrollBarWidget.cxx,v 1.2 2005-04-24 20:36:36 stephena Exp $
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
 * - Auto-repeat: if user clicks & holds on one of the arrows, then after a
 *   brief delay, it should start to contiously scroll
 * - Allow for a horizontal scrollbar, too?
 * - If there are less items than fit on one pages, no scrolling can be done
 *   and we thus should not highlight the arrows/slider.
 * - Allow the mouse wheel to scroll more than one line at a time
 */

#define UP_DOWN_BOX_HEIGHT	10

// Up arrow
static uInt32 up_arrow[8] = {
  0x00000000,
  0x00000000,
  0x00001000,
  0x00001000,
  0x00011100,
  0x00011100,
  0x00110110,
  0x00100010,
};

// Down arrow
static uInt32 down_arrow[8] = {
  0x00000000,
  0x00000000,
  0x00100010,
  0x00110110,
  0x00011100,
  0x00011100,
  0x00001000,
  0x00001000,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ScrollBarWidget::ScrollBarWidget(GuiObject* boss, Int32 x, Int32 y, Int32 w, Int32 h)
    : Widget(boss, x, y, w, h), CommandSender(boss)
{
  _flags = WIDGET_ENABLED | WIDGET_TRACK_MOUSE | WIDGET_CLEARBG;
  _type = kScrollBarWidget;

  _part = kNoPart;
  _sliderHeight = 0;
  _sliderPos = 0;

  _draggingPart = kNoPart;
  _sliderDeltaMouseDownPos = 0;

  _numEntries = 0;
  _entriesPerPage = 0;
  _currentPos = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::handleMouseDown(Int32 x, Int32 y, Int32 button,
                                      Int32 clickCount)
{
  Int32 old_pos = _currentPos;

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

  _boss->instance()->frameBuffer().refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::handleMouseUp(Int32 x, Int32 y, Int32 button,
                                    Int32 clickCount)
{
  _draggingPart = kNoPart;
  _boss->instance()->frameBuffer().refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::handleMouseWheel(Int32 x, Int32 y, Int32 direction)
{
  Int32 old_pos = _currentPos;

  if(_numEntries < _entriesPerPage)
    return;

  if(direction < 0)
    _currentPos--;
  else
    _currentPos++;

  // Make sure that _currentPos is still inside the bounds
  checkBounds(old_pos);
  _boss->instance()->frameBuffer().refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::handleMouseMoved(Int32 x, Int32 y, Int32 button)
{
  // Do nothing if there are less items than fit on one page
  if(_numEntries <= _entriesPerPage)
    return;

  if(_draggingPart == kSliderPart)
  {
    Int32 old_pos = _currentPos;
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
    Int32 old_part = _part;

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
      draw();
  }
  _boss->instance()->frameBuffer().refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::handleTickle() {
/*
	// FIXME/TODO - this code is supposed to allow for "click-repeat" (like key repeat),
	// i.e. if you click on one of the arrows and keep clicked, it will scroll
	// continuously. However, just like key repeat, this requires two delays:
	// First an "initial" delay that has to pass before repeating starts (otherwise
	// it is near to impossible to achieve single clicks). Secondly, a repeat delay
	// that determines how often per second a click is simulated.
	int old_pos = _currentPos;

	if (_draggingPart == kUpArrowPart)
		_currentPos--;
	else if (_draggingPart == kDownArrowPart)
		_currentPos++;

	// Make sure that _currentPos is still inside the bounds
	checkBounds(old_pos);
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::checkBounds(Int32 old_pos)
{
  if(_numEntries <= _entriesPerPage || _currentPos < 0)
    _currentPos = 0;
  else if(_currentPos > _numEntries - _entriesPerPage)
    _currentPos = _numEntries - _entriesPerPage;

  if (old_pos != _currentPos)
  {
    recalc();
    draw();
    sendCommand(kSetPositionCmd, _currentPos);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::recalc()
{
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::drawWidget(bool hilite)
{
  FrameBuffer& fb = _boss->instance()->frameBuffer();
  int bottomY = _y + _h;
  bool isSinglePage = (_numEntries <= _entriesPerPage);

  fb.frameRect(_x, _y, _w, _h, kShadowColor);

  if(_draggingPart != kNoPart)
    _part = _draggingPart;

  // Up arrow
  fb.frameRect(_x, _y, _w, UP_DOWN_BOX_HEIGHT, kColor);
  fb.drawBitmap(up_arrow, _x, _y,
                isSinglePage ? kColor :
                (hilite && _part == kUpArrowPart) ? kTextColorHi : kTextColor);

  // Down arrow
  fb.frameRect(_x, bottomY - UP_DOWN_BOX_HEIGHT, _w, UP_DOWN_BOX_HEIGHT, kColor);
  fb.drawBitmap(down_arrow, _x, bottomY - UP_DOWN_BOX_HEIGHT,
                isSinglePage ? kColor :
                (hilite && _part == kDownArrowPart) ? kTextColorHi : kTextColor);

  // Slider
  if(!isSinglePage)
  {
    fb.fillRect(_x, _y + _sliderPos, _w, _sliderHeight,
               (hilite && _part == kSliderPart) ? kTextColorHi : kTextColor);
    fb.frameRect(_x, _y + _sliderPos, _w, _sliderHeight, kColor);
    int y = _y + _sliderPos + _sliderHeight / 2;
    OverlayColor color = (hilite && _part == kSliderPart) ? kColor : kShadowColor;
    fb.hLine(_x + 2, y - 2, _x + _w - 3, color);
    fb.hLine(_x + 2, y,     _x + _w - 3, color);
    fb.hLine(_x + 2, y + 2, _x + _w - 3, color);
  }
}
