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
// $Id: Widget.cxx,v 1.7 2005-04-04 02:19:22 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "StellaFont.hxx"
#include "Dialog.hxx"
#include "Command.hxx"
#include "GuiObject.hxx"
#include "bspf.hxx"
#include "GuiUtils.hxx"
#include "Widget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget::Widget(GuiObject* boss, Int32 x, Int32 y, Int32 w, Int32 h)
    : GuiObject(boss->instance(), x, y, w, h),
      _type(0),
      _boss(boss),
      _id(0),
      _flags(0),
      _hasFocus(false)
{
  // Insert into the widget list of the boss
  _next = _boss->_firstWidget;
  _boss->_firstWidget = this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget::~Widget()
{
  delete _next;
  _next = NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::draw()
{
  FrameBuffer& fb = _boss->instance()->frameBuffer();

  if(!isVisible() || !_boss->isVisible())
    return;

  Int32 oldX = _x, oldY = _y;

  // Account for our relative position in the dialog
  _x = getAbsX();
  _y = getAbsY();

  // Clear background (unless alpha blending is enabled)
  if(_flags & WIDGET_CLEARBG)
    fb.fillRect(_x, _y, _w, _h, kBGColor);

  // Draw border
  if(_flags & WIDGET_BORDER) {
    OverlayColor colorA = kColor;
    OverlayColor colorB = kShadowColor;
    if((_flags & WIDGET_INV_BORDER) == WIDGET_INV_BORDER)
      SWAP(colorA, colorB);
    fb.box(_x, _y, _w, _h, colorA, colorB);
    _x += 4;
    _y += 4;
    _w -= 8;
    _h -= 8;
  }

  // Now perform the actual widget draw
  drawWidget((_flags & WIDGET_HILITED) ? true : false);

  // Restore x/y
  if (_flags & WIDGET_BORDER) {
    _x -= 4;
    _y -= 4;
    _w += 8;
    _h += 8;
  }

  _x = oldX;
  _y = oldY;

  // Draw all children
  Widget* w = _firstWidget;
  while(w)
  {
    w->draw();
    w = w->_next;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget* Widget::findWidgetInChain(Widget *w, Int32 x, Int32 y)
{
  while(w)
  {
    // Stop as soon as we find a widget that contains the point (x,y)
    if(x >= w->_x && x < w->_x + w->_w && y >= w->_y && y < w->_y + w->_h)
      break;
    w = w->_next;
  }

  if(w)
    w = w->findWidget(x - w->_x, y - w->_y);

  return w;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StaticTextWidget::StaticTextWidget(GuiObject *boss, Int32 x, Int32 y, Int32 w, Int32 h,
                                   const string& text, TextAlignment align)
    : Widget(boss, x, y, w, h),
      _align(align)
{
  _flags = WIDGET_ENABLED;
  _type = kStaticTextWidget;
  setLabel(text);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaticTextWidget::setValue(Int32 value)
{
  char buf[256];
  sprintf(buf, "%d", value);
  _label = buf;

  // Refresh the screen when the text has changed
  // FIXME - eventually, this should be a dirty rectangle
  _boss->instance()->frameBuffer().refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaticTextWidget::drawWidget(bool hilite)
{
  FrameBuffer& fb = _boss->instance()->frameBuffer();
  fb.font().drawString(_label, _x, _y, _w,
                       isEnabled() ? kTextColor : kColor, _align);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ButtonWidget::ButtonWidget(GuiObject *boss, Int32 x, Int32 y, Int32 w, Int32 h,
                           const string& label, Int32 cmd, uInt8 hotkey)
    : StaticTextWidget(boss, x, y, w, h, label, kTextAlignCenter),
      CommandSender(boss),
	  _cmd(cmd),
      _hotkey(hotkey)
{
  _flags = WIDGET_ENABLED | WIDGET_BORDER | WIDGET_CLEARBG;
  _type = kButtonWidget;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::handleMouseUp(Int32 x, Int32 y, Int32 button, Int32 clickCount)
{
  if(isEnabled() && x >= 0 && x < _w && y >= 0 && y < _h)
    sendCommand(_cmd, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::drawWidget(bool hilite)
{
  FrameBuffer& fb = _boss->instance()->frameBuffer();
  fb.font().drawString(_label, _x, _y + (_h - kLineHeight)/2 + 1, _w,
                       !isEnabled() ? kColor :
                       hilite ? kTextColorHi : kTextColor, _align);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/* 8x8 checkbox bitmap */
static uInt32 checked_img[8] =
{
	0x00000000,
	0x01000010,
	0x00100100,
	0x00011000,
	0x00011000,
	0x00100100,
	0x01000010,
	0x00000000,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheckboxWidget::CheckboxWidget(GuiObject *boss, Int32 x, Int32 y, Int32 w, Int32 h,
                               const string& label, Int32 cmd, uInt8 hotkey)
    : ButtonWidget(boss, x, y, w, h, label, cmd, hotkey),
      _state(false)
{
  _flags = WIDGET_ENABLED;
  _type = kCheckboxWidget;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::handleMouseUp(Int32 x, Int32 y, Int32 button, Int32 clickCount)
{
  if(isEnabled() && x >= 0 && x < _w && y >= 0 && y < _h)
    toggleState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::setState(bool state)
{
  if(_state != state)
  {
    _state = state;
    _flags ^= WIDGET_INV_BORDER;
    draw();
  }
  sendCommand(_cmd, _state);

  // Refresh the screen after the checkbox is drawn
  // FIXME - eventually, this should be a dirty rectangle
  _boss->instance()->frameBuffer().refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::drawWidget(bool hilite)
{
  FrameBuffer& fb = _boss->instance()->frameBuffer();

  // Draw the box
  fb.box(_x, _y, 14, 14, kColor, kShadowColor);

  // If checked, draw cross inside the box
  if(_state)
    fb.drawBitmap(checked_img, _x + 3, _y + 3,
                  isEnabled() ? kTextColor : kColor);
  else
    fb.fillRect(_x + 2, _y + 2, 10, 10, kBGColor);

  // Finally draw the label
  fb.font().drawString(_label, _x + 20, _y + 3, _w,
                       isEnabled() ? kTextColor : kColor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SliderWidget::SliderWidget(GuiObject *boss, Int32 x, Int32 y, Int32 w, Int32 h,
                           const string& label, Int32 labelWidth, Int32 cmd, uInt8 hotkey)
  : ButtonWidget(boss, x, y, w, h, label, cmd, hotkey),
    _value(0),
    _oldValue(0),
    _valueMin(0),
    _valueMax(100),
    _isDragging(false),
    _labelWidth(labelWidth)
{
  _flags = WIDGET_ENABLED | WIDGET_TRACK_MOUSE | WIDGET_CLEARBG;
  _type = kSliderWidget;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::handleMouseMoved(Int32 x, Int32 y, Int32 button)
{
  // TODO: when the mouse is dragged outside the widget, the slider should
  // snap back to the old value.
  if(isEnabled() && _isDragging && x >= (int)_labelWidth)
  {
    Int32 newValue = posToValue(x - _labelWidth);

    if(newValue < _valueMin)
      newValue = _valueMin;
    else if (newValue > _valueMax)
      newValue = _valueMax;

    if(newValue != _value)
    {
      _value = newValue; 
      draw();
      sendCommand(_cmd, _value);
    }
    // Refresh the screen while the slider is being redrawn
    // FIXME - eventually, this should be a dirty rectangle
    _boss->instance()->frameBuffer().refresh();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::handleMouseDown(Int32 x, Int32 y, Int32 button, Int32 clickCount)
{
  if(isEnabled())
  {
    _isDragging = true;
    handleMouseMoved(x, y, button);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::handleMouseUp(Int32 x, Int32 y, Int32 button, Int32 clickCount)
{
  if(isEnabled() && _isDragging)
    sendCommand(_cmd, _value);

  _isDragging = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::drawWidget(bool hilite)
{
  FrameBuffer& fb = _boss->instance()->frameBuffer();

  // Draw the label, if any
  if(_labelWidth > 0)
    fb.font().drawString(_label, _x, _y + 2, _labelWidth,
                         isEnabled() ? kTextColor : kColor, kTextAlignRight);

  // Draw the box
  fb.box(_x + _labelWidth, _y, _w - _labelWidth, _h, kColor, kShadowColor);

  // Draw the 'bar'
  fb.fillRect(_x + _labelWidth + 2, _y + 2, valueToPos(_value), _h - 4,
              !isEnabled() ? kColor :
              hilite ? kTextColorHi : kTextColor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 SliderWidget::valueToPos(Int32 value)
{
  return ((_w - _labelWidth - 4) * (value - _valueMin) / (_valueMax - _valueMin));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 SliderWidget::posToValue(Int32 pos)
{
  return (pos) * (_valueMax - _valueMin) / (_w - _labelWidth - 4) + _valueMin;
}
