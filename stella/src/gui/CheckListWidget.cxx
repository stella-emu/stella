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
// $Id: CheckListWidget.cxx,v 1.2 2005-08-22 18:17:10 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "ScrollBarWidget.hxx"
#include "CheckListWidget.hxx"

#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheckListWidget::CheckListWidget(GuiObject* boss, const GUI::Font& font,
                                 int x, int y, int w, int h)
  : ListWidget(boss, font, x, y, w, h)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheckListWidget::~CheckListWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckListWidget::drawWidget(bool hilite)
{
cerr << "CheckListWidget::drawWidget\n";
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

/*
  if (_numberingMode != kListNumberingOff)
  {
    char temp[10];
    // FIXME: Assumes that all digits have the same width.
    sprintf(temp, "%2d. ", (_list.size() - 1 + _numberingMode));
    r.left += _font->getStringWidth(temp);
  }
*/
	
  return r;
}
