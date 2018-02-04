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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "bspf.hxx"
#include "Command.hxx"
#include "Dialog.hxx"
#include "Font.hxx"
#include "FBSurface.hxx"
#include "GuiObject.hxx"
#include "OSystem.hxx"

#include "TimeLineWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeLineWidget::TimeLineWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, int w, int h,
                               const string& label, uInt32 labelWidth, int cmd)
  : ButtonWidget(boss, font, x, y, w, h, label, cmd),
    _value(0),
    _valueMin(0),
    _valueMax(0),
    _isDragging(false),
    _labelWidth(labelWidth)
{
  _flags = WIDGET_ENABLED | WIDGET_TRACK_MOUSE;
  _bgcolor = kDlgColor;
  _bgcolorhi = kDlgColor;

  if(!_label.empty() && _labelWidth == 0)
    _labelWidth = _font.getStringWidth(_label);

  _w = w + _labelWidth;

  _stepValue.reserve(100);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeLineWidget::setValue(uInt32 value)
{
  value = BSPF::clamp(value, _valueMin, _valueMax);

  if(value != _value)
  {
    _value = value;
    setDirty();
    sendCommand(_cmd, _value, _id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeLineWidget::setMinValue(uInt32 value)
{
  _valueMin = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeLineWidget::setMaxValue(uInt32 value)
{
  _valueMax = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeLineWidget::setStepValues(const IntArray& steps)
{
  _stepValue.clear();

  // If no steps are defined, just use the maximum value
  if(steps.size() > 0)
  {
    // Try to allocate as infrequently as possible
    if(steps.size() > _stepValue.capacity())
      _stepValue.reserve(2 * steps.size());

    double scale = (_w - _labelWidth - 2) / double(steps.back());

    // Skip the very last value; we take care of it outside the end of the loop
    for(uInt32 i = 0; i < steps.size() - 1; ++i)
      _stepValue.push_back(int(steps[i] * scale));

    // Due to integer <-> double conversion, the last value is sometimes
    // slightly less than the maximum value; we assign it manually to fix this
    _stepValue.push_back(_w - _labelWidth - 2);
  }
  else
    _stepValue.push_back(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeLineWidget::handleMouseMoved(int x, int y)
{
  if(isEnabled() && _isDragging && x >= int(_labelWidth))
    setValue(posToValue(x - _labelWidth));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeLineWidget::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  if(isEnabled() && b == MouseButton::LEFT)
  {
    _isDragging = true;
    handleMouseMoved(x, y);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeLineWidget::handleMouseUp(int x, int y, MouseButton b, int clickCount)
{
  if(isEnabled() && _isDragging)
    sendCommand(_cmd, _value, _id);

  _isDragging = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeLineWidget::handleMouseWheel(int x, int y, int direction)
{
  if(isEnabled())
  {
    if(direction < 0 && _value < _valueMax)
      setValue(_value + 1);
    else if(direction > 0 && _value > _valueMin)
      setValue(_value - 1);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeLineWidget::drawWidget(bool hilite)
{
  FBSurface& s = _boss->dialog().surface();

#ifndef FLAT_UI
  // Draw the label, if any
  if(_labelWidth > 0)
    s.drawString(_font, _label, _x, _y + 2, _labelWidth,
                 isEnabled() ? kTextColor : kColor, TextAlign::Right);

  // Draw the box
  s.frameRect(_x + _labelWidth, _y, _w - _labelWidth, _h, kColor);
  // Fill the box
  s.fillRect(_x + _labelWidth + 1, _y + 1, _w - _labelWidth - 2, _h - 2,
             !isEnabled() ? kBGColorHi : kWidColor);
  // Draw the 'bar'
  int vp = valueToPos(_value);
  s.fillRect(_x + _labelWidth + 1, _y + 1, vp, _h - 2,
             !isEnabled() ? kColor : hilite ? kSliderColorHi : kSliderColor);

  // add 4 tickmarks for 5 intervals
  int numTicks = std::min(5, int(_stepValue.size()));
  for(int i = 1; i < numTicks; ++i)
  {
    int idx = int((_stepValue.size() * i + numTicks / 2) / numTicks);
    if(idx > 1)
    {
      int tp = valueToPos(idx - 1);
      s.vLine(_x + _labelWidth + tp, _y + _h / 2, _y + _h - 2, tp > vp ? kSliderColor : kWidColor);
    }
  }
#else
  // Draw the label, if any
  if(_labelWidth > 0)
    s.drawString(_font, _label, _x, _y + 2, _labelWidth,
                 isEnabled() ? kTextColor : kColor, TextAlign::Left);

  // Draw the box
  s.frameRect(_x + _labelWidth, _y, _w - _labelWidth, _h, isEnabled() && hilite ? kSliderColorHi : kShadowColor);
  // Fill the box
  s.fillRect(_x + _labelWidth + 1, _y + 1, _w - _labelWidth - 2, _h - 2,
             !isEnabled() ? kBGColorHi : kWidColor);
  // Draw the 'bar'
  s.fillRect(_x + _labelWidth + 2, _y + 2, valueToPos(_value), _h - 4,
             !isEnabled() ? kColor : hilite ? kSliderColorHi : kSliderColor);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TimeLineWidget::valueToPos(uInt32 value)
{
  return _stepValue[BSPF::clamp(value, _valueMin, _valueMax)];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TimeLineWidget::posToValue(uInt32 pos)
{
  // Find the interval in which 'pos' falls, and then the endpoint which
  // it is closest to
  for(uInt32 i = 0; i < _stepValue.size() - 1; ++i)
    if(pos >= _stepValue[i] && pos <= _stepValue[i+1])
      return (_stepValue[i+1] - pos) < (pos - _stepValue[i]) ? i+1 : i;

  return _valueMax;
}
