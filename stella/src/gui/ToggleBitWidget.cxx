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
// $Id: ToggleBitWidget.cxx,v 1.6 2005-08-02 18:28:29 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "Widget.hxx"
#include "Dialog.hxx"
#include "Debugger.hxx"
#include "FrameBuffer.hxx"
#include "ToggleBitWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ToggleBitWidget::ToggleBitWidget(GuiObject* boss, const GUI::Font& font,
                                 int x, int y, int cols, int rows, int colchars)
  : Widget(boss, x, y, cols*(colchars * font.getMaxCharWidth() + 8) + 1,
           font.getLineHeight()*rows + 1),
    CommandSender(boss),
    _rows(rows),
    _cols(cols),
    _currentRow(0),
    _currentCol(0),
    _rowHeight(font.getLineHeight()),
    _colWidth(colchars * font.getMaxCharWidth() + 8),
    _selectedItem(0)
{
  setFont(font);

  _flags = WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS |
           WIDGET_TAB_NAVIGATE;
  _type = kToggleBitWidget;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ToggleBitWidget::~ToggleBitWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToggleBitWidget::setList(const StringList& off, const StringList& on)
{
  _offList.clear();
  _offList = off;
  _onList.clear();
  _onList = on;

  int size = _offList.size();  // assume _onList is the same size
  assert(size == _rows * _cols);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToggleBitWidget::setState(const BoolArray& state, const BoolArray& changed)
{
  _stateList.clear();
  _stateList = state;
  _changedList.clear();
  _changedList = changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToggleBitWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
  if (!isEnabled())
    return;

  // A click indicates this widget has been selected
  // It should receive focus (because it has the WIDGET_TAB_NAVIGATE property)
  receivedFocus();

  // First check whether the selection changed
  int newSelectedItem;
  newSelectedItem = findItem(x, y);
  if (newSelectedItem > (int)_stateList.size() - 1)
    newSelectedItem = -1;

  if (_selectedItem != newSelectedItem)
  {
    _selectedItem = newSelectedItem;
    _currentRow = _selectedItem / _cols;
    _currentCol = _selectedItem - (_currentRow * _cols);
  }
	
  setDirty(); draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToggleBitWidget::handleMouseUp(int x, int y, int button, int clickCount)
{
  // If this was a double click and the mouse is still over the selected item,
  // send the double click command
  if (clickCount == 2 && (_selectedItem == findItem(x, y)))
  {
    _stateList[_selectedItem] = !_stateList[_selectedItem];
    sendCommand(kTBItemDataChangedCmd, _selectedItem, _id);
    setDirty(); draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int ToggleBitWidget::findItem(int x, int y)
{
  int row = (y - 1) / _rowHeight;
  if(row >= _rows) row = _rows - 1;

  int col = x / _colWidth;
  if(col >= _cols) col = _cols - 1;

  return row * _cols + col;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ToggleBitWidget::handleKeyDown(int ascii, int keycode, int modifiers)
{
  // Ignore all mod keys
  if(instance()->eventHandler().kbdControl(modifiers) ||
     instance()->eventHandler().kbdAlt(modifiers))
    return true;

  bool handled = true;
  bool dirty = false, toggle = false;

  switch (keycode)
  {
	case '\n':  // enter/return
	case '\r':
	  if (_currentRow >= 0 && _currentCol >= 0)
	  {
		dirty = true;
        toggle = true;
	  }
	  break;

	case 256+17:  // up arrow
	  if (_currentRow > 0)
	  {
		_currentRow--;
		dirty = true;
	  }
	  break;

	case 256+18:  // down arrow
	  if (_currentRow < (int) _rows - 1)
	  {
		_currentRow++;
		dirty = true;
	  }
	  break;

	case 256+20:  // left arrow
	  if (_currentCol > 0)
	  {
		_currentCol--;
		dirty = true;
	  }
	  break;

	case 256+19:  // right arrow
	  if (_currentCol < (int) _cols - 1)
	  {
		_currentCol++;
		dirty = true;
	  }
	  break;

	case 256+24:  // pageup
	  if (_currentRow > 0)
	  {
		_currentRow = 0;
		dirty = true;
	  }
	  break;

	case 256+25:  // pagedown
	  if (_currentRow < (int) _rows - 1)
	  {
		_currentRow = _rows - 1;
		dirty = true;
	  }
	  break;

	case 256+22:  // home
	  if (_currentCol > 0)
	  {
		_currentCol = 0;
		dirty = true;
	  }
	  break;

	case 256+23:  // end
	  if (_currentCol < (int) _cols - 1)
	  {
		_currentCol = _cols - 1;
		dirty = true;
	  }
	  break;

	default:
	  handled = false;
  }

  if (dirty)
  {
    _selectedItem = _currentRow*_cols + _currentCol;

    if(toggle)
    {
      _stateList[_selectedItem] = !_stateList[_selectedItem];
      sendCommand(kTBItemDataChangedCmd, _selectedItem, _id);
    }

    setDirty(); draw();
  }

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToggleBitWidget::handleCommand(CommandSender* sender, int cmd,
                                    int data, int id)
{
  switch (cmd)
  {
    case kSetPositionCmd:
      if (_selectedItem != (int)data)
      {
        _selectedItem = data;
        setDirty(); draw();
      }
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToggleBitWidget::drawWidget(bool hilite)
{
cerr << "ToggleBitWidget::drawWidget\n";
  FrameBuffer& fb = instance()->frameBuffer();
  int row, col;
  string buffer;

  // Draw the internal grid and labels
  int linewidth = _cols * _colWidth;
  for (row = 0; row <= _rows; row++)
    fb.hLine(_x, _y + (row * _rowHeight), _x + linewidth, kColor);
  int lineheight = _rows * _rowHeight;
  for (col = 0; col <= _cols; col++)
    fb.vLine(_x + (col * _colWidth), _y, _y + lineheight, kColor);

  // Draw the list items
  for (row = 0; row < _rows; row++)
  {
    for (col = 0; col < _cols; col++)
    {
      int x = _x + 4 + (col * _colWidth);
      int y = _y + 2 + (row * _rowHeight);
      int pos = row*_cols + col;

      // Draw the selected item inverted, on a highlighted background.
      if (_currentRow == row && _currentCol == col)
      {
        if (_hasFocus)
          fb.fillRect(x - 4, y - 2, _colWidth+1, _rowHeight+1, kTextColorHi);
        else
          fb.frameRect(x - 4, y - 2, _colWidth+1, _rowHeight+1, kTextColorHi);
      }

      if(_stateList[pos])
        buffer = _onList[pos];
      else
        buffer = _offList[pos];

      // Highlight changes
      if(_changedList[pos])
      {
        fb.fillRect(x - 3, y - 1, _colWidth-1, _rowHeight-1, kTextColorEm);
        fb.drawString(_font, buffer, x, y, _colWidth, kTextColorHi);
      }
      else
        fb.drawString(_font, buffer, x, y, _colWidth, kTextColor);
    }
  }
}
