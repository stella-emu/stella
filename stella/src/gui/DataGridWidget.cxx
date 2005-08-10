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
// $Id: DataGridWidget.cxx,v 1.18 2005-08-10 12:23:42 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <cctype>
#include <algorithm>

#include "OSystem.hxx"
#include "Widget.hxx"
#include "Dialog.hxx"
#include "Debugger.hxx"
#include "FrameBuffer.hxx"
#include "DataGridWidget.hxx"
#include "RamWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DataGridWidget::DataGridWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, int cols, int rows,
                               int colchars, int bits, BaseFormat base)
  : EditableWidget(boss, x, y, cols*(colchars * font.getMaxCharWidth() + 8) + 1,
                   font.getLineHeight()*rows + 1),
    CommandSender(boss),
    _rows(rows),
    _cols(cols),
    _currentRow(0),
    _currentCol(0),
    _rowHeight(font.getLineHeight()),
    _colWidth(colchars * font.getMaxCharWidth() + 8),
    _bits(bits),
    _base(base),
    _selectedItem(0)
{
  setFont(font);

  _flags = WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS;
  _type = kDataGridWidget;
  _editMode = false;

  _currentKeyDown = 0;

  // The item is selected, thus _bgcolor is used to draw the caret and
  // _textcolorhi to erase it
  _caretInverse = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DataGridWidget::~DataGridWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::setList(const IntArray& alist, const IntArray& vlist,
                             const BoolArray& changed)
{
/*
cerr << "alist.size() = "     << alist.size()
     << ", vlist.size() = "   << vlist.size()
     << ", changed.size() = " << changed.size()
     << ", _rows*_cols = "    << _rows * _cols << endl << endl;
*/
  int size = vlist.size();  // assume the alist is the same size
  assert(size == _rows * _cols);

  _addrList.clear();
  _addrStringList.clear();
  _valueList.clear();
  _valueStringList.clear();
  _changedList.clear();

  _addrList     = alist;
  _valueList    = vlist;
  _changedList  = changed;

  // An efficiency thing
  string temp;
  for(int i = 0; i < size; ++i)
  {
    temp = instance()->debugger().valueToString(_valueList[i], _base);
    _valueStringList.push_back(temp);
  }

/*
cerr << "_addrList.size() = "     << _addrList.size()
     << ", _valueList.size() = "   << _valueList.size()
     << ", _changedList.size() = " << _changedList.size()
     << ", _valueStringList.size() = " << _valueStringList.size()
     << ", _rows*_cols = "    << _rows * _cols << endl << endl;
*/
  _editMode = false;

  // Send item selected signal for starting with cell 0
  sendCommand(kDGSelectionChangedCmd, _selectedItem, _id);

  setDirty(); draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::setSelectedValue(int value)
{
  // Correctly format the data for viewing
  _editString = instance()->debugger().valueToString(value, _base);

  _valueStringList[_selectedItem] = _editString;
  _changedList[_selectedItem] = (_valueList[_selectedItem] != value);
  _valueList[_selectedItem] = value;

  sendCommand(kDGItemDataChangedCmd, _selectedItem, _id);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
  if (!isEnabled())
    return;

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
    _currentRow = _selectedItem / _cols;
    _currentCol = _selectedItem - (_currentRow * _cols);

    sendCommand(kDGSelectionChangedCmd, _selectedItem, _id);
    setDirty(); draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::handleMouseUp(int x, int y, int button, int clickCount)
{
  // If this was a double click and the mouse is still over the selected item,
  // send the double click command
  if (clickCount == 2 && (_selectedItem == findItem(x, y)))
  {
    sendCommand(kDGItemDoubleClickedCmd, _selectedItem, _id);

    // Start edit mode
    if(_editable && !_editMode)
      startEditMode();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int DataGridWidget::findItem(int x, int y)
{
  int row = (y - 1) / _rowHeight;
  if(row >= _rows) row = _rows - 1;

  int col = x / _colWidth;
  if(col >= _cols) col = _cols - 1;

  return row * _cols + col;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DataGridWidget::handleKeyDown(int ascii, int keycode, int modifiers)
{
  // Ignore all mod keys
  if(instance()->eventHandler().kbdControl(modifiers) ||
     instance()->eventHandler().kbdAlt(modifiers))
    return true;

  bool handled = true;
  bool dirty = false;

  if (_editMode)
  {
    // Class EditableWidget handles all text editing related key presses for us
    handled = EditableWidget::handleKeyDown(ascii, keycode, modifiers);
    if(handled)
      setDirty(); draw();
  }
  else
  {
    // not editmode
    switch (keycode)
    {
      case '\n':  // enter/return
      case '\r':
        if (_currentRow >= 0 && _currentCol >= 0)
        {
          dirty = true;
          _selectedItem = _currentRow*_cols + _currentCol;
          startEditMode();
        }
        break;

      case 256+17:  // up arrow
        if (_currentRow > 0)
        {
          _currentRow--;
          dirty = true;
        }
        else if(_currentCol > 0)
        {
          _currentRow = _rows - 1;
          _currentCol--;
          dirty = true;
        }
        break;

      case 256+18:  // down arrow
        if (_currentRow < (int) _rows - 1)
        {
          _currentRow++;
          dirty = true;
        }
        else if(_currentCol < (int) _cols - 1)
        {
          _currentRow = 0;
          _currentCol++;
          dirty = true;
        }
        break;

      case 256+20:  // left arrow
        if (_currentCol > 0)
        {
          _currentCol--;
          dirty = true;
        }
        else if(_currentRow > 0)
        {
          _currentCol = _cols - 1;
          _currentRow--;
          dirty = true;
        }
        break;

      case 256+19:  // right arrow
        if (_currentCol < (int) _cols - 1)
        {
          _currentCol++;
          dirty = true;
        }
        else if(_currentRow < (int) _rows - 1)
        {
          _currentCol = 0;
          _currentRow++;
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

      case 256+25:	// pagedown
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

      case 'n': // negate
        if(_editable)
        {
          negateCell();
          dirty = true;
        }
        break;

      case 'i': // invert
      case '!':
        if(_editable)
        {
          invertCell();
          dirty = true;
        }
        break;

      case '-': // decrement
        if(_editable)
        {
          decrementCell();
          dirty = true;
        }
        break;

      case '+': // increment
      case '=':
        if(_editable)
        {
          incrementCell();
          dirty = true;
        }
        break;

      case '<': // shift left
      case ',':
        if(_editable)
        {
          lshiftCell();
          dirty = true;
        }
        break;

      case '>': // shift right
      case '.':
        if(_editable)
        {
          rshiftCell();
          dirty = true;
        }
        break;

      case 'z': // zero
        if(_editable)
        {
          zeroCell();
          dirty = true;
        }
        break;

      default:
        handled = false;
    }
  }

  if (dirty)
  {
    int oldItem = _selectedItem;
    _selectedItem = _currentRow*_cols + _currentCol;

    if(_selectedItem != oldItem)
      sendCommand(kDGSelectionChangedCmd, _selectedItem, _id);

    setDirty(); draw();
  }

  _currentKeyDown = keycode;
  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DataGridWidget::handleKeyUp(int ascii, int keycode, int modifiers)
{
  if (keycode == _currentKeyDown)
    _currentKeyDown = 0;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::lostFocusWidget()
{
  _editMode = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::handleCommand(CommandSender* sender, int cmd,
                                   int data, int id)
{
  switch (cmd)
  {
    case kSetPositionCmd:
      if (_selectedItem != (int)data)
        _selectedItem = data;
      break;

    case kDGZeroCmd:
      zeroCell();
      break;

    case kDGInvertCmd:
      invertCell();
      break;

    case kDGNegateCmd:
      negateCell();
      break;

    case kDGIncCmd:
      incrementCell();
      break;

    case kDGDecCmd:
      decrementCell();
      break;

    case kDGShiftLCmd:
      lshiftCell();
      break;

    case kDGShiftRCmd:
      rshiftCell();
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::drawWidget(bool hilite)
{
  FrameBuffer& fb = _boss->instance()->frameBuffer();
  int row, col, deltax;
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
        if (_hasFocus && !_editMode)
          fb.fillRect(x - 4, y - 2, _colWidth+1, _rowHeight+1, kTextColorHi);
        else
          fb.frameRect(x - 4, y - 2, _colWidth+1, _rowHeight+1, kTextColorHi);
      }

      if (_selectedItem == pos && _editMode)
      {
        buffer = _editString;
        adjustOffset();
        deltax = -_editScrollOffset;

        fb.drawString(_font, buffer, x, y, _colWidth, kTextColor,
                      kTextAlignLeft, deltax, false);
      }
      else
      {
        buffer = _valueStringList[pos];
        deltax = 0;

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

  // Only draw the caret while editing, and if it's in the current viewport
  if(_editMode)
    drawCaret();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GUI::Rect DataGridWidget::getEditRect() const
{
  GUI::Rect r(1, 0, _colWidth, _rowHeight);
  const int rowoffset = _currentRow * _rowHeight;
  const int coloffset = _currentCol * _colWidth + 4;
  r.top += rowoffset;
  r.bottom += rowoffset;
  r.left += coloffset;
  r.right += coloffset - 5;

  return r;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::startEditMode()
{
  if (_editable && !_editMode && _selectedItem >= 0)
  {
    _editMode = true;
    setEditString("");  // Erase current entry when starting editing
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::endEditMode()
{
  if (!_editMode)
    return;

  // send a message that editing finished with a return/enter key press
  _editMode = false;

  // Update the both the string representation and the real data
  int value = instance()->debugger().stringToValue(_editString);
  if(value < 0 || value > (1 << _bits))
  {
    abortEditMode();
    return;
  }

  setSelectedValue(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::abortEditMode()
{
  // undo any changes made
  assert(_selectedItem >= 0);
  _editMode = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DataGridWidget::tryInsertChar(char c, int pos)
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::negateCell()
{
  int mask  = (1 << _bits) - 1;
  int value = getSelectedValue();

  value = ((~value) + 1) & mask;
  setSelectedValue(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::invertCell()
{
  int mask  = (1 << _bits) - 1;
  int value = getSelectedValue();

  value = ~value & mask;
  setSelectedValue(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::decrementCell()
{
  int mask  = (1 << _bits) - 1;
  int value = getSelectedValue();

  value = (value - 1) & mask;
  setSelectedValue(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::incrementCell()
{
  int mask  = (1 << _bits) - 1;
  int value = getSelectedValue();

  value = (value + 1) & mask;
  setSelectedValue(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::lshiftCell()
{
  int mask  = (1 << _bits) - 1;
  int value = getSelectedValue();

  value = (value << 1) & mask;
  setSelectedValue(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::rshiftCell()
{
  int mask  = (1 << _bits) - 1;
  int value = getSelectedValue();

  value = (value >> 1) & mask;
  setSelectedValue(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::zeroCell()
{
  setSelectedValue(0);
}
