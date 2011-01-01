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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
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

#include "OSystem.hxx"
#include "Widget.hxx"
#include "ToggleWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ToggleWidget::ToggleWidget(GuiObject* boss, const GUI::Font& font,
                           int x, int y, int cols, int rows)
  : Widget(boss, font, x, y, 16, 16),
    CommandSender(boss),
    _rows(rows),
    _cols(cols),
    _currentRow(0),
    _currentCol(0),
    _selectedItem(0),
    _editable(true)
{
  _flags = WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS |
           WIDGET_WANTS_RAWDATA;
  _type = kToggleWidget;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ToggleWidget::~ToggleWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToggleWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
  if (!isEnabled())
    return;

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
    setDirty(); draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToggleWidget::handleMouseUp(int x, int y, int button, int clickCount)
{
  if (!isEnabled() || !_editable)
    return;

  // If this was a double click and the mouse is still over the selected item,
  // send the double click command
  if (clickCount == 2 && (_selectedItem == findItem(x, y)))
  {
    _stateList[_selectedItem] = !_stateList[_selectedItem];
    _changedList[_selectedItem] = !_changedList[_selectedItem];
    sendCommand(kTWItemDataChangedCmd, _selectedItem, _id);
    setDirty(); draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int ToggleWidget::findItem(int x, int y)
{
  int row = (y - 1) / _rowHeight;
  if(row >= _rows) row = _rows - 1;

  int col = x / _colWidth;
  if(col >= _cols) col = _cols - 1;

  return row * _cols + col;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ToggleWidget::handleKeyDown(int ascii, int keycode, int modifiers)
{
  // Ignore all mod keys
  if(instance().eventHandler().kbdControl(modifiers) ||
     instance().eventHandler().kbdAlt(modifiers))
    return true;

  bool handled = true;
  bool dirty = false, toggle = false;

  switch(ascii)
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

    if(toggle && _editable)
    {
      _stateList[_selectedItem] = !_stateList[_selectedItem];
      _changedList[_selectedItem] = !_changedList[_selectedItem];
      sendCommand(kTWItemDataChangedCmd, _selectedItem, _id);
    }

    setDirty(); draw();
  }

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToggleWidget::handleCommand(CommandSender* sender, int cmd,
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
