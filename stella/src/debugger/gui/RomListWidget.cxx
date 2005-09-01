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
// $Id: RomListWidget.cxx,v 1.3 2005-09-01 19:14:09 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "ContextMenu.hxx"
#include "RomListWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomListWidget::RomListWidget(GuiObject* boss, const GUI::Font& font,
                                 int x, int y, int w, int h)
  : CheckListWidget(boss, font, x, y, w, h),
    myMenu(NULL),
    myHighlightedItem(-1)
{
  setFont(font);
  myMenu = new ContextMenu(this, font);

  StringList l;
  l.push_back("Add bookmark");
  l.push_back("Patch ROM");
  l.push_back("Save ROM");
  l.push_back("Set Breakpoint");
  l.push_back("Set PC");

  myMenu->setList(l);

  myLabelWidth  = font.getMaxCharWidth() * 16;
  myBytesWidth  = font.getMaxCharWidth() * 12;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomListWidget::~RomListWidget()
{
  delete myMenu;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListWidget::setList(StringList& label, StringList& bytes, StringList& disasm,
                            BoolArray& state)
{
  myLabel  = label;
  myDisasm = disasm;

  CheckListWidget::setList(bytes, state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
  // Grab right mouse button for context menu, send left to base class
  if(button == 2)
  {
    myMenu->setPos(x + getAbsX(), y + getAbsY());
    myMenu->show();
  }

  ListWidget::handleMouseDown(x, y, button, clickCount);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case kCMenuItemSelectedCmd:
      cerr << "RMB selected: " << myMenu->getSelectedString() << endl;
      break;

    default:
      ListWidget::handleCommand(sender, cmd, data, id);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListWidget::drawWidget(bool hilite)
{
//cerr << "RomListWidget::drawWidget\n";
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

    GUI::Rect l = getLineRect();
    GUI::Rect r = getEditRect();

    // Draw highlighted item in a frame
    if (_highlightedItem == pos)
    {
      fb.frameRect(_x + l.left - 3, _y + 1 + _rowHeight * i,
                   _w - l.left, _rowHeight,
                   kHiliteColor);
    }

    // Draw the selected item inverted, on a highlighted background.
    if (_selectedItem == pos && _hasFocus)
    {
      if (!_editMode)
        fb.fillRect(_x + r.left - 3, _y + 1 + _rowHeight * i,
                    r.width(), _rowHeight,
                    kTextColorHi);
      else
        fb.frameRect(_x + r.left - 3, _y + 1 + _rowHeight * i,
                     r.width(), _rowHeight,
                     kTextColorHi);
    }

    // Draw labels and actual disassembly
    fb.drawString(_font, myLabel[pos], _x + r.left - myLabelWidth, y,
                  myLabelWidth, kTextColor);

    fb.drawString(_font, myDisasm[pos], _x + r.right, y,
                  _w - r.right, kTextColor);

    // Draw editable bytes
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
GUI::Rect RomListWidget::getLineRect() const
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
GUI::Rect RomListWidget::getEditRect() const
{
  GUI::Rect r(2, 1, _w, _rowHeight);
  const int yoffset = (_selectedItem - _currentPos) * _rowHeight,
            xoffset = CheckboxWidget::boxSize() + 10;
  r.top    += yoffset;
  r.bottom += yoffset;
  r.left   += xoffset + myLabelWidth;
  r.right   = r.left + myBytesWidth;
	
  return r;
}
