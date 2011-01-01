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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "Dialog.hxx"
#include "EditableWidget.hxx"


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EditableWidget::EditableWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, int w, int h)
  : Widget(boss, font, x, y, w, h),
    CommandSender(boss),
    _editable(true)
{
  _caretVisible = false;
  _caretTime = 0;
  _caretPos = 0;

  _caretInverse = false;

  _editScrollOffset = 0;

  _bgcolor = kWidColor;
  _bgcolorhi = kWidColor;
  _textcolor = kTextColor;
  _textcolorhi = kTextColor;
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

  if(_editable)
    startEditMode();

  // Make sure the new string is seen onscreen
  setDirty(); draw();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditableWidget::setEditable(bool editable)
{
  _editable = editable;
  if(_editable)
    setFlags(WIDGET_WANTS_RAWDATA|WIDGET_RETAIN_FOCUS);
  else
    clearFlags(WIDGET_WANTS_RAWDATA|WIDGET_RETAIN_FOCUS);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::tryInsertChar(char c, int pos)
{
  if(isprint(c) && c != '\"')
  {
    _editString.insert(pos, 1, c);
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::handleKeyDown(int ascii, int keycode, int modifiers)
{
  if(!_editable)
    return true;

  // Ignore all alt-mod keys
  if(instance().eventHandler().kbdAlt(modifiers))
    return true;

  bool handled = true;
  bool dirty = false;

  switch (ascii)
  {
    case '\n':  // enter/return
    case '\r':
      // confirm edit and exit editmode
      endEditMode();
      sendCommand(kEditAcceptCmd, 0, _id);
      dirty = true;
      break;

    case 27:    // escape
      abortEditMode();
      sendCommand(kEditCancelCmd, 0, _id);
      dirty = true;
      break;

    case 8:     // backspace
      dirty = killChar(-1);
      if(dirty)  sendCommand(kEditChangedCmd, ascii, _id);
      break;

    case 127:   // delete
      dirty = killChar(+1);
      if(dirty)  sendCommand(kEditChangedCmd, ascii, _id);
      break;

    case 256 + 20:  // left arrow
      if(instance().eventHandler().kbdControl(modifiers))
        dirty = specialKeys(ascii, keycode);
      else if(_caretPos > 0)
        dirty = setCaretPos(_caretPos - 1);
      break;

    case 256 + 19:  // right arrow
      if(instance().eventHandler().kbdControl(modifiers))
        dirty = specialKeys(ascii, keycode);
      else if(_caretPos < (int)_editString.size())
        dirty = setCaretPos(_caretPos + 1);
      break;

    case 256 + 22:  // home
      dirty = setCaretPos(0);
      break;

    case 256 + 23:  // end
      dirty = setCaretPos(_editString.size());
      break;

    default:
      if (instance().eventHandler().kbdControl(modifiers))
      {
        dirty = specialKeys(ascii, keycode);
      }
      else if (tryInsertChar((char)ascii, _caretPos))
      {
        _caretPos++;
        sendCommand(kEditChangedCmd, ascii, _id);
        dirty = true;
      }
      else
        handled = false;
  }

  if (dirty)
  {
    setDirty(); draw();
  }

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
void EditableWidget::drawCaret()
{
//cerr << "EditableWidget::drawCaret()\n";
  // Only draw if item is visible
  if (!_editable || !isVisible() || !_boss->isVisible() || !_hasFocus)
    return;

  const GUI::Rect& editRect = getEditRect();
  int x = editRect.left;
  int y = editRect.top;

  x += getCaretOffset();

  x += _x;
  y += _y;

  FBSurface& s = _boss->dialog().surface();
  s.vLine(x, y+2, y + editRect.height() - 3, kTextColorHi);
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

  // For some reason (differences in ScummVM event handling??),
  // this method should always return true.

  int caretpos = getCaretOffset();
  const int editWidth = getEditRect().width();

  if (caretpos < 0)
  {
    // scroll left
    _editScrollOffset += caretpos;
  }
  else if (caretpos >= editWidth)
  {
    // scroll right
    _editScrollOffset -= (editWidth - caretpos);
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

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::specialKeys(int ascii, int keycode)
{
  bool handled = true;

  switch (keycode)
  {
    case 'a':
      setCaretPos(0);
      break;

    case 'c':
      copySelectedText();
      if(handled) sendCommand(kEditChangedCmd, ascii, _id);
      break;

    case 'e':
      setCaretPos(_editString.size());
      break;

    case 'd':
      handled = killChar(+1);
      if(handled) sendCommand(kEditChangedCmd, ascii, _id);
      break;

    case 'k':
      handled = killLine(+1);
      if(handled) sendCommand(kEditChangedCmd, ascii, _id);
      break;

    case 'u':
      handled = killLine(-1);
      if(handled) sendCommand(kEditChangedCmd, ascii, _id);
      break;

    case 'v':
      pasteSelectedText();
      if(handled) sendCommand(kEditChangedCmd, ascii, _id);
      break;

    case 'w':
      handled = killLastWord();
      if(handled) sendCommand(kEditChangedCmd, ascii, _id);
      break;

    case 256 + 20:  // left arrow
      handled = moveWord(-1);
      break;

    case 256 + 19:  // right arrow
      handled = moveWord(+1);
      break;

    default:
      handled = false;
  }

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::killChar(int direction)
{
  bool handled = false;

  if(direction == -1)      // Delete previous character (backspace)
  {
    if(_caretPos > 0)
    {
      _caretPos--;
      _editString.erase(_caretPos, 1);
      handled = true;
    }
  }
  else if(direction == 1)  // Delete next character (delete)
  {
    _editString.erase(_caretPos, 1);
    handled = true;
  }

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::killLine(int direction)
{
  bool handled = false;

  if(direction == -1)  // erase from current position to beginning of line
  {
    int count = _caretPos;
    if(count > 0)
    {
      for (int i = 0; i < count; i++)
        killChar(-1);

      handled = true;
    }
  }
  else if(direction == 1)  // erase from current position to end of line
  {
    int count = _editString.size() - _caretPos;
    if(count > 0)
    {
      for (int i = 0; i < count; i++)
        killChar(+1);

      handled = true;
    }
  }

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::killLastWord()
{
  bool handled = false;
  int count = 0, currentPos = _caretPos;
  bool space = true;
  while (currentPos > 0)
  {
    if (_editString[currentPos - 1] == ' ')
    {
      if (!space)
        break;
    }
    else
      space = false;

    currentPos--;
    count++;
  }

  if(count > 0)
  {
    for (int i = 0; i < count; i++)
      killChar(-1);

    handled = true;
  }

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::moveWord(int direction)
{
  bool handled = false;
  bool space = true;
  int currentPos = _caretPos;

  if(direction == -1)  // move to first character of previous word
  {
    while (currentPos > 0)
    {
      if (_editString[currentPos - 1] == ' ')
      {
        if (!space)
          break;
      }
      else
        space = false;

      currentPos--;
    }
    _caretPos = currentPos;
    handled = true;
  }
  else if(direction == +1)  // move to first character of next word
  {
    while (currentPos < (int)_editString.size())
    {
      if (_editString[currentPos - 1] == ' ')
      {
        if (!space)
          break;
      }
      else
        space = false;

      currentPos++;
    }
    _caretPos = currentPos;
    handled = true;
  }

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditableWidget::copySelectedText()
{
  _clippedText = _editString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditableWidget::pasteSelectedText()
{
  _editString = _clippedText;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EditableWidget::_clippedText = "";
