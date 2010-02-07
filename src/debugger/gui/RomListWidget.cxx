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
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "bspf.hxx"
#include "ContextMenu.hxx"
#include "RomListWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomListWidget::RomListWidget(GuiObject* boss, const GUI::Font& font,
                             int x, int y, int w, int h)
  : CheckListWidget(boss, font, x, y, w, h),
    myMenu(NULL)
{
  _type = kRomListWidget;

  StringMap l;
//  l.push_back("Add bookmark");
  l.push_back("Save ROM", "saverom");
  l.push_back("Set PC", "setpc");
  myMenu = new ContextMenu(this, font, l);

  // Take advantage of a wide debugger window when possible
  const int fontWidth = font.getMaxCharWidth(),
            numchars = w / fontWidth;

  myLabelWidth = BSPF_max(16, int(0.35 * (numchars - 12))) * fontWidth;
  myBytesWidth = 12 * fontWidth;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomListWidget::~RomListWidget()
{
  delete myMenu;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListWidget::setList(const CartDebug::DisassemblyList& list,
                            const BoolArray& state)
{
  myList = &list;

  // TODO - maybe there's a better way than copying all the bytes again??
  StringList bytes;
  for(uInt32 i = 0; i < list.size(); ++i)
    bytes.push_back(list[i].bytes);
  CheckListWidget::setList(bytes, state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
  // Grab right mouse button for context menu, send left to base class
  if(button == 2)
  {
    // Add menu at current x,y mouse location
    myMenu->show(x + getAbsX(), y + getAbsY());
  }
  else
    ListWidget::handleMouseDown(x, y, button, clickCount);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RomListWidget::handleEvent(Event::Type e)
{
  return ListWidget::handleEvent(e); // override CheckListWidget::handleEvent()
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListWidget::drawWidget(bool hilite)
{
//cerr << "RomListWidget::drawWidget\n";
  FBSurface& s = _boss->dialog().surface();
  const CartDebug::DisassemblyList& dlist = *myList;
  int i, pos, xpos, ypos, len = dlist.size();
  string buffer;
  int deltax;

  // Draw a thin frame around the list and to separate columns
  s.hLine(_x, _y, _x + _w - 1, kColor);
  s.hLine(_x, _y + _h - 1, _x + _w - 1, kShadowColor);
  s.vLine(_x, _y, _y + _h - 1, kColor);

  s.vLine(_x + CheckboxWidget::boxSize() + 5, _y, _y + _h - 1, kColor);

  // Draw the list items
  GUI::Rect r = getEditRect();
  GUI::Rect l = getLineRect();
  xpos = _x + CheckboxWidget::boxSize() + 10;  ypos = _y + 2;
  for (i = 0, pos = _currentPos; i < _rows && pos < len; i++, pos++, ypos += _fontHeight)
  {
    // Draw checkboxes for correct lines (takes scrolling into account)
    _checkList[i]->setState(_stateList[pos]);
    _checkList[i]->setDirty();
    _checkList[i]->draw();

    // Draw highlighted item in a frame
    if (_highlightedItem == pos)
    {
      s.frameRect(_x + l.left - 3, _y + 1 + _fontHeight * i,
                  _w - l.left, _fontHeight, kDbgColorHi);
    }

    // Draw the selected item inverted, on a highlighted background.
    if (_selectedItem == pos && _hasFocus)
    {
      if (!_editMode)
        s.fillRect(_x + r.left - 3, _y + 1 + _fontHeight * i,
                   r.width(), _fontHeight, kTextColorHi);
      else
        s.frameRect(_x + r.left - 3, _y + 1 + _fontHeight * i,
                    r.width(), _fontHeight, kTextColorHi);
    }

    // Draw labels
    s.drawString(_font, dlist[pos].label, xpos, ypos,
                 myLabelWidth, kTextColor);

    // Draw disassembly
    s.drawString(_font, dlist[pos].disasm, xpos + myLabelWidth, ypos,
                 r.left, kTextColor);

    // Draw editable bytes
    if (_selectedItem == pos && _editMode)
    {
      buffer = _editString;
      adjustOffset();
      deltax = -_editScrollOffset;

      s.drawString(_font, buffer, _x + r.left, ypos, r.width(), kTextColor,
                   kTextAlignLeft, deltax, false);
    }
    else
    {
      buffer = _list[pos];
      deltax = 0;
      s.drawString(_font, buffer, _x + r.left, ypos, r.width(), kTextColor);
    }
  }

  // Only draw the caret while editing, and if it's in the current viewport
  if(_editMode && (_selectedItem >= _scrollBar->_currentPos) &&
    (_selectedItem < _scrollBar->_currentPos + _rows))
    drawCaret();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GUI::Rect RomListWidget::getLineRect() const
{
  GUI::Rect r(2, 1, _w, _fontHeight);
  const int yoffset = (_selectedItem - _currentPos) * _fontHeight,
            xoffset = CheckboxWidget::boxSize() + 10;
  r.top    += yoffset;
  r.bottom += yoffset;
  r.left  += xoffset;
  r.right -= xoffset - 15;
	
  return r;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GUI::Rect RomListWidget::getEditRect() const
{
  GUI::Rect r(2, 1, _w, _fontHeight);
  const int yoffset = (_selectedItem - _currentPos) * _fontHeight;
  r.top    += yoffset;
  r.bottom += yoffset;
  r.left   += _w - myBytesWidth;
  r.right   = _w;
	
  return r;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RomListWidget::tryInsertChar(char c, int pos)
{
  // Not sure how efficient this is, or should we even care?
  c = tolower(c);
  if((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
     c == '%' || c == '#' || c == '$' || c == ' ')
  {
    _editString.insert(pos, 1, c);
    return true;
  }
  else
    return false;
}
