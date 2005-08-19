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
// $Id: TogglePixelWidget.cxx,v 1.1 2005-08-19 15:05:09 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "Widget.hxx"
#include "Dialog.hxx"
#include "Debugger.hxx"
#include "FrameBuffer.hxx"
#include "TogglePixelWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TogglePixelWidget::TogglePixelWidget(GuiObject* boss, int x, int y,
                                 int cols, int rows)
  : ToggleWidget(boss, x, y, cols, rows),
    _pixelColor(kBGColor)
{
  _rowHeight = _font->getLineHeight();
  _colWidth  = 15;

  // Calculate real dimensions
  _w = _colWidth  * cols + 1;
  _h = _rowHeight * rows + 1;

  // Changed state isn't used, but we still need to fill it
  while((int)_changedList.size() < rows * cols)
    _changedList.push_back(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TogglePixelWidget::~TogglePixelWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TogglePixelWidget::setState(const BoolArray& state)
{
  _stateList.clear();
  _stateList = state;

  setDirty(); draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TogglePixelWidget::drawWidget(bool hilite)
{
  FrameBuffer& fb = instance()->frameBuffer();
  int row, col;

  // Draw the internal grid and labels
  int linewidth = _cols * _colWidth;
  for (row = 0; row <= _rows; row++)
    fb.hLine(_x, _y + (row * _rowHeight), _x + linewidth, kColor);
  int lineheight = _rows * _rowHeight;
  for (col = 0; col <= _cols; col++)
    fb.vLine(_x + (col * _colWidth), _y, _y + lineheight, kColor);

  // Draw the pixels
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

      // Either draw the pixel in given color, or erase (show background)
      if(_stateList[pos])
        fb.fillRect(x - 3, y - 1, _colWidth-1, _rowHeight-1, _pixelColor);
      else
        fb.fillRect(x - 3, y - 1, _colWidth-1, _rowHeight-1, kBGColor);
    }
  }
}
