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
// Copyright (c) 1995-2006 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: ToggleBitWidget.cxx,v 1.5 2006-12-08 16:49:18 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "Widget.hxx"
#include "Dialog.hxx"
#include "Debugger.hxx"
#include "FrameBuffer.hxx"
#include "StringList.hxx"
#include "ToggleBitWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ToggleBitWidget::ToggleBitWidget(GuiObject* boss, const GUI::Font& font,
                                 int x, int y, int cols, int rows, int colchars)
  : ToggleWidget(boss, font, x, y, cols, rows)
{
  _type = kToggleBitWidget;

  _rowHeight = font.getLineHeight();
  _colWidth  = colchars * font.getMaxCharWidth() + 8;

  // Calculate real dimensions
  _w = _colWidth  * cols + 1;
  _h = _rowHeight * rows + 1;
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

  setDirty(); draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToggleBitWidget::drawWidget(bool hilite)
{
//cerr << "ToggleBitWidget::drawWidget\n";
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
      if (_currentRow == row && _currentCol == col && _hasFocus)
        fb.fillRect(x - 4, y - 2, _colWidth+1, _rowHeight+1, kTextColorHi);

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
