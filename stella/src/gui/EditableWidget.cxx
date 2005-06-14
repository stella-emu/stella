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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: EditableWidget.cxx,v 1.1 2005-06-14 01:11:48 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "EditableWidget.hxx"


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EditableWidget::EditableWidget(GuiObject* boss, int x, int y, int w, int h)
  : Widget(boss, x, y, w, h)
{
  _caretVisible = false;
  _caretTime = 0;
  _caretPos = 0;	// FIXME

  _caretInverse = false;

  _editScrollOffset = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EditableWidget::~EditableWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditableWidget::setEditString(const string& str)
{
  // TODO: We probably should filter the input string here,
  // e.g. using tryInsertChar.
  _editString = str;
  _caretPos = _editString.size();

  _editScrollOffset = (_font->getStringWidth(_editString) - (getEditRect().width()));
  if (_editScrollOffset < 0)
    _editScrollOffset = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::tryInsertChar(char c, int pos)
{
  if (isprint(c))
  {
    _editString.insert(pos, c, 1);
    return true;
  }
  return false;
}

/*
void EditableWidget::handleTickle() {
	uint32 time = getMillis();
	if (_caretTime < time) {
		_caretTime = time + kCaretBlinkTime;
		drawCaret(_caretVisible);
	}
}
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::handleKeyDown(int ascii, int keycode, int modifiers)
{
  bool handled = true;
  bool dirty = false;

  // First remove caret
  if (_caretVisible)
    drawCaret(true);

  switch (keycode)
  {
    case '\n':  // enter/return
    case '\r':
      // confirm edit and exit editmode
      endEditMode();
      dirty = true;
      break;

    case 27:    // escape
      abortEditMode();
      dirty = true;
      break;

    case 8:     // backspace
      if (_caretPos > 0)
      {
        _caretPos--;
        _editString.erase(_caretPos);
        dirty = true;
      }
      break;

    case 127:   // delete
      _editString.erase(_caretPos);
      dirty = true;
      break;

    case 256 + 20:  // left arrow
		if (_caretPos > 0)
          dirty = setCaretPos(_caretPos - 1);
        break;

    case 256 + 19:  // right arrow
      if (_caretPos < (int)_editString.size())
        dirty = setCaretPos(_caretPos + 1);
      break;

    case 256 + 22:  // home
      dirty = setCaretPos(0);
      break;

    case 256 + 23:  // end
      dirty = setCaretPos(_editString.size());
      break;

    default:
      if (tryInsertChar((char)ascii, _caretPos))
      {
	    _caretPos++;
        dirty = true;
      }
      else
        handled = false;
  }

  if (dirty)
    draw();

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int EditableWidget::getCaretOffset() const
{
  int caretpos = 0;
  for (int i = 0; i < _caretPos; i++)
    caretpos += _font->getCharWidth(_editString[i]);

  caretpos -= _editScrollOffset;

  return caretpos;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditableWidget::drawCaret(bool erase)
{
  // Only draw if item is visible
  if (!isVisible() || !_boss->isVisible())
    return;

  GUI::Rect editRect = getEditRect();

  OverlayColor color = (erase ^ _caretInverse) ? kBGColor : kTextColorHi;
  int x = editRect.left;
  int y = editRect.top + 1;

  x += getCaretOffset();

  if (y < 0 || y + editRect.height() - 2 >= _h)
    return;

  x += getAbsX();
  y += getAbsY();

  FrameBuffer& fb = _boss->instance()->frameBuffer();
  fb.vLine(x, y, y + editRect.height() - 2, color);
  fb.refresh();//FIXMEfb.addDirtyRect(x, y, 2, editRect.height() - 2);

  _caretVisible = !erase;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::setCaretPos(int newPos)
{
  assert(newPos >= 0 && newPos <= (int)_editString.size());
  _caretPos = newPos;

  return adjustOffset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::adjustOffset()
{
  // check if the caret is still within the textbox; if it isn't,
  // adjust _editScrollOffset 

  int caretpos = getCaretOffset();
  const int editWidth = getEditRect().width();

  if (caretpos < 0)
  {
    // scroll left
    _editScrollOffset += caretpos;
    return true;
  }
  else if (caretpos >= editWidth)
  {
    // scroll right
    _editScrollOffset -= (editWidth - caretpos);
    return true;
  }
  else if (_editScrollOffset > 0)
  {
    const int strWidth = _font->getStringWidth(_editString);
    if (strWidth - _editScrollOffset < editWidth)
    {
      // scroll right
      _editScrollOffset = (strWidth - editWidth);
      if (_editScrollOffset < 0)
        _editScrollOffset = 0;
    }
  }

  return false;
}
