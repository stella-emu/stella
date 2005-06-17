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
// $Id: ByteGridWidget.cxx,v 1.5 2005-06-17 18:17:15 stephena Exp $
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
#include "ByteGridWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ByteGridWidget::ByteGridWidget(GuiObject* boss, int x, int y, int cols, int rows)
  : EditableWidget(boss, x, y, kColWidth*cols + 1, kLineHeight*rows + 1),
    CommandSender(boss),
    _rows(rows),
    _cols(cols),
    _currentRow(0),
    _currentCol(0),
    _selectedItem(0)
{
  // This widget always uses a monospace font
  setFont(instance()->consoleFont());

  _flags = WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS |
           WIDGET_TAB_NAVIGATE;
  _type = kByteGridWidget;
  _editMode = false;

  _currentKeyDown = 0;

  // The item is selected, thus _bgcolor is used to draw the caret and
  // _textcolorhi to erase it
  _caretInverse = true;

  // FIXME: This flag should come from widget definition
  _editable = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ByteGridWidget::~ByteGridWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ByteGridWidget::setList(const ByteAddrList& alist, const ByteValueList& vlist)
{
  _addrList.clear();
  _valueList.clear();
  _addrStringList.clear();
  _valueStringList.clear();

  _addrList = alist;
  _valueList = vlist;

  int size = _addrList.size();  // assume vlist is the same size
  assert(size == _rows * _cols);

  // An efficiency thing
  char temp[10];
  for(unsigned int i = 0; i < (unsigned int)size; ++i)
  {
    sprintf(temp, "%.4x:", _addrList[i]);
    _addrStringList.push_back(temp);
    sprintf(temp, "%.2x", _valueList[i]);
    _valueStringList.push_back(temp);
  }

  _editMode = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ByteGridWidget::handleMouseDown(int x, int y, int button, int clickCount)
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
    _currentRow = _selectedItem / _cols;
    _currentCol = _selectedItem - (_currentRow * _cols);

    sendCommand(kBGSelectionChangedCmd, _selectedItem);
    instance()->frameBuffer().refresh();
  }
	
  // TODO: Determine where inside the string the user clicked and place the
  // caret accordingly. See _editScrollOffset and EditTextWidget::handleMouseDown.
  draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ByteGridWidget::handleMouseUp(int x, int y, int button, int clickCount)
{
  // If this was a double click and the mouse is still over the selected item,
  // send the double click command
  if (clickCount == 2 && (_selectedItem == findItem(x, y)))
  {
    sendCommand(kBGItemDoubleClickedCmd, _selectedItem);

    // Start edit mode
    if(_editable && !_editMode)
      startEditMode();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int ByteGridWidget::findItem(int x, int y)
{
  int row = (y - 1) / kLineHeight;
  if(row >= _rows) row = _rows - 1;

  int col = x / kColWidth;
  if(col >= _cols) col = _cols - 1;

  return row * _cols + col;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ByteGridWidget::handleKeyDown(int ascii, int keycode, int modifiers)
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

      default:
        handled = false;
    }
  }

  if (dirty)
  {
    _selectedItem = _currentRow*_cols + _currentCol;
    draw();
    sendCommand(kBGSelectionChangedCmd, _selectedItem);
    instance()->frameBuffer().refresh();
  }

  _currentKeyDown = keycode;
  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ByteGridWidget::handleKeyUp(int ascii, int keycode, int modifiers)
{
  if (keycode == _currentKeyDown)
    _currentKeyDown = 0;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ByteGridWidget::lostFocusWidget()
{
  _editMode = false;
  draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ByteGridWidget::handleCommand(CommandSender* sender, int cmd, int data)
{
  switch (cmd)
  {
    case kSetPositionCmd:
      if (_selectedItem != (int)data)
      {
        _selectedItem = data;
        draw();
      }
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ByteGridWidget::drawWidget(bool hilite)
{
  FrameBuffer& fb = _boss->instance()->frameBuffer();
  int row, col, deltax;
  string buffer;

  // Draw the internal grid and labels
  int linewidth = _cols * kColWidth;
  for (row = 0; row <= _rows; row++)
    fb.hLine(_x, _y + (row * kLineHeight), _x + linewidth, kColor);
  int lineheight = _rows * kLineHeight;
  for (col = 0; col <= _cols; col++)
    fb.vLine(_x + (col * kColWidth), _y, _y + lineheight, kColor);

  // Draw the list items
  for (row = 0; row < _rows; row++)
  {
    for (col = 0; col < _cols; col++)
    {
      int x = _x + 4 + (col * kColWidth);
      int y = _y + 2 + (row * kLineHeight);
      int pos = row*_cols + col;

      // Draw the selected item inverted, on a highlighted background.
      if (_currentRow == row && _currentCol == col)
      {
        if (_hasFocus && !_editMode)
          fb.fillRect(x - 4, y - 2, kColWidth+1, kLineHeight+1, kTextColorHi);
        else
          fb.frameRect(x - 4, y - 2, kColWidth+1, kLineHeight+1, kTextColorHi);
      }

      if (_selectedItem == pos && _editMode)
      {
        buffer = _editString;
        adjustOffset();
        deltax = -_editScrollOffset;

        fb.drawString(_font, buffer, x, y, kColWidth, kTextColor,
                      kTextAlignLeft, deltax, false);
      }
      else
      {
        buffer = _valueStringList[pos];
        deltax = 0;
        fb.drawString(_font, buffer, x, y, kColWidth, kTextColor);
      }
    }
  }

  // Only draw the caret while editing, and if it's in the current viewport
  if(_editMode)
    drawCaret();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GUI::Rect ByteGridWidget::getEditRect() const
{
  GUI::Rect r(1, 0, kColWidth, kLineHeight);
  const int rowoffset = _currentRow * kLineHeight;
  const int coloffset = _currentCol * kColWidth + 4;
  r.top += rowoffset;
  r.bottom += rowoffset;
  r.left += coloffset;
  r.right += coloffset - 5;

  return r;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ByteGridWidget::startEditMode()
{
  if (_editable && !_editMode && _selectedItem >= 0)
  {
    _editMode = true;
    setEditString("");  // Erase current entry when starting editing
    draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ByteGridWidget::endEditMode()
{
  if (!_editMode)
    return;

  // send a message that editing finished with a return/enter key press
  _editMode = false;

  // Update the both the string representation and the real data
  int value = Debugger::hex_to_dec(_editString.c_str());
  if(_editString.length() == 0 || value < 0 || value > 255)
  {
    abortEditMode();
    return;
  }

  // Correctly format the data for viewing
  _editString = Debugger::to_hex_8(value);

  _valueStringList[_selectedItem] = _editString;
  _valueList[_selectedItem] = value;

  sendCommand(kBGItemDataChangedCmd, _selectedItem);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ByteGridWidget::abortEditMode()
{
  // undo any changes made
  assert(_selectedItem >= 0);
  _editMode = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ByteGridWidget::tryInsertChar(char c, int pos)
{
  // Not sure how efficient this is, or should we even care?
  c = tolower(c);
  if (c >= '0' && c <= '9' || c >= 'a' && c <= 'f')
  {
    _editString.insert(pos, 1, c);
    return true;
  }
  return false;
}
