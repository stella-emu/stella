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

#include "bspf.hxx"

#include "Command.hxx"
#include "DialogContainer.hxx"
#include "Dialog.hxx"
#include "EditableWidget.hxx"
#include "Font.hxx"
#include "FrameBuffer.hxx"
#include "GuiObject.hxx"
#include "OSystem.hxx"

#include "Widget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget::Widget(GuiObject* boss, const GUI::Font& font,
               int x, int y, int w, int h)
  : GuiObject(boss->instance(), boss->parent(), boss->dialog(), x, y, w, h),
    _type(0),
    _boss(boss),
    _font((GUI::Font*)&font),
    _id(-1),
    _flags(0),
    _hasFocus(false),
    _bgcolor(kWidColor),
    _bgcolorhi(kWidColor),
    _textcolor(kTextColor),
    _textcolorhi(kTextColorHi)
{
  // Insert into the widget list of the boss
  _next = _boss->_firstWidget;
  _boss->_firstWidget = this;

  _fontWidth  = _font->getMaxCharWidth();
  _fontHeight = _font->getLineHeight();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget::~Widget()
{
  delete _next;
  _next = NULL;

  _focusList.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::draw()
{
  if(!_dirty)
    return;

  _dirty = false;
  
  FBSurface& s = _boss->dialog().surface();

  if(!isVisible() || !_boss->isVisible())
    return;

  bool hasBorder = _flags & WIDGET_BORDER;
  int oldX = _x, oldY = _y, oldW = _w, oldH = _h;

  // Account for our relative position in the dialog
  _x = getAbsX();
  _y = getAbsY();

  // Clear background (unless alpha blending is enabled)
  if(_flags & WIDGET_CLEARBG)
  {
    int x = _x, y = _y, w = _w, h = _h;
    if(hasBorder)
    {
      x++; y++; w-=2; h-=2;
    }
    s.fillRect(x, y, w, h, (_flags & WIDGET_HILITED) ? _bgcolorhi : _bgcolor);
  }

  // Draw border
  if(hasBorder) {
    s.box(_x, _y, _w, _h, kColor, kShadowColor);
    _x += 4;
    _y += 4;
    _w -= 8;
    _h -= 8;
  }

  // Now perform the actual widget draw
  drawWidget((_flags & WIDGET_HILITED) ? true : false);

  // Restore x/y
  if (hasBorder) {
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

  // Tell the framebuffer this area is dirty
  s.addDirtyRect(getAbsX(), getAbsY(), oldW, oldH);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::receivedFocus()
{
  if(_hasFocus)
    return;

  _hasFocus = true;
  receivedFocusWidget();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::lostFocus()
{
  if(!_hasFocus)
    return;

  _hasFocus = false;
  lostFocusWidget();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setEnabled(bool e)
{
  if(e)
    setFlags(WIDGET_ENABLED);
  else
    clearFlags(WIDGET_ENABLED);

  setDirty(); draw();
}

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
bool Widget::isWidgetInChain(WidgetArray& list, Widget* find)
{
  bool found = false;

  for(int i = 0; i < (int)list.size(); ++i)
  {
    if(list[i] == find)
    {
      found = true;
      break;
    }
  }

  return found;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget* Widget::setFocusForChain(GuiObject* boss, WidgetArray& arr,
                                 Widget* wid, int direction)
{
  FBSurface& s = boss->dialog().surface();
  int size = arr.size(), pos = -1;
  Widget* tmp;
  for(int i = 0; i < size; ++i)
  {
    tmp = arr[i];

    // Determine position of widget 'w'
    if(wid == tmp)
      pos = i;

    // Get area around widget
    // Note: we must use getXXX() methods and not access the variables
    // directly, since in some cases (notably those widgets with embedded
    // ScrollBars) the two quantities may be different
    int x = tmp->getAbsX() - 1,  y = tmp->getAbsY() - 1,
        w = tmp->getWidth() + 2, h = tmp->getHeight() + 2;

    // First clear area surrounding all widgets
    if(tmp->_hasFocus)
    {
      tmp->lostFocus();
      s.frameRect(x, y, w, h, kDlgColor);

      tmp->setDirty(); tmp->draw();
      s.addDirtyRect(x, y, w, h);
    }
  }

  // Figure out which which should be active
  if(pos == -1)
    return 0;
  else
  {
    switch(direction)
    {
      case -1:  // previous widget
        pos--;
        if(pos < 0)
          pos = size - 1;
        break;

      case +1:  // next widget
        pos++;
        if(pos >= size)
          pos = 0;
        break;

      default:
        // pos already set
        break;
    }
  }

  // Now highlight the active widget
  tmp = arr[pos];

  // Get area around widget
  // Note: we must use getXXX() methods and not access the variables
  // directly, since in some cases (notably those widgets with embedded
  // ScrollBars) the two quantities may be different
  int x = tmp->getAbsX() - 1,  y = tmp->getAbsY() - 1,
      w = tmp->getWidth() + 2, h = tmp->getHeight() + 2;

  tmp->receivedFocus();
  s.frameRect(x, y, w, h, kWidFrameColor, kDashLine);

  tmp->setDirty(); tmp->draw();
  s.addDirtyRect(x, y, w, h);

  return tmp;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setDirtyInChain(Widget* start)
{
  while(start)
  {
    start->setDirty();
    start = start->_next;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StaticTextWidget::StaticTextWidget(GuiObject *boss, const GUI::Font& font,
                                   int x, int y, int w, int h,
                                   const string& text, TextAlignment align)
  : Widget(boss, font, x, y, w, h),
    _align(align)
{
  _flags = WIDGET_ENABLED | WIDGET_CLEARBG;
  _type = kStaticTextWidget;
  _bgcolor = kDlgColor;
  _bgcolorhi = kDlgColor;
  _textcolor = kTextColor;
  _textcolorhi = kTextColor;

  _label = text;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaticTextWidget::setValue(int value)
{
  char buf[256];
  BSPF_snprintf(buf, 255, "%d", value);
  _label = buf;

  setDirty(); draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaticTextWidget::setLabel(const string& label)
{
  _label = label;

  setDirty(); draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaticTextWidget::drawWidget(bool hilite)
{
  FBSurface& s = _boss->dialog().surface();
  s.drawString(_font, _label, _x, _y, _w,
               isEnabled() ? _textcolor : kColor, _align);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ButtonWidget::ButtonWidget(GuiObject *boss, const GUI::Font& font,
                           int x, int y, int w, int h,
                           const string& label, int cmd)
  : StaticTextWidget(boss, font, x, y, w, h, label, kTextAlignCenter),
    CommandSender(boss),
    _cmd(cmd)
{
  _flags = WIDGET_ENABLED | WIDGET_BORDER | WIDGET_CLEARBG;
  _type = kButtonWidget;
  _bgcolor = kBtnColor;
  _bgcolorhi = kBtnColorHi;
  _textcolor = kBtnTextColor;
  _textcolorhi = kBtnTextColorHi;

  _editable = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::handleMouseEntered(int button)
{
  setFlags(WIDGET_HILITED);
  setDirty(); draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::handleMouseLeft(int button)
{
  clearFlags(WIDGET_HILITED);
  setDirty(); draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ButtonWidget::handleEvent(Event::Type e)
{
  if(!isEnabled())
    return false;

  switch(e)
  {
    case Event::UISelect:
      // Simulate mouse event
      handleMouseUp(0, 0, 1, 0);
      return true;
    default:
      return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::handleMouseUp(int x, int y, int button, int clickCount)
{
  if(isEnabled() && x >= 0 && x < _w && y >= 0 && y < _h)
  {
    clearFlags(WIDGET_HILITED);
    sendCommand(_cmd, 0, _id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::drawWidget(bool hilite)
{
  FBSurface& s = _boss->dialog().surface();
  s.drawString(_font, _label, _x, _y + (_h - _fontHeight)/2 + 1, _w,
               !isEnabled() ? kColor : hilite ? _textcolorhi : _textcolor, _align);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/* 8x8 checkbox bitmap */
static unsigned int checked_img_x[8] =
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

static unsigned int checked_img_o[8] =
{
	0x00011000,
	0x00111100,
	0x01111110,
	0x11111111,
	0x11111111,
	0x01111110,
	0x00111100,
	0x00011000,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheckboxWidget::CheckboxWidget(GuiObject *boss, const GUI::Font& font,
                               int x, int y, const string& label,
                               int cmd)
  : ButtonWidget(boss, font, x, y, 16, 16, label, cmd),
    _state(false),
    _holdFocus(true),
    _fillRect(false),
    _drawBox(true),
    _fillColor(kColor),
    _boxY(0),
    _textY(0)
{
  _flags = WIDGET_ENABLED;
  _type = kCheckboxWidget;
  _bgcolor = _bgcolorhi = kWidColor;

  _editable = true;

  if(label == "")
    _w = 14;
  else
    _w = font.getStringWidth(label) + 20;
  _h = font.getFontHeight() < 14 ? 14 : font.getFontHeight();


  // Depending on font size, either the font or box will need to be 
  // centered vertically
  if(_h > 14)  // center box
    _boxY = (_h - 14) / 2;
  else         // center text
    _textY = (14 - _font->getFontHeight()) / 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::handleMouseUp(int x, int y, int button, int clickCount)
{
  if(isEnabled() && _editable && x >= 0 && x < _w && y >= 0 && y < _h)
  {
    toggleState();

    // We only send a command when the widget has been changed interactively
    sendCommand(_cmd, _state, _id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::setEditable(bool editable)
{
  _editable = editable;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::setState(bool state)
{
  if(_state != state)
  {
    _state = state;
    setDirty(); draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::drawWidget(bool hilite)
{
  FBSurface& s = _boss->dialog().surface();

  // Draw the box
  if(_drawBox)
    s.box(_x, _y + _boxY, 14, 14, kColor, kShadowColor);

  // Do we draw a square or cross?
  s.fillRect(_x + 2, _y + _boxY + 2, 10, 10, _bgcolor);
  if(isEnabled())
  {
    if(_state)
    {
      uInt32* img  = _fillRect ? checked_img_o : checked_img_x;
	  uInt32 color = _fillRect ? kWidFrameColor : kCheckColor;
	  s.drawBitmap(img, _x + 3, _y + _boxY + 3, color);
    }
  }
  else
    s.fillRect(_x + 2, _y + _boxY + 2, 10, 10, kColor);

  // Finally draw the label
  s.drawString(_font, _label, _x + 20, _y + _textY, _w,
               isEnabled() ? kTextColor : kColor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SliderWidget::SliderWidget(GuiObject *boss, const GUI::Font& font,
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
  _flags = WIDGET_ENABLED | WIDGET_TRACK_MOUSE | WIDGET_CLEARBG;
  _type = kSliderWidget;
  _bgcolor = kDlgColor;
  _bgcolorhi = kDlgColor;

  if(!_label.empty() && _labelWidth == 0)
    _labelWidth = _font->getStringWidth(_label);

  _w = w + _labelWidth;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setValue(int value)
{
  if(value < _valueMin)      value = _valueMin;
  else if(value > _valueMax) value = _valueMax;

  if(value != _value)
  {
    _value = value; 
    setDirty(); draw();
    sendCommand(_cmd, _value, _id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setMinValue(int value)
{
  _valueMin = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setMaxValue(int value)
{
  _valueMax = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setStepValue(int value)
{
  _stepValue = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::handleMouseMoved(int x, int y, int button)
{
  // TODO: when the mouse is dragged outside the widget, the slider should
  // snap back to the old value.
  if(isEnabled() && _isDragging && x >= (int)_labelWidth)
    setValue(posToValue(x - _labelWidth));
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
    sendCommand(_cmd, _value, _id);

  _isDragging = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::handleMouseWheel(int x, int y, int direction)
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
bool SliderWidget::handleEvent(Event::Type e)
{
  if(!isEnabled())
    return false;

  switch(e)
  {
    case Event::UIDown:
    case Event::UIPgDown:
      setValue(_value - _stepValue);
      break;
  
    case Event::UIUp:
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
      break;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::drawWidget(bool hilite)
{
  FBSurface& s = _boss->dialog().surface();

  // Draw the label, if any
  if(_labelWidth > 0)
    s.drawString(_font, _label, _x, _y + 2, _labelWidth,
                 isEnabled() ? kTextColor : kColor, kTextAlignRight);

  // Draw the box
  s.box(_x + _labelWidth, _y, _w - _labelWidth, _h, kColor, kShadowColor);

  // Fill the box
  s.fillRect(_x + _labelWidth + 2, _y + 2, _w - _labelWidth - 4, _h - 4,
             !isEnabled() ? kColor : kWidColor);

  // Draw the 'bar'
  s.fillRect(_x + _labelWidth + 2, _y + 2, valueToPos(_value), _h - 4,
             !isEnabled() ? kColor : hilite ? kSliderColorHi : kSliderColor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SliderWidget::valueToPos(int value)
{
  if(value < _valueMin)      value = _valueMin;
  else if(value > _valueMax) value = _valueMax;
  int range = BSPF_max(_valueMax - _valueMin, 1);  // don't divide by zero
  
  return ((_w - _labelWidth - 4) * (value - _valueMin) / range);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SliderWidget::posToValue(int pos)
{
  int value = (pos) * (_valueMax - _valueMin) / (_w - _labelWidth - 4) + _valueMin;

  // Scale the position to the correct interval (according to step value)
  return value - (value % _stepValue);
}
