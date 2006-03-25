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
// $Id: Widget.cxx,v 1.43 2006-03-25 00:34:17 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "Font.hxx"
#include "Dialog.hxx"
#include "DialogContainer.hxx"
#include "Command.hxx"
#include "GuiObject.hxx"
#include "bspf.hxx"
#include "GuiUtils.hxx"
#include "Widget.hxx"
#include "EditableWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget::Widget(GuiObject* boss, const GUI::Font& font,
               int x, int y, int w, int h)
  : GuiObject(boss->instance(), boss->parent(), x, y, w, h),
    _type(0),
    _boss(boss),
    _font((GUI::Font*)&font),
    _id(-1),
    _flags(0),
    _hasFocus(false),
    _color(kTextColor)
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
  
  FrameBuffer& fb = _boss->instance()->frameBuffer();

  if(!isVisible() || !_boss->isVisible())
    return;

  int oldX = _x, oldY = _y, oldW = _w, oldH = _h;

  // Account for our relative position in the dialog
  _x = getAbsX();
  _y = getAbsY();

  // Clear background (unless alpha blending is enabled)
  if(_flags & WIDGET_CLEARBG)
    fb.fillRect(_x, _y, _w, _h, kBGColor);

  // Draw border
  if(_flags & WIDGET_BORDER) {
    int colorA = kColor;
    int colorB = kShadowColor;
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

  // Tell the framebuffer this area is dirty
  fb.addDirtyRect(getAbsX(), getAbsY(), oldW, oldH);
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
GUI::Rect Widget::getRect() const
{
  int x = getAbsX() - 1,  y = getAbsY() - 1,
      w = getWidth() + 2, h = getHeight() + 2;

  GUI::Rect r(x, y, x+w, y+h);
  return r;
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
  FrameBuffer& fb = boss->instance()->frameBuffer();
  int size = arr.size(), pos = -1;
  Widget* tmp;
  for(int i = 0; i < size; ++i)
  {
    tmp = arr[i];

    // Determine position of widget 'w'
    if(wid == tmp)
      pos = i;

    GUI::Rect rect = tmp->getRect();
    int x = rect.left,    y = rect.top,
        w = rect.width(), h = rect.height();

    // First clear area surrounding all widgets
    if(tmp->_hasFocus)
    {
      tmp->lostFocus();
      if(!(tmp->_flags & WIDGET_NODRAW_FOCUS))
        fb.frameRect(x, y, w, h, kBGColor);

      tmp->setDirty(); tmp->draw();
      fb.addDirtyRect(x, y, w, h);
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
  GUI::Rect rect = tmp->getRect();
  int x = rect.left,    y = rect.top,
      w = rect.width(), h = rect.height();

  tmp->receivedFocus();
  if(!(tmp->_flags & WIDGET_NODRAW_FOCUS))
    fb.frameRect(x, y, w, h, kTextColorEm, kDashLine);

  tmp->setDirty(); tmp->draw();
  fb.addDirtyRect(x, y, w, h);

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
  _label = text;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaticTextWidget::setValue(int value)
{
  char buf[256];
  sprintf(buf, "%d", value);
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
  FrameBuffer& fb = _boss->instance()->frameBuffer();
  fb.drawString(_font, _label, _x, _y, _w,
                isEnabled() ? _color : kColor, _align);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ButtonWidget::ButtonWidget(GuiObject *boss, const GUI::Font& font,
                           int x, int y, int w, int h,
                           const string& label, int cmd, uInt8 hotkey)
  : StaticTextWidget(boss, font, x, y, w, h, label, kTextAlignCenter),
    CommandSender(boss),
    _cmd(cmd),
    _editable(false),
    _hotkey(hotkey)
{
  _flags = WIDGET_ENABLED | WIDGET_BORDER | WIDGET_CLEARBG;
  _type = kButtonWidget;
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
bool ButtonWidget::handleKeyDown(int ascii, int keycode, int modifiers)
{
  // (De)activate with space or return
  switch(ascii)
  {
    case '\n':  // enter/return
    case '\r':
    case ' ' :  // space
      // Simulate mouse event
      handleMouseUp(0, 0, 1, 0);
      return true;
    default:
      return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::handleJoyDown(int stick, int button)
{
  // Any button activates the button, but only while in joymouse mode
  if(DialogContainer::joymouse())
    handleMouseUp(0, 0, 1, 0);
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
bool ButtonWidget::wantsFocus()
{
  return _editable;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::setEditable(bool editable)
{
  _editable = editable;
  if(_editable)
    setFlags(WIDGET_RETAIN_FOCUS);
  else
    clearFlags(WIDGET_RETAIN_FOCUS);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::drawWidget(bool hilite)
{
  FrameBuffer& fb = _boss->instance()->frameBuffer();
  fb.drawString(_font, _label, _x, _y + (_h - _fontHeight)/2 + 1, _w,
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
CheckboxWidget::CheckboxWidget(GuiObject *boss, const GUI::Font& font,
                               int x, int y, const string& label,
                               int cmd)
  : ButtonWidget(boss, font, x, y, 16, 16, label, cmd, 0),
    _state(false),
    _editable(true),
    _holdFocus(true),
    _fillRect(false),
    _drawBox(true),
    _fillColor(kColor),
    _boxY(0),
    _textY(0)
{
  _flags = WIDGET_ENABLED | WIDGET_RETAIN_FOCUS;
  _type = kCheckboxWidget;

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
bool CheckboxWidget::handleKeyDown(int ascii, int keycode, int modifiers)
{
  // (De)activate with space or return
  switch(ascii)
  {
    case '\n':  // enter/return
    case '\r':
    case ' ' :  // space
      // Simulate mouse event
      handleMouseUp(0, 0, 1, 0);
      return true;
    default:
      return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CheckboxWidget::wantsFocus()
{
  if(!_holdFocus)
    return false;
  else
    return _editable;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::setEditable(bool editable)
{
  _holdFocus = _editable = editable;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::setState(bool state)
{
  if(_state != state)
  {
    _state = state;
    _flags ^= WIDGET_INV_BORDER;
    setDirty(); draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::drawWidget(bool hilite)
{
  FrameBuffer& fb = _boss->instance()->frameBuffer();

  // Draw the box
  if(_drawBox)
    fb.box(_x, _y + _boxY, 14, 14, kColor, kShadowColor);

  // If checked, draw cross inside the box
  if(_state)
  {
    if(_fillRect)
      fb.fillRect(_x + 2, _y + _boxY + 2, 10, 10,
                  isEnabled() ? _color : kColor);
    else
      fb.drawBitmap(checked_img, _x + 3, _y + _boxY + 3,
                    isEnabled() ? _color : kColor);
  }
  else
    fb.fillRect(_x + 2, _y + _boxY + 2, 10, 10, kBGColor);

  // Finally draw the label
  fb.drawString(_font, _label, _x + 20, _y + _textY, _w,
                isEnabled() ? _color : kColor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SliderWidget::SliderWidget(GuiObject *boss, const GUI::Font& font,
                           int x, int y, int w, int h,
                           const string& label, int labelWidth, int cmd, uInt8 hotkey)
  : ButtonWidget(boss, font, x, y, w, h, label, cmd, hotkey),
    _value(0),
    _oldValue(0),
    _valueMin(0),
    _valueMax(100),
    _isDragging(false),
    _labelWidth(labelWidth)
{
  _flags = WIDGET_ENABLED | WIDGET_TRACK_MOUSE | WIDGET_CLEARBG;
  _type = kSliderWidget;

  if(!_label.empty() && _labelWidth == 0)
    _labelWidth = _font->getStringWidth(_label);

  _w = w + _labelWidth;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setValue(int value)
{
  _value = value;
  setDirty(); draw();
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
      setDirty(); draw();
      sendCommand(_cmd, _value, _id);
    }
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
    sendCommand(_cmd, _value, _id);

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
