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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Dialog.hxx"
#include "FBSurface.hxx"
#include "ScrollBarWidget.hxx"
#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ScrollBarWidget::ScrollBarWidget(GuiObject* boss, const GUI::Font& font)
  : Widget(boss, font),
    CommandSender(boss),
    _scrollBarWidth{scrollBarWidth(font)}
{
  // My width is my own business -- it follows the font.  My position and height
  // are the list's, which sets them whenever it moves or resizes
  _w = _scrollBarWidth;

  _flags = Widget::Flag::Enabled | Widget::Flag::TrackMouse | Widget::Flag::ClearBG;
  _bgcolor = kWidColor;
  _bgcolorhi = kWidColor;

  setArrows();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::setArrows()
{
  // The arrow is sized from the font, and everything around it follows from
  // the arrow -- so the scroll bar scales instead of stepping at one font size.
  // At the default 9x18 this reproduces the old 7x6 arrow in an 18px box
  _arrowWidth = arrowWidth(_font);
  _arrowHeight = _arrowWidth - 1;
  _arrowThickness = (_arrowWidth / 3) + 1;
  _upDownBoxHeight = _arrowHeight * 3;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::refreshFont()
{
  Widget::refreshFont();

  // All of these are font-dependent, so recompute them for the new font.  The
  // bar width is intrinsic to the scrollbar (the list positions and sets our
  // height, but the width follows the font)
  _scrollBarWidth = scrollBarWidth(_font);
  setArrows();
  Widget::setWidth(_scrollBarWidth);
  recalc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::handleMouseDown(int x, int y, MouseButton b,
                                      int clickCount)
{
  // Ignore subsequent mouse clicks when the slider is being moved
  if(_draggingPart == Part::Slider)
    return;

  const int old_pos = _currentPos;

  // Do nothing if there are less items than fit on one page
  if(_numEntries <= _entriesPerPage)
    return;

  if(y <= _upDownBoxHeight)
  {
    // Up arrow
    _currentPos--;
    _draggingPart = Part::UpArrow;
  }
  else if(y >= _h - _upDownBoxHeight)
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
  const int old_pos = _currentPos;

  if(_numEntries < _entriesPerPage)
    return;

  if(direction < 0)
    _currentPos -= _wheel_lines ? _wheel_lines : S_WHEEL_LINES;
  else
    _currentPos += _wheel_lines ? _wheel_lines : S_WHEEL_LINES;

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
    _sliderPos = BSPF::clamp(y - _sliderDeltaMouseDownPos,
        _upDownBoxHeight, _h - _upDownBoxHeight - _sliderHeight);

    const int old_pos = _currentPos;
    _currentPos = (_sliderPos - _upDownBoxHeight) * (_numEntries - _entriesPerPage) /
                  (_h - 2 * _upDownBoxHeight - _sliderHeight);
    checkBounds(old_pos);
  }
  else
  {
    const Part old_part = _part;

    if(y <= _upDownBoxHeight)   // Up arrow
      _part = Part::UpArrow;
    else if(y >= _h - _upDownBoxHeight)	// Down arrow
      _part = Part::DownArrow;
    else if(y < _sliderPos)
      _part = Part::PageUp;
    else if(y >= _sliderPos + _sliderHeight)
      _part = Part::PageDown;
    else
      _part = Part::Slider;

    if(old_part != _part)
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

  if(old_pos != _currentPos)
  {
    recalc();
    setDirty();
    sendCommand(GuiObject::Cmd::SetPosition, _currentPos, _id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::handleMouseLeft()
{
  _part = Part::None;
  Widget::handleMouseLeft();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::recalc()
{
  const int oldSliderHeight = _sliderHeight,
            oldSliderPos = _sliderPos;

  if(_numEntries > _entriesPerPage)
  {
    _sliderHeight = std::max(_upDownBoxHeight,
        (_h - 2 * _upDownBoxHeight) * _entriesPerPage / _numEntries);

    _sliderPos = std::max(0,
      _upDownBoxHeight + (_h - 2 * _upDownBoxHeight - _sliderHeight) *
      _currentPos / (_numEntries - _entriesPerPage));
  }
  else
  {
    _sliderHeight = _h - 2 * _upDownBoxHeight;
    _sliderPos = _upDownBoxHeight;
  }

  if(oldSliderHeight != _sliderHeight || oldSliderPos != _sliderPos)
    setDirty(); // only set dirty when something changed
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScrollBarWidget::drawWidget(bool hilite)
{
  FBSurface& s = _boss->dialog().surface();
  const int bottomY = _y + _h;
  const bool isSinglePage = (_numEntries <= _entriesPerPage);

  s.frameRect(_x, _y, _w, _h, hilite ? kWidColorHi : kColor);

  if(_draggingPart != Part::None)
    _part = _draggingPart;

  // Up arrow
  if(hilite && _part == Part::UpArrow)
    s.fillRect(_x + 1, _y + 1, _w - 2, _upDownBoxHeight - 2, kScrollColor);
  s.drawArrow(_x + (_scrollBarWidth - _arrowWidth) / 2,
              _y + (_upDownBoxHeight - _arrowHeight) / 2,
              _arrowWidth, _arrowHeight, ArrowDirection::Up,
              isSinglePage ? kColor
                           : (hilite && _part == Part::UpArrow) ? kWidColor : kTextColor,
              _arrowThickness);

  // Down arrow
  if(hilite && _part == Part::DownArrow)
    s.fillRect(_x + 1, bottomY - _upDownBoxHeight + 1, _w - 2, _upDownBoxHeight - 2, kScrollColor);
  s.drawArrow(_x + (_scrollBarWidth - _arrowWidth) / 2,
              bottomY - _upDownBoxHeight + (_upDownBoxHeight - _arrowHeight) / 2,
              _arrowWidth, _arrowHeight, ArrowDirection::Down,
              isSinglePage ? kColor
                           : (hilite && _part == Part::DownArrow) ? kWidColor : kTextColor,
              _arrowThickness);

  // Slider
  if(!isSinglePage)
  {
    // align slider to scroll intervals
    const int alignedPos = std::max(0,
      _upDownBoxHeight + (_h - 2 * _upDownBoxHeight - _sliderHeight) *
      _currentPos / (_numEntries - _entriesPerPage));

    s.fillRect(_x + 1, _y + alignedPos - 1, _w - 2, _sliderHeight + 2,
              (hilite && _part == Part::Slider) ? kScrollColorHi : kScrollColor);
  }
  clearDirty();
}
