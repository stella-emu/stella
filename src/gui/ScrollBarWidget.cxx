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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "OSystem.hxx"
#include "Dialog.hxx"
#include "FBSurface.hxx"
#include "ScrollBarWidget.hxx"
#include "bspf.hxx"

/*
 * TODO:
 * - Allow for a horizontal scrollbar, too?
 * - If there are less items than fit on one pages, no scrolling can be done
 *   and we thus should not highlight the arrows/slider.
 */

#define UP_DOWN_BOX_HEIGHT	18

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ScrollBarWidget::ScrollBarWidget(GuiObject* boss, const GUI::Font& font,
                                 int x, int y, int w, int h)
  : Widget(boss, font, x, y, w, h), CommandSender(boss)
{
  _flags = Widget::FLAG_ENABLED | Widget::FLAG_TRACK_MOUSE | Widget::FLAG_CLEARBG;
  _bgcolor = kWidColor;
  _bgcolorhi = kWidColor;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::handleMouseDown(int x, int y, MouseButton b,
                                      int clickCount)
{
  // Ignore subsequent mouse clicks when the slider is being moved
  if(_draggingPart == Part::Slider)
    return;

  int old_pos = _currentPos;

  // Do nothing if there are less items than fit on one page
  if(_numEntries <= _entriesPerPage)
    return;

  if (y <= UP_DOWN_BOX_HEIGHT)
  {
    // Up arrow
    _currentPos--;
    _draggingPart = Part::UpArrow;
  }
  else if(y >= _h - UP_DOWN_BOX_HEIGHT)
  {
    // Down arrow
    _currentPos++;
    _draggingPart = Part::DownArrow;
  }
  else if(y < _sliderPos)
  {
    _currentPos -= _entriesPerPage - 1;
  }
  else if(y >= _sliderPos + _sliderHeight)
  {
    _currentPos += _entriesPerPage - 1;
  }
  else
  {
    _draggingPart = Part::Slider;
    _sliderDeltaMouseDownPos = y - _sliderPos;
  }

  // Make sure that _currentPos is still inside the bounds
  checkBounds(old_pos);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::handleMouseUp(int x, int y, MouseButton b,
                                    int clickCount)
{
  _draggingPart = Part::None;
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
void ScrollBarWidget::handleMouseMoved(int x, int y)
{
  // Do nothing if there are less items than fit on one page
  if(_numEntries <= _entriesPerPage)
    return;

  if(_draggingPart == Part::Slider)
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
    Part old_part = _part;

    if(y <= UP_DOWN_BOX_HEIGHT)   // Up arrow
      _part = Part::UpArrow;
    else if(y >= _h - UP_DOWN_BOX_HEIGHT)	// Down arrow
      _part = Part::DownArrow;
    else if(y < _sliderPos)
      _part = Part::PageUp;
    else if(y >= _sliderPos + _sliderHeight)
      _part = Part::PageDown;
    else
      _part = Part::Slider;

    if (old_part != _part)
      setDirty();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ScrollBarWidget::handleMouseClicks(int x, int y, MouseButton b)
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
    recalc();
    sendCommand(GuiObject::kSetPositionCmd, _currentPos, _id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::handleMouseEntered()
{
  setFlags(Widget::FLAG_HILITED);
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::handleMouseLeft()
{
  _part = Part::None;
  clearFlags(Widget::FLAG_HILITED);
  setDirty();
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

  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::drawWidget(bool hilite)
{
  // Up arrow
  static constexpr std::array<uInt32, 8> up_arrow = {
    0b00000000,
    0b00010000,
    0b00111000,
    0b01111100,
    0b11101110,
    0b11000110,
    0b10000010,
    0b00000000
  };

  // Down arrow
  static constexpr std::array<uInt32, 8> down_arrow = {
    0b00000000,
    0b10000010,
    0b11000110,
    0b11101110,
    0b01111100,
    0b00111000,
    0b00010000,
    0b00000000
  };

//cerr << "ScrollBarWidget::drawWidget\n";
  FBSurface& s = _boss->dialog().surface();
  bool onTop = _boss->dialog().isOnTop();
  int bottomY = _y + _h;
  bool isSinglePage = (_numEntries <= _entriesPerPage);

  s.frameRect(_x, _y, _w, _h, hilite ? kWidColorHi : kColor);

  if(_draggingPart != Part::None)
    _part = _draggingPart;

  // Up arrow
  if(hilite && _part == Part::UpArrow)
    s.fillRect(_x + 1, _y + 1, _w - 2, UP_DOWN_BOX_HEIGHT - 2, kScrollColor);
  s.drawBitmap(up_arrow.data(), _x+4, _y+5,
               onTop ? isSinglePage ? kColor : (hilite && _part == Part::UpArrow) ? kWidColor
               : kTextColor : kColor, 8);

  // Down arrow
  if(hilite && _part == Part::DownArrow)
    s.fillRect(_x + 1, bottomY - UP_DOWN_BOX_HEIGHT + 1, _w - 2, UP_DOWN_BOX_HEIGHT - 2, kScrollColor);
  s.drawBitmap(down_arrow.data(), _x+4, bottomY - UP_DOWN_BOX_HEIGHT + 5,
               onTop ? isSinglePage ? kColor : (hilite && _part == Part::DownArrow) ?
               kWidColor : kTextColor : kColor, 8);

  // Slider
  if(!isSinglePage)
  {
    s.fillRect(_x + 1, _y + _sliderPos - 1, _w - 2, _sliderHeight + 2,
              onTop ? (hilite && _part == Part::Slider) ? kScrollColorHi : kScrollColor : kColor);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int ScrollBarWidget::_WHEEL_LINES = 4;
