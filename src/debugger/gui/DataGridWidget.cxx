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

#include "OSystem.hxx"
#include "Widget.hxx"
#include "Dialog.hxx"
#include "Debugger.hxx"
#include "FrameBuffer.hxx"
#include "DataGridWidget.hxx"
#include "DataGridOpsWidget.hxx"
#include "RamWidget.hxx"
#include "ScrollBarWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DataGridWidget::DataGridWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, int cols, int rows,
                               int colchars, int bits, BaseFormat base,
                               bool useScrollbar)
  : EditableWidget(boss, font, x, y,
                   cols*(colchars * font.getMaxCharWidth() + 8) + 1,
                   font.getLineHeight()*rows + 1),
    _rows(rows),
    _cols(cols),
    _currentRow(0),
    _currentCol(0),
    _rowHeight(font.getLineHeight()),
    _colWidth(colchars * font.getMaxCharWidth() + 8),
    _bits(bits),
    _base(base),
    _selectedItem(0),
    _currentKeyDown(KBDK_UNKNOWN),
    _opsWidget(NULL),
    _scrollBar(NULL)
{
  _flags = WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS |
           WIDGET_WANTS_RAWDATA;
  _type = kDataGridWidget;
  _editMode = false;

  // The item is selected, thus _bgcolor is used to draw the caret and
  // _textcolorhi to erase it
  _caretInverse = true;

  // Make sure hilite list contains all false values
  _hiliteList.clear();
  int size = _rows * _cols;
  while(size--)
    _hiliteList.push_back(false);

  // Set lower and upper bounds to sane values
  setRange(0, 1 << bits);

  // Add a scrollbar if necessary
  if(useScrollbar)
  {
    _scrollBar = new ScrollBarWidget(boss, font, _x + _w, _y, kScrollBarWidth, _h);
    _scrollBar->setTarget(this);
    _scrollBar->_numEntries = 1;
    _scrollBar->_currentPos = 0;
    _scrollBar->_entriesPerPage = 1;
    _scrollBar->_wheel_lines = 1;
  }
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

  _addrList    = alist;
  _valueList   = vlist;
  _changedList = changed;

  // An efficiency thing
  string temp;
  for(int i = 0; i < size; ++i)
  {
    temp = instance().debugger().valueToString(_valueList[i], _base);
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
void DataGridWidget::setList(int a, int v, bool c)
{
  IntArray alist, vlist;
  BoolArray changed;

  alist.push_back(a);
  vlist.push_back(v);
  changed.push_back(c);

  setList(alist, vlist, changed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::setList(int a, int v)
{
  IntArray alist, vlist;
  BoolArray changed;

  alist.push_back(a);
  vlist.push_back(v);
  bool diff = _addrList.size() == 1 ? getSelectedValue() != v : false;
  changed.push_back(diff);

  setList(alist, vlist, changed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::setHiliteList(const BoolArray& hilitelist)
{
  assert(hilitelist.size() == uInt32(_rows * _cols));
  _hiliteList.clear();
  _hiliteList = hilitelist;

  setDirty(); draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::setNumRows(int rows)
{
  if(_scrollBar)
    _scrollBar->_numEntries = rows;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::setSelectedValue(int value)
{
  setValue(_selectedItem, value, _valueList[_selectedItem] != value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::setValue(int position, int value)
{
  setValue(position, value, _valueList[position] != value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::setValue(int position, int value, bool changed)
{
  if(position >= 0 && uInt32(position) < _valueList.size())
  {
    // Correctly format the data for viewing
    _editString = instance().debugger().valueToString(value, _base);

    _valueStringList[position] = _editString;
    _changedList[position] = changed;
    _valueList[position] = value;

    sendCommand(kDGItemDataChangedCmd, position, _id);

    setDirty(); draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::setRange(int lower, int upper)
{
  _lowerBound = BSPF_max(0, lower);
  _upperBound = BSPF_min(1 << _bits, upper);
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
void DataGridWidget::handleMouseWheel(int x, int y, int direction)
{
  if(_scrollBar)
    _scrollBar->handleMouseWheel(x, y, direction);
  else if(_editable)
  {
    if(direction > 0)
      decrementCell();
    else if(direction < 0)
      incrementCell();
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
bool DataGridWidget::handleKeyDown(StellaKey key, StellaMod mod, char ascii)
{
  // Ignore all mod keys
  if(instance().eventHandler().kbdControl(mod) ||
     instance().eventHandler().kbdAlt(mod))
    return true;

  bool handled = true;
  bool dirty = false;

  if (_editMode)
  {
    // Class EditableWidget handles all text editing related key presses for us
    handled = EditableWidget::handleKeyDown(key, mod, ascii);
    if(handled)
      setDirty(); draw();
  }
  else
  {
    // not editmode
    switch(key)
    {
      case KBDK_RETURN:
        if (_currentRow >= 0 && _currentCol >= 0)
        {
          dirty = true;
          _selectedItem = _currentRow*_cols + _currentCol;
          startEditMode();
        }
        break;

      case KBDK_UP:
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

      case KBDK_DOWN:
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

      case KBDK_LEFT:
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

      case KBDK_RIGHT:
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

      case KBDK_PAGEUP:
        if(instance().eventHandler().kbdShift(mod) && _scrollBar)
          handleMouseWheel(0, 0, -1);
        else if (_currentRow > 0)
        {
          _currentRow = 0;
          dirty = true;
        }
        break;

      case KBDK_PAGEDOWN:
        if(instance().eventHandler().kbdShift(mod) && _scrollBar)
          handleMouseWheel(0, 0, +1);
        else if (_currentRow < (int) _rows - 1)
        {
          _currentRow = _rows - 1;
          dirty = true;
        }
        break;

      case KBDK_HOME:
        if (_currentCol > 0)
        {
          _currentCol = 0;
          dirty = true;
        }
        break;

      case KBDK_END:
        if (_currentCol < (int) _cols - 1)
        {
          _currentCol = _cols - 1;
          dirty = true;
        }
        break;

      case KBDK_n: // negate
        if(_editable)
          negateCell();
        break;

      default:
        handled = false;
    }
    if(!handled)
    {
      handled = true;

      switch(ascii)
      {
        case 'i': // invert
        case '!':
          if(_editable)
            invertCell();
          break;

        case '-': // decrement
          if(_editable)
            decrementCell();
          break;

        case '+': // increment
        case '=':
          if(_editable)
            incrementCell();
          break;

        case '<': // shift left
        case ',':
          if(_editable)
            lshiftCell();
          break;

        case '>': // shift right
        case '.':
          if(_editable)
            rshiftCell();
          break;

        case 'z': // zero
          if(_editable)
            zeroCell();
          break;

        default:
          handled = false;
      }
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

  _currentKeyDown = key;
  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DataGridWidget::handleKeyUp(StellaKey key, StellaMod mod, char ascii)
{
  if (key == _currentKeyDown)
    _currentKeyDown = KBDK_UNKNOWN;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::receivedFocusWidget()
{
  // Enable the operations widget and make it send its signals here
  if(_opsWidget)
  {
    _opsWidget->setEnabled(true);
    _opsWidget->setTarget(this);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::lostFocusWidget()
{
  _editMode = false;

  // Disable the operations widget
  if(_opsWidget)
    _opsWidget->setEnabled(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::handleCommand(CommandSender* sender, int cmd,
                                   int data, int id)
{
  switch (cmd)
  {
    case kSetPositionCmd:
      // Chain access; pass to parent
      sendCommand(kSetPositionCmd, data, _id);
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
//cerr << "DataGridWidget::drawWidget\n";
  FBSurface& s = _boss->dialog().surface();
  int row, col, deltax;

  // Draw the internal grid and labels
  int linewidth = _cols * _colWidth;
  for (row = 0; row <= _rows; row++)
    s.hLine(_x, _y + (row * _rowHeight), _x + linewidth, kColor);
  int lineheight = _rows * _rowHeight;
  for (col = 0; col <= _cols; col++)
    s.vLine(_x + (col * _colWidth), _y, _y + lineheight, kColor);

  // Draw the list items
  for (row = 0; row < _rows; row++)
  {
    for (col = 0; col < _cols; col++)
    {
      int x = _x + 4 + (col * _colWidth);
      int y = _y + 2 + (row * _rowHeight);
      int pos = row*_cols + col;

      // Draw the selected item inverted, on a highlighted background.
      if (_currentRow == row && _currentCol == col &&
          _hasFocus && !_editMode)
        s.fillRect(x - 4, y - 2, _colWidth+1, _rowHeight+1, kTextColorHi);

      if (_selectedItem == pos && _editMode)
      {
        adjustOffset();
        deltax = -_editScrollOffset;

        s.drawString(_font, _editString, x, y, _colWidth, kTextColor,
                     kTextAlignLeft, deltax, false);
      }
      else
      {
        deltax = 0;

        uInt32 color = kTextColor;
        if(_changedList[pos])
        {
          s.fillRect(x - 3, y - 1, _colWidth-1, _rowHeight-1, kDbgChangedColor);

          if(_hiliteList[pos])
            color = kDbgColorHi;
          else
            color = kDbgChangedTextColor;
        }
        else if(_hiliteList[pos])
          color = kDbgColorHi;

        s.drawString(_font, _valueStringList[pos], x, y, _colWidth, color);
      }
    }
  }

  // Only draw the caret while editing, and if it's in the current viewport
  if(_editMode)
    drawCaret();

  // Draw the scrollbar
  if(_scrollBar)
    _scrollBar->recalc();  // takes care of the draw
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
int DataGridWidget::getWidth() const
{
  return _w + (_scrollBar ? kScrollBarWidth : 0);
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
  int value = instance().debugger().stringToValue(_editString);
  if(value < _lowerBound || value >= _upperBound)
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
     c == '\\' || c == '#' || c == '$')
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
  if(mask != _upperBound - 1)     // ignore when values aren't byte-aligned
    return;

  value = ((~value) + 1) & mask;
  setSelectedValue(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::invertCell()
{
  int mask  = (1 << _bits) - 1;
  int value = getSelectedValue();
  if(mask != _upperBound - 1)     // ignore when values aren't byte-aligned
    return;

  value = ~value & mask;
  setSelectedValue(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::decrementCell()
{
  int mask  = (1 << _bits) - 1;
  int value = getSelectedValue();
  if(value <= _lowerBound)        // take care of wrap-around
    value = _upperBound;

  value = (value - 1) & mask;
  setSelectedValue(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::incrementCell()
{
  int mask  = (1 << _bits) - 1;
  int value = getSelectedValue();
  if(value >= _upperBound - 1)    // take care of wrap-around
    value = _lowerBound - 1;

  value = (value + 1) & mask;
  setSelectedValue(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::lshiftCell()
{
  int mask  = (1 << _bits) - 1;
  int value = getSelectedValue();
  if(mask != _upperBound - 1)     // ignore when values aren't byte-aligned
    return;

  value = (value << 1) & mask;
  setSelectedValue(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::rshiftCell()
{
  int mask  = (1 << _bits) - 1;
  int value = getSelectedValue();
  if(mask != _upperBound - 1)     // ignore when values aren't byte-aligned
    return;

  value = (value >> 1) & mask;
  setSelectedValue(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridWidget::zeroCell()
{
  setSelectedValue(0);
}
