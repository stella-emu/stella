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
// $Id: CheckListWidget.cxx,v 1.1 2005-08-22 13:53:23 stephena Exp $
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
#include "CheckListWidget.hxx"
#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheckListWidget::CheckListWidget(GuiObject* boss, const GUI::Font& font,
                                 int x, int y, int cols, int rows)
  : EditableWidget(boss, x, y, 16, 16)
{
  _flags = WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS;

  setFont(font);

  _cols = cols;
  _rows = rows;

  _colWidth  = font.getMaxCharWidth();
  _rowHeight = font.getLineHeight();

  _w = _cols * _colWidth - kScrollBarWidth;
  _h = _rows * _rowHeight + 2;


  _editMode = false;
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
CheckListWidget::~CheckListWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckListWidget::setSelected(int item)
{
  assert(item >= -1 && item < (int)_list.size());

  if (isEnabled() && _selectedItem != item)
  {
    if (_editMode)
      abortEditMode();

    _selectedItem = item;
    sendCommand(kListSelectionChangedCmd, _selectedItem, _id);

    _currentPos = _selectedItem - _rows / 2;
    scrollToCurrent();
    setDirty(); draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckListWidget::setList(const StringList& list)
{
  int size = list.size();
  _list = list;
  if (_currentPos >= size)
    _currentPos = size - 1;
  if (_currentPos < 0)
    _currentPos = 0;
  _selectedItem = -1;
  _editMode = false;
  scrollBarRecalc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckListWidget::scrollTo(int item)
{
  int size = _list.size();
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
void CheckListWidget::scrollBarRecalc()
{
  _scrollBar->_numEntries     = _list.size();
  _scrollBar->_entriesPerPage = _rows;
  _scrollBar->_currentPos     = _currentPos;
  _scrollBar->recalc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckListWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
  if (!isEnabled())
    return;

  // First check whether the selection changed
  int newSelectedItem;
  newSelectedItem = findItem(x, y);
  if (newSelectedItem > (int)_list.size() - 1)
    newSelectedItem = -1;

  if (_selectedItem != newSelectedItem)
  {
    if (_editMode)
      abortEditMode();
    _selectedItem = newSelectedItem;
    sendCommand(kListSelectionChangedCmd, _selectedItem, _id);
    setDirty(); draw();
  }
	
  // TODO: Determine where inside the string the user clicked and place the
  // caret accordingly. See _editScrollOffset and EditTextWidget::handleMouseDown.
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckListWidget::handleMouseUp(int x, int y, int button, int clickCount)
{
  // If this was a double click and the mouse is still over the selected item,
  // send the double click command
  if (clickCount == 2 && (_selectedItem == findItem(x, y)))
  {
    sendCommand(kListItemDoubleClickedCmd, _selectedItem, _id);

    // Start edit mode
    if(_editable && !_editMode)
      startEditMode();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckListWidget::handleMouseWheel(int x, int y, int direction)
{
  _scrollBar->handleMouseWheel(x, y, direction);
  setDirty(); draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CheckListWidget::findItem(int x, int y) const
{
  return (y - 1) / _rowHeight + _currentPos;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static bool matchingCharsIgnoringCase(string s, string pattern)
{
  // Make the strings uppercase so we can compare them
  transform(s.begin(), s.end(), s.begin(), (int(*)(int)) toupper);
  transform(pattern.begin(), pattern.end(), pattern.begin(), (int(*)(int)) toupper);

  // Make sure that if the pattern is found, it occurs at the start of 's'
  return (s.find(pattern, 0) == string::size_type(0));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CheckListWidget::handleKeyDown(int ascii, int keycode, int modifiers)
{
  // Ignore all mod keys
  if(instance()->eventHandler().kbdControl(modifiers) ||
     instance()->eventHandler().kbdAlt(modifiers))
    return true;

  bool handled = true;
  bool dirty = false;
  int oldSelectedItem = _selectedItem;

  if (!_editMode && isalnum((char)ascii))
  {
    // Quick selection mode: Go to first list item starting with this key
    // (or a substring accumulated from the last couple key presses).
    // Only works in a useful fashion if the list entries are sorted.
    // TODO: Maybe this should be off by default, and instead we add a
    // method "enableQuickSelect()" or so ?
    int time = instance()->getTicks() / 1000;
    if (_quickSelectTime < time)
      _quickSelectStr = (char)ascii;
    else
      _quickSelectStr += (char)ascii;
    _quickSelectTime = time + 300; 

    // FIXME: This is bad slow code (it scans the list linearly each time a
    // key is pressed); it could be much faster. Only of importance if we have
    // quite big lists to deal with -- so for now we can live with this lazy
    // implementation :-)
    int newSelectedItem = 0;
    for (StringList::const_iterator i = _list.begin(); i != _list.end(); ++i)
    {
      const bool match = matchingCharsIgnoringCase(*i, _quickSelectStr);
      if (match)
      {
        _selectedItem = newSelectedItem;
        break;
      }
      newSelectedItem++;
    }
    scrollToCurrent();
  }
  else if (_editMode)
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
            sendCommand(kListItemActivatedCmd, _selectedItem, _id);
        }
        break;

      case 256+17:  // up arrow
        if (_selectedItem > 0)
          _selectedItem--;
        break;

      case 256+18:  // down arrow
        if (_selectedItem < (int)_list.size() - 1)
          _selectedItem++;
        break;

      case 256+24:  // pageup
        _selectedItem -= _rows - 1;
        if (_selectedItem < 0)
          _selectedItem = 0;
        break;

      case 256+25:	// pagedown
        _selectedItem += _rows - 1;
        if (_selectedItem >= (int)_list.size() )
          _selectedItem = _list.size() - 1;
        break;

      case 256+22:  // home
        _selectedItem = 0;
        break;

      case 256+23:  // end
        _selectedItem = _list.size() - 1;
        break;

      default:
        handled = false;
    }

    scrollToCurrent();
  }

  if (_selectedItem != oldSelectedItem)
  {
    sendCommand(kListSelectionChangedCmd, _selectedItem, _id);
    // also draw scrollbar
    _scrollBar->draw();

    setDirty(); draw();
  }

  _currentKeyDown = keycode;
  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CheckListWidget::handleKeyUp(int ascii, int keycode, int modifiers)
{
  if (keycode == _currentKeyDown)
    _currentKeyDown = 0;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckListWidget::lostFocusWidget()
{
  _editMode = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckListWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch (cmd)
  {
    case kSetPositionCmd:
      if (_currentPos != (int)data)
      {
        _currentPos = data;
        setDirty(); draw();
      }
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckListWidget::drawWidget(bool hilite)
{
//cerr << "CheckListWidget::drawWidget\n";
  FrameBuffer& fb = _boss->instance()->frameBuffer();
  int i, pos, len = _list.size();
  string buffer;
  int deltax;

  // Draw a thin frame around the list.
  fb.hLine(_x, _y, _x + _w - 1, kColor);
  fb.hLine(_x, _y + _h - 1, _x + _w - 1, kShadowColor);
  fb.vLine(_x, _y, _y + _h - 1, kColor);

  // Draw the list items
  for (i = 0, pos = _currentPos; i < _rows && pos < len; i++, pos++)
  {
    const OverlayColor textColor = (_selectedItem == pos && _editMode)
                                    ? kColor : kTextColor;
    const int y = _y + 2 + _rowHeight * i;

    // Draw the selected item inverted, on a highlighted background.
    if (_selectedItem == pos)
    {
      if (_hasFocus && !_editMode)
        fb.fillRect(_x + 1, _y + 1 + _rowHeight * i, _w - 1, _rowHeight, kTextColorHi);
      else
        fb.frameRect(_x + 1, _y + 1 + _rowHeight * i, _w - 1, _rowHeight, kTextColorHi);
    }

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
      buffer = _list[pos];
      deltax = 0;
      fb.drawString(_font, buffer, _x + r.left, y, r.width(), kTextColor);
    }
  }

  // Only draw the caret while editing, and if it's in the current viewport
  if(_editMode && (_selectedItem >= _scrollBar->_currentPos) &&
    (_selectedItem < _scrollBar->_currentPos + _rows))
    drawCaret();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GUI::Rect CheckListWidget::getEditRect() const
{
  GUI::Rect r(2, 1, _w - 2 , _rowHeight);
  const int offset = (_selectedItem - _currentPos) * _rowHeight;
  r.top += offset;
  r.bottom += offset;
	
  return r;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckListWidget::scrollToCurrent()
{
  // Only do something if the current item is not in our view port
  if (_selectedItem < _currentPos)
  {
    // it's above our view
    _currentPos = _selectedItem;
  }
  else if (_selectedItem >= _currentPos + _rows)
  {
    // it's below our view
    _currentPos = _selectedItem - _rows + 1;
  }

  if (_currentPos < 0 || _rows > (int)_list.size())
    _currentPos = 0;
  else if (_currentPos + _rows > (int)_list.size())
    _currentPos = _list.size() - _rows;

  _scrollBar->_currentPos = _currentPos;
  _scrollBar->recalc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckListWidget::startEditMode()
{
  if (_editable && !_editMode && _selectedItem >= 0)
  {
    _editMode = true;
    setEditString(_list[_selectedItem]);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckListWidget::endEditMode()
{
  if (!_editMode)
    return;

  // send a message that editing finished with a return/enter key press
  _editMode = false;
  _list[_selectedItem] = _editString;
  sendCommand(kListItemDataChangedCmd, _selectedItem, _id);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckListWidget::abortEditMode()
{
  // undo any changes made
  assert(_selectedItem >= 0);
  _editMode = false;
}
