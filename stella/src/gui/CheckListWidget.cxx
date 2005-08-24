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
// $Id: CheckListWidget.cxx,v 1.5 2005-08-24 13:18:02 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "ScrollBarWidget.hxx"
#include "CheckListWidget.hxx"
#include "Widget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheckListWidget::CheckListWidget(GuiObject* boss, const GUI::Font& font,
                                 int x, int y, int w, int h)
  : ListWidget(boss, font, x, y, w, h)
{
  int ypos = _y + 2;

  // rowheight is determined by largest item on a line
  _rowHeight = MAX(_rowHeight, CheckboxWidget::boxSize());

  // Create a CheckboxWidget for each row in the list
  CheckboxWidget* t;
  for(int i = 0; i < _rows; ++i)
  {
    t = new CheckboxWidget(boss, font, _x + 2, ypos, "", kCheckActionCmd);
    t->setTarget(this);
    t->setID(i);
    t->holdFocus(false);
    ypos += _rowHeight;

    _checkList.push_back(t);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheckListWidget::~CheckListWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckListWidget::setStyle(CheckStyle style)
{
  for(unsigned int i = 0; i < _checkList.size(); ++i)
  {
    if(style == kXFill)
    {
      _checkList[i]->drawBox(true);
      _checkList[i]->setFill(false);
      _checkList[i]->setColor(kTextColor);
    }
    else if(style == kSolidFill)
    {
      _checkList[i]->drawBox(false);
      _checkList[i]->setFill(true);
      _checkList[i]->setColor(kTextColorEm);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckListWidget::setList(const StringList& list, const BoolArray& state)
{
  _list = list;
  _stateList = state;

  assert(_list.size() == _stateList.size());

  // Enable all checkboxes
  for(int i = 0; i < _rows; ++i)
    _checkList[i]->setFlags(WIDGET_ENABLED);

  // Then turn off any extras
  if((int)_stateList.size() < _rows)
    for(int i = _stateList.size(); i < _rows; ++i)
      _checkList[i]->clearFlags(WIDGET_ENABLED);

  ListWidget::recalc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckListWidget::setLine(int line, const string& str, const bool& state)
{
  if(line >= (int)_list.size())
    return;

  _list[line]      = str;
  _stateList[line] = state;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckListWidget::drawWidget(bool hilite)
{
//cerr << "CheckListWidget::drawWidget\n";
  FrameBuffer& fb = _boss->instance()->frameBuffer();
  int i, pos, len = _list.size();
  string buffer;
  int deltax;

  // Draw a thin frame around the list and to separate columns
  fb.hLine(_x, _y, _x + _w - 1, kColor);
  fb.hLine(_x, _y + _h - 1, _x + _w - 1, kShadowColor);
  fb.vLine(_x, _y, _y + _h - 1, kColor);

  fb.vLine(_x + CheckboxWidget::boxSize() + 5, _y, _y + _h - 1, kColor);

  // Draw the list items
  for (i = 0, pos = _currentPos; i < _rows && pos < len; i++, pos++)
  {
    // Draw checkboxes for correct lines (takes scrolling into account)
    _checkList[i]->setState(_stateList[pos]);
    _checkList[i]->setDirty();
    _checkList[i]->draw();

    const int y = _y + 2 + _rowHeight * i;

    GUI::Rect r(getEditRect());

    // Draw the selected item inverted, on a highlighted background.
    if (_selectedItem == pos)
    {
      if (_hasFocus && !_editMode)
        fb.fillRect(_x + r.left - 3, _y + 1 + _rowHeight * i,
                    _w - r.left, _rowHeight,
                    kTextColorHi);
      else
        fb.frameRect(_x + r.left - 3, _y + 1 + _rowHeight * i,
                     _w - r.left, _rowHeight,
                     kTextColorHi);
    }

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
  GUI::Rect r(2, 1, _w, _rowHeight);
  const int yoffset = (_selectedItem - _currentPos) * _rowHeight,
            xoffset = CheckboxWidget::boxSize() + 10;
  r.top    += yoffset;
  r.bottom += yoffset;
  r.left  += xoffset;
  r.right -= xoffset - 15;
	
  return r;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckListWidget::handleCommand(CommandSender* sender, int cmd,
                                    int data, int id)
{
  switch(cmd)
  {
    case kCheckActionCmd:
    {
      // Figure out which line has been checked
      int line = _currentPos + id;
      _stateList[line] = bool(data);

      // Let the boss know about it
      sendCommand(kListItemChecked, line, _id);
      break;
    }

    default:
      ListWidget::handleCommand(sender, cmd, data, id);
  }
}
