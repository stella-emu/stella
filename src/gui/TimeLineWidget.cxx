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
                               const string& label, int labelWidth, int cmd)
  : ButtonWidget(boss, font, x, y, w, h, label, cmd),
    _value(0),
    _stepValue(1),
    _valueMin(0),
    _valueMax(100),
    _isDragging(false),
    _labelWidth(labelWidth)
{
  _flags = WIDGET_ENABLED | WIDGET_TRACK_MOUSE;
  _bgcolor = kDlgColor;
  _bgcolorhi = kDlgColor;

  if(!_label.empty() && _labelWidth == 0)
    _labelWidth = _font.getStringWidth(_label);

  _w = w + _labelWidth;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeLineWidget::setValue(int value)
{
  if(value < _valueMin)      value = _valueMin;
  else if(value > _valueMax) value = _valueMax;

  if(value != _value)
  {
    _value = value;
    setDirty();
    sendCommand(_cmd, _value, _id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeLineWidget::setMinValue(int value)
{
  _valueMin = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeLineWidget::setMaxValue(int value)
{
  _valueMax = value;
}

#if 0
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeLineWidget::setStepValue(int value)
{
  _stepValue = value;
}
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeLineWidget::handleMouseMoved(int x, int y)
{
  // TODO: when the mouse is dragged outside the widget, the slider should
  // snap back to the old value.
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
    if(direction < 0)
      handleEvent(Event::UIUp);
    else if(direction > 0)
      handleEvent(Event::UIDown);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TimeLineWidget::handleEvent(Event::Type e)
{
  if(!isEnabled())
    return false;

  switch(e)
  {
    case Event::UIDown:
    case Event::UILeft:
    case Event::UIPgDown:
      setValue(_value - _stepValue);
      break;

    case Event::UIUp:
    case Event::UIRight:
    case Event::UIPgUp:
      setValue(_value + _stepValue);
      break;

    case Event::UIHome:
      setValue(_valueMin);
      break;

    case Event::UIEnd:
      setValue(_valueMax);
      break;

    default:
      return false;
  }
  return true;
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
  s.box(_x + _labelWidth, _y, _w - _labelWidth, _h, kColor, kShadowColor);
  // Fill the box
  s.fillRect(_x + _labelWidth + 2, _y + 2, _w - _labelWidth - 4, _h - 4,
             !isEnabled() ? kBGColorHi : kWidColor);
  // Draw the 'bar'
  s.fillRect(_x + _labelWidth + 2, _y + 2, valueToPos(_value), _h - 4,
             !isEnabled() ? kColor : hilite ? kSliderColorHi : kSliderColor);
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
int TimeLineWidget::valueToPos(int value)
{
  if(value < _valueMin)      value = _valueMin;
  else if(value > _valueMax) value = _valueMax;
  int range = std::max(_valueMax - _valueMin, 1);  // don't divide by zero

  return ((_w - _labelWidth - 4) * (value - _valueMin) / range);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TimeLineWidget::posToValue(int pos)
{
  int value = (pos) * (_valueMax - _valueMin) / (_w - _labelWidth - 4) + _valueMin;

  // Scale the position to the correct interval (according to step value)
  return value - (value % _stepValue);
}
