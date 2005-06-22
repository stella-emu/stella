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
// $Id: AddrValueWidget.cxx,v 1.6 2005-06-22 18:30:43 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <cctype>
#include <algorithm>

#include "OSystem.hxx"
#include "Widget.hxx"
#include "ScrollBarWidget.hxx"
#include "Dialog.hxx"
#include "FrameBuffer.hxx"
#include "AddrValueWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AddrValueWidget::AddrValueWidget(GuiObject* boss, int x, int y, int w, int h,
                                 int range, BaseFormat base)
  : EditableWidget(boss, x, y, w, h),
    CommandSender(boss),
    _range(range),
    _base(base)
{
  _w = w - kScrollBarWidth;
	
  _flags = WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS |
           WIDGET_TAB_NAVIGATE;
  _type = kListWidget;  // we're just a slightly modified listwidget
  _editMode = false;

  _entriesPerPage = (_h - 2) / kLineHeight;
  _currentPos = 0;
  _selectedItem = -1;
  _scrollBar = new ScrollBarWidget(boss, _x + _w, _y, kScrollBarWidth, _h);
  _scrollBar->setTarget(this);
  _currentKeyDown = 0;
	
  _quickSelectTime = 0;

  // The item is selected, thus _bgcolor is used to draw the caret and
  // _textcolorhi to erase it
  _caretInverse = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AddrValueWidget::~AddrValueWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AddrValueWidget::setList(const AddrList& alist, const ValueList& vlist)
{
  _addrList.clear();
  _valueList.clear();
  _addrStringList.clear();
  _valueStringList.clear();

  _addrList = alist;
  _valueList = vlist;

  int size = _addrList.size();  // assume vlist is the same size

  // An efficiency thing
  char temp[10];
  string str;
  for(unsigned int i = 0; i < (unsigned int)size; ++i)
  {
    sprintf(temp, "%.4x:", _addrList[i]);
    _addrStringList.push_back(temp);
    str = instance()->debugger().valueToString(_valueList[i], _base);
    _valueStringList.push_back(str);
  }

  if (_currentPos >= size)
    _currentPos = size - 1;
  if (_currentPos < 0)
    _currentPos = 0;
  _selectedItem = -1;
  _editMode = false;
  scrollBarRecalc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AddrValueWidget::scrollTo(int item)
{
  int size = _valueList.size();
  if (item >= size)
    item = size - 1;
  if (item < 0)
    item = 0;

  if (_currentPos != item)
  {
    _currentPos = item;
    scrollBarRecalc();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AddrValueWidget::scrollBarRecalc()
{
  _scrollBar->_numEntries = _valueList.size();
  _scrollBar->_entriesPerPage = _entriesPerPage;
  _scrollBar->_currentPos = _currentPos;
  _scrollBar->recalc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AddrValueWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
  if (!isEnabled())
    return;

  // A click indicates this widget has been selected
  // It should receive focus (because it has the WIDGET_TAB_NAVIGATE property)
  receivedFocus();

  // First check whether the selection changed
  int newSelectedItem;
  newSelectedItem = findItem(x, y);
  if (newSelectedItem > (int)_valueList.size() - 1)
    newSelectedItem = -1;

  if (_selectedItem != newSelectedItem)
  {
    if (_editMode)
      abortEditMode();
    _selectedItem = newSelectedItem;
    sendCommand(kAVSelectionChangedCmd, _selectedItem);
    instance()->frameBuffer().refresh();
  }
	
  // TODO: Determine where inside the string the user clicked and place the
  // caret accordingly. See _editScrollOffset and EditTextWidget::handleMouseDown.
  draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AddrValueWidget::handleMouseUp(int x, int y, int button, int clickCount)
{
  // If this was a double click and the mouse is still over the selected item,
  // send the double click command
  if (clickCount == 2 && (_selectedItem == findItem(x, y)))
  {
    sendCommand(kAVItemDoubleClickedCmd, _selectedItem);

    // Start edit mode
    if(_editable && !_editMode)
      startEditMode();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AddrValueWidget::handleMouseWheel(int x, int y, int direction)
{
  _scrollBar->handleMouseWheel(x, y, direction);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int AddrValueWidget::findItem(int x, int y) const
{
  return (y - 1) / kLineHeight + _currentPos;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AddrValueWidget::handleKeyDown(int ascii, int keycode, int modifiers)
{
  // Ignore all mod keys
  if(instance()->eventHandler().kbdControl(modifiers) ||
     instance()->eventHandler().kbdAlt(modifiers))
    return true;

  bool handled = true;
  bool dirty = false;
  int oldSelectedItem = _selectedItem;

  if (_editMode)
  {
    // Class EditableWidget handles all text editing related key presses for us
    handled = EditableWidget::handleKeyDown(ascii, keycode, modifiers);
  }
  else
  {
    // not editmode
    switch (keycode)
    {
      case '\n':  // enter/return
      case '\r':
        if (_selectedItem >= 0)
        {
          // override continuous enter keydown
          if (_editable && (_currentKeyDown != '\n' && _currentKeyDown != '\r'))
          {
            dirty = true;
            startEditMode();
          }
          else
            sendCommand(kAVItemActivatedCmd, _selectedItem);
        }
        break;

      case 256+17:  // up arrow
        if (_selectedItem > 0)
          _selectedItem--;
        break;

      case 256+18:  // down arrow
        if (_selectedItem < (int)_valueList.size() - 1)
          _selectedItem++;
        break;

      case 256+24:  // pageup
        _selectedItem -= _entriesPerPage - 1;
        if (_selectedItem < 0)
          _selectedItem = 0;
        break;

      case 256+25:	// pagedown
        _selectedItem += _entriesPerPage - 1;
        if (_selectedItem >= (int)_valueList.size() )
          _selectedItem = _valueList.size() - 1;
        break;

      case 256+22:  // home
        _selectedItem = 0;
        break;

      case 256+23:  // end
        _selectedItem = _valueList.size() - 1;
        break;

      default:
        handled = false;
    }

    scrollToCurrent();
  }

  if (dirty || _selectedItem != oldSelectedItem)
    draw();

  if (_selectedItem != oldSelectedItem)
  {
    sendCommand(kAVSelectionChangedCmd, _selectedItem);
    // also draw scrollbar
    _scrollBar->draw();

    instance()->frameBuffer().refresh();
  }

  _currentKeyDown = keycode;
  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AddrValueWidget::handleKeyUp(int ascii, int keycode, int modifiers)
{
  if (keycode == _currentKeyDown)
    _currentKeyDown = 0;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AddrValueWidget::lostFocusWidget()
{
  _editMode = false;
  draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AddrValueWidget::handleCommand(CommandSender* sender, int cmd, int data)
{
  switch (cmd)
  {
    case kSetPositionCmd:
      if (_currentPos != (int)data)
      {
        _currentPos = data;
        draw();
      }
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AddrValueWidget::drawWidget(bool hilite)
{
  FrameBuffer& fb = _boss->instance()->frameBuffer();
  int i, pos, len = _valueList.size();
  string buffer;
  int deltax;

  // Draw a thin frame around the list.
  fb.hLine(_x, _y, _x + _w - 1, kColor);
  fb.hLine(_x, _y + _h - 1, _x + _w - 1, kShadowColor);
  fb.vLine(_x, _y, _y + _h - 1, kColor);

  // Draw the list items
  for (i = 0, pos = _currentPos; i < _entriesPerPage && pos < len; i++, pos++)
  {
    const OverlayColor textColor = (_selectedItem == pos && _editMode)
                                    ? kColor : kTextColor;
    const int y = _y + 2 + kLineHeight * i;

    // Draw the selected item inverted, on a highlighted background.
    if (_selectedItem == pos)
    {
      if (_hasFocus && !_editMode)
        fb.fillRect(_x + 1, _y + 1 + kLineHeight * i, _w - 1, kLineHeight, kTextColorHi);
      else
        fb.frameRect(_x + 1, _y + 1 + kLineHeight * i, _w - 2, kLineHeight, kTextColorHi);
    }

    // Print the address
    fb.drawString(_font, _addrStringList[pos], _x + 2, y, _w - 4, textColor);

    GUI::Rect r(getEditRect());
    if (_selectedItem == pos && _editMode)
    {
      buffer = _editString;
      adjustOffset();
      deltax = -_editScrollOffset;

      fb.drawString(_font, buffer, _x + r.left, y, r.width(), kTextColor,
                    kTextAlignLeft, deltax, false);
    }
    else
    {
      buffer = _valueStringList[pos];
      deltax = 0;
      fb.drawString(_font, buffer, _x + r.left, y, r.width(), kTextColor);
    }
  }

  // Only draw the caret while editing, and if it's in the current viewport
  if(_editMode && (_selectedItem >= _scrollBar->_currentPos) &&
    (_selectedItem < _scrollBar->_currentPos + _entriesPerPage))
    drawCaret();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GUI::Rect AddrValueWidget::getEditRect() const
{
  GUI::Rect r(2, 1, _w - 3 , kLineHeight);
  const int offset = (_selectedItem - _currentPos) * kLineHeight;
  r.top += offset;
  r.bottom += offset;
  r.left += 9 * _font->getMaxCharWidth();  // address takes 9 characters
	
  return r;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AddrValueWidget::scrollToCurrent()
{
  // Only do something if the current item is not in our view port
  if (_selectedItem < _currentPos)
  {
    // it's above our view
    _currentPos = _selectedItem;
  }
  else if (_selectedItem >= _currentPos + _entriesPerPage )
  {
    // it's below our view
    _currentPos = _selectedItem - _entriesPerPage + 1;
  }

  if (_currentPos < 0 || _entriesPerPage > (int)_valueList.size())
    _currentPos = 0;
  else if (_currentPos + _entriesPerPage > (int)_valueList.size())
    _currentPos = _valueList.size() - _entriesPerPage;

  _scrollBar->_currentPos = _currentPos;
  _scrollBar->recalc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AddrValueWidget::startEditMode()
{
  if (_editable && !_editMode && _selectedItem >= 0)
  {
    _editMode = true;
    setEditString("");  // Erase current entry when starting editing
    draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AddrValueWidget::endEditMode()
{
  if (!_editMode)
    return;

  // send a message that editing finished with a return/enter key press
  _editMode = false;

  // Update the both the string representation and the real data
  int value = instance()->debugger().stringToValue(_editString);
  if(value < 0 || value > _range)
  {
    abortEditMode();
    return;
  }

  // Correctly format the data for viewing
  _editString = instance()->debugger().valueToString(value, _base);

  _valueStringList[_selectedItem] = _editString;
  _valueList[_selectedItem] = value;

  sendCommand(kAVItemDataChangedCmd, _selectedItem);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AddrValueWidget::abortEditMode()
{
  // undo any changes made
  assert(_selectedItem >= 0);
  _editMode = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AddrValueWidget::tryInsertChar(char c, int pos)
{
  // Not sure how efficient this is, or should we even care?
  c = tolower(c);
  if((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
     c == '%' || c == '#' || c == '$')
  {
    _editString.insert(pos, 1, c);
    return true;
  }
  else
    return false;
}
