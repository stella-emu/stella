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
// $Id: ListWidget.cxx,v 1.4 2005-05-04 00:43:22 stephena Exp $
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
#include "ListWidget.hxx"
#include "bspf.hxx"


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ListWidget::ListWidget(GuiObject* boss, Int32 x, Int32 y, Int32 w, Int32 h)
    : Widget(boss, x, y, w - kScrollBarWidth, h),
      CommandSender(boss)
{
  _flags = WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS | WIDGET_WANT_TICKLE;
  _type = kListWidget;
  _numberingMode = kListNumberingOne;
  _entriesPerPage = (_h - 2) / kLineHeight;
  _currentPos = 0;
  _selectedItem = -1;
  _scrollBar = new ScrollBarWidget(boss, _x + _w, _y, kScrollBarWidth, _h);
  _scrollBar->setTarget(this);
  _currentKeyDown = 0;

  _caretVisible = false;
  _caretTime = 0;
	
  _quickSelectTime = 0;

  // FIXME: This flag should come from widget definition
  _editable = true;

  _editMode = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ListWidget::~ListWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::setSelected(Int32 item)
{
  assert(item >= -1 && item < (Int32)_list.size());

  if(isEnabled() && _selectedItem != item)
  {
    Int32 oldSelectedItem = _selectedItem;
    _selectedItem = item;

    if (_editMode)
    {
      // undo any changes made
      _list[oldSelectedItem] = _backupString;
      _editMode = false;
      drawCaret(true);
    }

    sendCommand(kListSelectionChangedCmd, _selectedItem);

    _currentPos = _selectedItem - _entriesPerPage / 2;
    scrollToCurrent();
    draw();

    _boss->instance()->frameBuffer().refresh();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::setList(const StringList& list)
{
  if (_editMode && _caretVisible)
    drawCaret(true);

  Int32 size = list.size();
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
void ListWidget::scrollTo(Int32 item)
{
  Int32 size = _list.size();
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
void ListWidget::scrollBarRecalc()
{
  _scrollBar->_numEntries = _list.size();
  _scrollBar->_entriesPerPage = _entriesPerPage;
  _scrollBar->_currentPos = _currentPos;
  _scrollBar->recalc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::handleTickle()
{
/*
	uint32 time = g_system->getMillis();
	if (_editMode && _caretTime < time) {
		_caretTime = time + kCaretBlinkTime;
		drawCaret(_caretVisible);
	}
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::handleMouseDown(Int32 x, Int32 y, Int32 button, Int32 clickCount)
{
  if (isEnabled())
  {
    Int32 oldSelectedItem = _selectedItem;
    _selectedItem = (y - 1) / kLineHeight + _currentPos;
    if (_selectedItem > (Int32)_list.size() - 1)
      _selectedItem = -1;

    if (oldSelectedItem != _selectedItem)
    {
      if (_editMode)
      {
        // undo any changes made
        _list[oldSelectedItem] = _backupString;
        _editMode = false;
        drawCaret(true);
      }
      sendCommand(kListSelectionChangedCmd, _selectedItem);
    }
    draw();
    _boss->instance()->frameBuffer().refresh();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::handleMouseUp(Int32 x, Int32 y, Int32 button, Int32 clickCount)
{
  // If this was a double click and the mouse is still over the selected item,
  // send the double click command
  if (clickCount == 2 && (_selectedItem == (y - 1) / kLineHeight + _currentPos))
    sendCommand(kListItemDoubleClickedCmd, _selectedItem);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::handleMouseWheel(Int32 x, Int32 y, Int32 direction)
{
  _scrollBar->handleMouseWheel(x, y, direction);
  _boss->instance()->frameBuffer().refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static bool matchingCharsIgnoringCase(string s, string pattern)
{
  // Make the strings uppercase so we can compare them
  transform(s.begin(), s.end(), s.begin(), (int(*)(int)) toupper);
  transform(pattern.begin(), pattern.end(), pattern.begin(), (int(*)(int)) toupper);

  uInt32 pos = s.find(pattern, 0);

  // Make sure that if the pattern is found, it occurs at the start of 's'
  return (pos != string::npos && pos < pattern.length());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ListWidget::handleKeyDown(uInt16 ascii, Int32 keycode, Int32 modifiers)
{
  bool handled = true;
  bool dirty = false;
  Int32 oldSelectedItem = _selectedItem;

  if (!_editMode && isalpha((char)ascii))
  {
    // Quick selection mode: Go to first list item starting with this key
    // (or a substring accumulated from the last couple key presses).
    // Only works in a useful fashion if the list entries are sorted.
    // TODO: Maybe this should be off by default, and instead we add a
    // method "enableQuickSelect()" or so ?
    uInt32 time = instance()->getTicks() / 1000;
    if (_quickSelectTime < time)
      _quickSelectStr = (char)ascii;
    else
      _quickSelectStr += (char)ascii;

    _quickSelectTime = time + 300;  // TODO: Turn this into a proper constant (kQuickSelectDelay ?)

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
    if (_caretVisible)
      drawCaret(true);

    switch (keycode)
    {
      case '\n':   // enter/return
      case '\r':
        // confirm edit and exit editmode
        _editMode = false;
        dirty = true;
        sendCommand(kListItemActivatedCmd, _selectedItem);
        break;
      case 27:  // escape
        // abort edit and exit editmode
        _editMode = false;
        dirty = true;
        _list[_selectedItem] = _backupString;
        break;
      case 8:		// backspace
        _list[_selectedItem].erase(_list[_selectedItem].length()-1);
        dirty = true;
        break;
      default:
        if (isprint((char)ascii))
        {
          _list[_selectedItem] += (char)ascii;
          dirty = true;
        }
        else
          handled = false;
    }
  }
  else  // not editmode
  {
    switch (keycode)
    {
      case '\n':   // enter/return
      case '\r':
        if (_selectedItem >= 0)
        {
          // override continuous enter keydown
          if (_editable && (_currentKeyDown != '\n' && _currentKeyDown != '\r'))
          {
            dirty = true;
            _editMode = true;
            _backupString = _list[_selectedItem];
          }
          else
            sendCommand(kListItemActivatedCmd, _selectedItem);
        }
        break;

      case 256+17:   // up arrow
        if (_selectedItem > 0)
          _selectedItem--;
        break;

      case 256+18:   // down arrow
        if (_selectedItem < (int)_list.size() - 1)
          _selectedItem++;
        break;

      case 256+24:   // pageup
        _selectedItem -= _entriesPerPage - 1;
        if (_selectedItem < 0)
          _selectedItem = 0;
        break;

      case 256+25:   // pagedown
        _selectedItem += _entriesPerPage - 1;
          if (_selectedItem >= (int)_list.size() )
            _selectedItem = _list.size() - 1;
          break;

      case 256+22:   // home
        _selectedItem = 0;
        break;

      case 256+23:   // end
        _selectedItem = _list.size() - 1;
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
    sendCommand(kListSelectionChangedCmd, _selectedItem);
    // also draw scrollbar
    _scrollBar->draw();
  }

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ListWidget::handleKeyUp(uInt16 ascii, Int32 keycode, Int32 modifiers)
{
  if (keycode == _currentKeyDown)
    _currentKeyDown = 0;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::lostFocusWidget()
{
  _editMode = false;
  drawCaret(true);
  draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::handleCommand(CommandSender* sender, uInt32 cmd, uInt32 data)
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
void ListWidget::drawWidget(bool hilite)
{
  FrameBuffer& fb = _boss->instance()->frameBuffer();
  int i, pos, len = _list.size();
  string buffer;

  // Draw a thin frame around the list.
  fb.hLine(_x, _y, _x + _w - 1, kColor);
  fb.hLine(_x, _y + _h - 1, _x + _w - 1, kShadowColor);
  fb.vLine(_x, _y, _y + _h - 1, kColor);

  // Draw the list items
  for (i = 0, pos = _currentPos; i < _entriesPerPage && pos < len; i++, pos++)
  {
    if (_numberingMode == kListNumberingZero || _numberingMode == kListNumberingOne)
    {
      char temp[10];
      sprintf(temp, "%2d. ", (pos + _numberingMode));
      buffer = temp;
      buffer += _list[pos];
    }
    else
      buffer = _list[pos];

    if (_selectedItem == pos)
    {
      if (_hasFocus)
        fb.fillRect(_x + 1, _y + 1 + kLineHeight * i, _w - 1, kLineHeight, kTextColorHi);
      else
        fb.frameRect(_x + 1, _y + 1 + kLineHeight * i, _w - 1, kLineHeight, kTextColorHi);
    }
    fb.font().drawString(buffer, _x + 2, _y + 2 + kLineHeight * i, _w - 4,
                        (_selectedItem == pos && _hasFocus) ? kBGColor : kTextColor);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 ListWidget::getCaretPos() const
{
  Int32 caretpos = 0;
  FrameBuffer& fb = _boss->instance()->frameBuffer();

  if (_numberingMode == kListNumberingZero || _numberingMode == kListNumberingOne)
  {
    char temp[10];
    sprintf(temp, "%2d. ", (_selectedItem + _numberingMode));
    caretpos += fb.font().getStringWidth(temp);
  }

  caretpos += fb.font().getStringWidth(_list[_selectedItem]);

  return caretpos;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::drawCaret(bool erase)
{
  // Only draw if item is visible
  if (_selectedItem < _currentPos || _selectedItem >= _currentPos + _entriesPerPage)
    return;
  if (!isVisible() || !_boss->isVisible())
    return;

  FrameBuffer& fb = _boss->instance()->frameBuffer();

  // The item is selected, thus _bgcolor is used to draw the caret and _textcolorhi to erase it
  OverlayColor color = erase ? kTextColorHi : kBGColor;
  int x = getAbsX() + 3;
  int y = getAbsY() + 1;

  y += (_selectedItem - _currentPos) * kLineHeight;
  x += getCaretPos();

  fb.vLine(x, y, y+kLineHeight, color);
	
  _caretVisible = !erase;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::scrollToCurrent()
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

  if (_currentPos < 0 || _entriesPerPage > (int)_list.size())
    _currentPos = 0;
  else if (_currentPos + _entriesPerPage > (int)_list.size())
    _currentPos = _list.size() - _entriesPerPage;

  _scrollBar->_currentPos = _currentPos;
  _scrollBar->recalc();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::startEditMode()
{
cerr << "ListWidget::startEditMode()\n";

  if (_editable && !_editMode && _selectedItem >= 0)
  {
    _editMode = true;
    _backupString = _list[_selectedItem];
    draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::abortEditMode()
{
cerr << "ListWidget::abortEditMode()\n";

  if (_editMode)
  {
    _editMode = false;
    _list[_selectedItem] = _backupString;
    drawCaret(true);
    draw();
  }
}
