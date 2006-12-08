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
// $Id: EditTextWidget.cxx,v 1.16 2006-12-08 16:49:33 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "Dialog.hxx"
#include "EditTextWidget.hxx"


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EditTextWidget::EditTextWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, int w, int h, const string& text)
  : EditableWidget(boss, font, x, y - 1, w, h + 2)
{
  _flags = WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS;
  _type = kEditTextWidget;

  _editable = true;
  startEditMode();  // We're always in edit mode
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::setEditString(const string& str)
{
  EditableWidget::setEditString(str);
  _backupString = str;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
  if(!_editable)
    return;

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
    setDirty(); draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::drawWidget(bool hilite)
{
//cerr << "EditTextWidget::drawWidget\n";
  FrameBuffer& fb = _boss->instance()->frameBuffer();

  // Draw a thin frame around us.
  fb.hLine(_x, _y, _x + _w - 1, kColor);
  fb.hLine(_x, _y + _h - 1, _x +_w - 1, kShadowColor);
  fb.vLine(_x, _y, _y + _h - 1, kColor);
  fb.vLine(_x + _w - 1, _y, _y + _h - 1, kShadowColor);

  // Draw the text
  adjustOffset();
  fb.drawString(_font, _editString, _x + 2, _y + 2, getEditRect().width(),
                _color, kTextAlignLeft, -_editScrollOffset, false);

  // Draw the caret 
  drawCaret();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GUI::Rect EditTextWidget::getEditRect() const
{
  GUI::Rect r(2, 1, _w - 2, _h);
  return r;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::lostFocusWidget()
{
  // If we loose focus, 'commit' the user changes
  _backupString = _editString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::startEditMode()
{
  EditableWidget::startEditMode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::endEditMode()
{
  EditableWidget::endEditMode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::abortEditMode()
{
  setEditString(_backupString);
  EditableWidget::abortEditMode();
}
