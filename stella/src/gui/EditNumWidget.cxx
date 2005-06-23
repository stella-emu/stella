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
// $Id: EditNumWidget.cxx,v 1.5 2005-06-23 14:33:11 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "Dialog.hxx"
#include "EditNumWidget.hxx"


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EditNumWidget::EditNumWidget(GuiObject* boss, int x, int y, int w, int h,
                               const string& text)
  : EditableWidget(boss, x, y - 1, w, h + 2)
{
  _flags = WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS |
           WIDGET_TAB_NAVIGATE;
  _type = kEditTextWidget;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditNumWidget::setEditString(const string& str)
{
  EditableWidget::setEditString(str);
  _backupString = str;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditNumWidget::tryInsertChar(char c, int pos)
{
  // Not sure how efficient this is, or should we even care?
  c = tolower(c);
  if((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
     c == '%' || c == '#' || c == '$' || c == '+' || c == '-')
  {
    _editString.insert(pos, 1, c);
    return true;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditNumWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
  x += _editScrollOffset;

  int width = 0;
  unsigned int i;

  for (i = 0; i < _editString.size(); ++i)
  {
    width += _font->getCharWidth(_editString[i]);
    if (width >= x)
      break;
  }

  if (setCaretPos(i))
  {
    draw();
    // TODO - dirty rectangle
    _boss->instance()->frameBuffer().refreshOverlay();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditNumWidget::drawWidget(bool hilite)
{
  FrameBuffer& fb = _boss->instance()->frameBuffer();

  // Draw a thin frame around us.
  fb.hLine(_x, _y, _x + _w - 1, kColor);
  fb.hLine(_x, _y + _h - 1, _x +_w - 1, kShadowColor);
  fb.vLine(_x, _y, _y + _h - 1, kColor);
  fb.vLine(_x + _w - 1, _y, _y + _h - 1, kShadowColor);

  // Draw the text
  adjustOffset();
  fb.drawString(_font, _editString, _x + 2, _y + 2, getEditRect().width(),
                kTextColor, kTextAlignLeft, -_editScrollOffset, false);

  // Draw the caret
  drawCaret();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GUI::Rect EditNumWidget::getEditRect() const
{
  GUI::Rect r(2, 0, _w - 2, _h - 1);
  return r;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditNumWidget::receivedFocusWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditNumWidget::lostFocusWidget()
{
  // If we loose focus, 'commit' the user changes
  _backupString = _editString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditNumWidget::startEditMode()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditNumWidget::endEditMode()
{
  releaseFocus();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditNumWidget::abortEditMode()
{
  setEditString(_backupString);
  releaseFocus();
}
