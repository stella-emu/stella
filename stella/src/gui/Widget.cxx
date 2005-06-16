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
// $Id: Widget.cxx,v 1.18 2005-06-16 22:18:02 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "Font.hxx"
#include "Dialog.hxx"
#include "Command.hxx"
#include "GuiObject.hxx"
#include "bspf.hxx"
#include "GuiUtils.hxx"
#include "Widget.hxx"

//FIXMEstatic int COUNT = 0;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget::Widget(GuiObject* boss, int x, int y, int w, int h)
    : GuiObject(boss->instance(), boss->parent(), x, y, w, h),
      _type(0),
      _boss(boss),
      _id(0),
      _flags(0),
      _hasFocus(false),
      _color(kTextColor),
      _font((GUI::Font*)&(boss->instance()->font()))
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

  int oldX = _x, oldY = _y;

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

  // Indicate if this is the currently active widget
  // by drawing a box around it.
  if((_activeWidget == this) && (_flags & WIDGET_TAB_NAVIGATE))
    fb.frameRect(_x-1, _y-1, _w+2, _h+2, kTextColorEm); // FIXME - maybe chose a better color

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
void Widget::receivedFocus()
{
  if(_hasFocus)
    return;

  _hasFocus = true;
  receivedFocusWidget();

  // Only signal a new active widget if the widget has defined that capability
  // We only care about widgets with WIDGET_TAB_NAVIGATE property
  if(getFlags() & WIDGET_TAB_NAVIGATE)
  {
    _activeWidget = this;
    _boss->handleCommand(NULL, kActiveWidgetCmd, 0);
  }
}
/*  FIXME
void Widget::lostFocus()
{
  _hasFocus = false;
  lostFocusWidget(); 

if(getFlags() & WIDGET_TAB_NAVIGATE)
  cerr << "lost focus: " << this << endl;
}
*/
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget* Widget::findWidgetInChain(Widget *w, int x, int y)
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
bool Widget::isWidgetInChain(Widget* w, Widget* find)
{
  bool found = false;

  while(w)
  {
    // Stop as soon as we find the widget
    if(w == find)
    {
      found = true;
      break;
    }
    w = w->_next;
  }

  return found;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setFocusForChain(Widget* w, Widget* hasFocus)
{
  if(!hasFocus)
    return;

  while(w)
  {
    if(w != hasFocus)
      w->lostFocus();

    w = w->_next;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setPrevInChain(Widget* start, Widget* hasFocus)
{
  if(!start)
    return;

  // We search the array in circular fashion until the 'end' is reached
  Widget* w = hasFocus;
  Widget* active = NULL;

  if(w)  // start from 'hasFocus'
  {
    w = w->_next;
    while(w)
    {
      if(w->getFlags() & WIDGET_TAB_NAVIGATE)
      {
        active = w;
        break;
      }
      w = w->_next;
    }

    // If we haven't found an active widget by now, start searching from
    // the beginning of the list
    if(!active)
    {
      w = start;
      while(w != hasFocus)
      {
        if(w->getFlags() & WIDGET_TAB_NAVIGATE)
        {
          active = w;
          break;
        }
        w = w->_next;
      }
    }
  }
  else  // start from the beginning, since no widget currently has focus
  {
    w = start;
    while(w)
    {
      if(w->getFlags() & WIDGET_TAB_NAVIGATE)
      {
        active = w;
        break;
      }
      w = w->_next;
    }
  }

  // At this point, we *should* have an active widget
  if(active)
    active->receivedFocus();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setNextInChain(Widget* start, Widget* hasFocus)
{
  if(!start)
    return;
// FIXME - get this working
cerr << "--------------------------------\nWidget list:\n";
    Widget* w1 = start;
    while(w1)
    {
      if(w1->getFlags() & WIDGET_TAB_NAVIGATE)
      {
        cerr << w1 << endl;
      }
      w1 = w1->_next;
    }
cerr << "\n--------------------------------\n";


  // We search the array in circular fashion until the 'end' is reached
  Widget* w = hasFocus;
  Widget* active = NULL;

  if(w)  // start from 'hasFocus'
  {
    w = w->_next;
    while(w)
    {
      if(w->getFlags() & WIDGET_TAB_NAVIGATE)
      {
        active = w;
        break;
      }
      w = w->_next;
    }

    // If we haven't found an active widget by now, start searching from
    // the beginning of the list
    if(!active)
    {
      w = start;
      while(w != hasFocus)
      {
        if(w->getFlags() & WIDGET_TAB_NAVIGATE)
        {
          active = w;
          break;
        }
        w = w->_next;
      }
    }
  }
  else  // start from the beginning, since no widget currently has focus
  {
    w = start;
    while(w)
    {
      if(w->getFlags() & WIDGET_TAB_NAVIGATE)
      {
        active = w;
        break;
      }
      w = w->_next;
    }
  }

  // At this point, we *should* have an active widget
  if(active)
    active->receivedFocus();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StaticTextWidget::StaticTextWidget(GuiObject *boss, int x, int y, int w, int h,
                                   const string& text, TextAlignment align)
    : Widget(boss, x, y, w, h),
      _align(align)
{
  _flags = WIDGET_ENABLED;
  _type = kStaticTextWidget;
  setLabel(text);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaticTextWidget::setValue(int value)
{
  char buf[256];
  sprintf(buf, "%d", value);
  _label = buf;

  // Refresh the screen when the text has changed
  // TODO - eventually, this should be a dirty rectangle
  _boss->instance()->frameBuffer().refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaticTextWidget::drawWidget(bool hilite)
{
  FrameBuffer& fb = _boss->instance()->frameBuffer();
  fb.drawString(_font, _label, _x, _y, _w,
                isEnabled() ? _color : kColor, _align);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ButtonWidget::ButtonWidget(GuiObject *boss, int x, int y, int w, int h,
                           const string& label, int cmd, uInt8 hotkey)
    : StaticTextWidget(boss, x, y, w, h, label, kTextAlignCenter),
      CommandSender(boss),
	  _cmd(cmd),
      _hotkey(hotkey)
{
  _flags = WIDGET_ENABLED | WIDGET_BORDER | WIDGET_CLEARBG;
  _type = kButtonWidget;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::handleMouseUp(int x, int y, int button, int clickCount)
{
  if(isEnabled() && x >= 0 && x < _w && y >= 0 && y < _h)
    sendCommand(_cmd, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::drawWidget(bool hilite)
{
  FrameBuffer& fb = _boss->instance()->frameBuffer();
  fb.drawString(_font, _label, _x, _y + (_h - kLineHeight)/2 + 1, _w,
                !isEnabled() ? kColor : hilite ? kTextColorHi : _color, _align);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/* 8x8 checkbox bitmap */
static unsigned int checked_img[8] =
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
CheckboxWidget::CheckboxWidget(GuiObject *boss, int x, int y, int w, int h,
                               const string& label, int cmd, uInt8 hotkey)
    : ButtonWidget(boss, x, y, w, h, label, cmd, hotkey),
      _state(false)
{
  _flags = WIDGET_ENABLED;
  _type = kCheckboxWidget;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::handleMouseUp(int x, int y, int button, int clickCount)
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
  // TODO - eventually, this should be a dirty rectangle
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
                  isEnabled() ? _color : kColor);
  else
    fb.fillRect(_x + 2, _y + 2, 10, 10, kBGColor);

  // Finally draw the label
  fb.drawString(_font, _label, _x + 20, _y + 3, _w, isEnabled() ? _color : kColor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SliderWidget::SliderWidget(GuiObject *boss, int x, int y, int w, int h,
                           const string& label, int labelWidth, int cmd, uInt8 hotkey)
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
void SliderWidget::handleMouseMoved(int x, int y, int button)
{
  // TODO: when the mouse is dragged outside the widget, the slider should
  // snap back to the old value.
  if(isEnabled() && _isDragging && x >= (int)_labelWidth)
  {
    int newValue = posToValue(x - _labelWidth);

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
    // TODO - eventually, this should be a dirty rectangle
    _boss->instance()->frameBuffer().refresh();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
  if(isEnabled())
  {
    _isDragging = true;
    handleMouseMoved(x, y, button);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::handleMouseUp(int x, int y, int button, int clickCount)
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
    fb.drawString(_font, _label, _x, _y + 2, _labelWidth,
                  isEnabled() ? _color : kColor, kTextAlignRight);

  // Draw the box
  fb.box(_x + _labelWidth, _y, _w - _labelWidth, _h, kColor, kShadowColor);

  // Draw the 'bar'
  fb.fillRect(_x + _labelWidth + 2, _y + 2, valueToPos(_value), _h - 4,
              !isEnabled() ? kColor :
              hilite ? kTextColorHi : _color);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SliderWidget::valueToPos(int value)
{
  return ((_w - _labelWidth - 4) * (value - _valueMin) / (_valueMax - _valueMin));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SliderWidget::posToValue(int pos)
{
  return (pos) * (_valueMax - _valueMin) / (_w - _labelWidth - 4) + _valueMin;
}
