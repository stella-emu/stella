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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Dialog.hxx"
#include "StellaKeys.hxx"
#include "FBSurface.hxx"
#include "Font.hxx"
#include "OSystem.hxx"
#include "EventHandler.hxx"
#include "EditableWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EditableWidget::EditableWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, int w, int h, const string& str)
  : Widget(boss, font, x, y, w, h),
    CommandSender(boss),
    _editString(str),
    _filter([](char c) { return isprint(c) && c != '\"'; })
{
  _bgcolor = kWidColor;
  _bgcolorhi = kWidColor;
  _bgcolorlo = kDlgColor;
  _textcolor = kTextColor;
  _textcolorhi = kTextColor;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditableWidget::setText(const string& str, bool)
{
  // Filter input string
  _editString = "";
  for(char c: str)
    if(_filter(tolower(c)))
      _editString.push_back(c);

  _caretPos = int(_editString.size());
  _selectSize = 0;

  _editScrollOffset = (_font.getStringWidth(_editString) - (getEditRect().w()));
  if (_editScrollOffset < 0)
    _editScrollOffset = 0;

  setDirty();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditableWidget::setEditable(bool editable, bool hiliteBG)
{
  _editable = editable;
  if(_editable)
  {
    setFlags(Widget::FLAG_WANTS_RAWDATA | Widget::FLAG_RETAIN_FOCUS);
    _bgcolor = kWidColor;
  }
  else
  {
    clearFlags(Widget::FLAG_WANTS_RAWDATA | Widget::FLAG_RETAIN_FOCUS);
    _bgcolor = hiliteBG ? kBGColorHi : kWidColor;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditableWidget::lostFocusWidget()
{
  _selectSize = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::tryInsertChar(char c, int pos)
{
  if(_filter(tolower(c)))
  {
    _editString.insert(pos, 1, c);
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::handleText(char text)
{
  if(!_editable)
    return true;

  if(tryInsertChar(text, _caretPos))
  {
    _caretPos++;
    _selectSize = 0;
    sendCommand(EditableWidget::kChangedCmd, 0, _id);
    setDirty();
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::handleKeyDown(StellaKey key, StellaMod mod)
{
  if(!_editable)
    return true;

  // Ignore all alt-mod keys
  if(StellaModTest::isAlt(mod))
    return true;

  // Handle Control and Control-Shift keys
  if(StellaModTest::isControl(mod) && handleControlKeys(key, mod))
    return true;

  // Handle Shift keys
  if(StellaModTest::isShift(mod) && handleShiftKeys(key))
    return true;

  // Handle keys without modifiers
  return handleNormalKeys(key);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::handleControlKeys(StellaKey key, StellaMod mod)
{
  bool shift = StellaModTest::isShift(mod);
  bool handled = true;
  bool dirty = true;

  switch(key)
  {
    case KBDK_A:
      setCaretPos(0);
      _selectSize = -int(_editString.size());
      break;

    case KBDK_C:
    case KBDK_INSERT:
      handled = copySelectedText();
      break;

    case KBDK_E:
      if(shift)
        _selectSize += _caretPos - int(_editString.size());
      else
        _selectSize = 0;
      setCaretPos(int(_editString.size()));
      break;

    case KBDK_D:
      handled = killChar(+1);
      if(handled) sendCommand(EditableWidget::kChangedCmd, key, _id);
      break;

    case KBDK_K:
      handled = killLine(+1);
      if(handled) sendCommand(EditableWidget::kChangedCmd, key, _id);
      break;

    case KBDK_U:
      handled = killLine(-1);
      if(handled) sendCommand(EditableWidget::kChangedCmd, key, _id);
      break;

    case KBDK_V:
      handled = pasteSelectedText();
      if(handled)
        sendCommand(EditableWidget::kChangedCmd, key, _id);
      break;

    case KBDK_W:
      handled = killLastWord();
      if(handled) sendCommand(EditableWidget::kChangedCmd, key, _id);
      break;

    case KBDK_X:
      handled = cutSelectedText();
      if(handled)
        sendCommand(EditableWidget::kChangedCmd, key, _id);
      break;

    case KBDK_LEFT:
      handled = moveWord(-1, shift);
      if(!shift)
        _selectSize = 0;
      break;

    case KBDK_RIGHT:
      handled = moveWord(+1, shift);
      if(!shift)
        _selectSize = 0;
      break;

    default:
      handled = false;
      dirty = false;
  }

  if(dirty)
    setDirty();

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::handleShiftKeys(StellaKey key)
{
  bool handled = true;

  switch(key)
  {
    case KBDK_DELETE:
    case KBDK_KP_PERIOD:
      handled = cutSelectedText();
      if(handled)
        sendCommand(EditableWidget::kChangedCmd, key, _id);
      break;

    case KBDK_INSERT:
      handled = pasteSelectedText();
      if(handled)
        sendCommand(EditableWidget::kChangedCmd, key, _id);
      break;

    case KBDK_LEFT:
      if(_caretPos > 0)
        handled = moveCaretPos(-1);
      break;

    case KBDK_RIGHT:
      if(_caretPos < int(_editString.size()))
        handled = moveCaretPos(+1);
      break;

    case KBDK_HOME:
      handled = moveCaretPos(-_caretPos);
      break;

    case KBDK_END:
      handled = moveCaretPos(int(_editString.size()) - _caretPos);
      break;


    default:
      handled = false;
  }

  if(handled)
    setDirty();

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::handleNormalKeys(StellaKey key)
{
  bool selectMode = false;
  bool handled = true;

  switch(key)
  {
    case KBDK_LSHIFT:
    case KBDK_RSHIFT:
    case KBDK_LCTRL:
    case KBDK_RCTRL:
      // stay in select mode
      selectMode = _selectSize;
      handled = false;
      break;

    case KBDK_RETURN:
    case KBDK_KP_ENTER:
      // confirm edit and exit editmode
      endEditMode();
      sendCommand(EditableWidget::kAcceptCmd, 0, _id);
      break;

    case KBDK_ESCAPE:
      abortEditMode();
      sendCommand(EditableWidget::kCancelCmd, 0, _id);
      break;

    case KBDK_BACKSPACE:
      handled = killSelectedText();
      if(!handled)
        handled = killChar(-1);
      if(handled) sendCommand(EditableWidget::kChangedCmd, key, _id);
      break;

    case KBDK_DELETE:
    case KBDK_KP_PERIOD:
      handled = killSelectedText();
      if(!handled)
        handled = killChar(+1);
      if(handled) sendCommand(EditableWidget::kChangedCmd, key, _id);
      break;

    case KBDK_LEFT:
      if (_selectSize)
        handled = setCaretPos(selectStartPos());
      else if(_caretPos > 0)
        handled = setCaretPos(_caretPos - 1);
      break;

    case KBDK_RIGHT:
      if(_selectSize)
        handled = setCaretPos(selectEndPos());
      else if(_caretPos < int(_editString.size()))
        handled = setCaretPos(_caretPos + 1);
      break;

    case KBDK_HOME:
      handled = setCaretPos(0);
      break;

    case KBDK_END:
      handled = setCaretPos(int(_editString.size()));
      break;

    default:
      killSelectedText();
      handled = false;
  }

  if(handled)
    setDirty();
  if(!selectMode)
    _selectSize = 0;

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int EditableWidget::getCaretOffset() const
{
  int caretOfs = 0;
  for (int i = 0; i < _caretPos; i++)
    caretOfs += _font.getCharWidth(_editString[i]);

  caretOfs -= _editScrollOffset;

  return caretOfs;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditableWidget::drawCaretSelection()
{
  // Only draw if item is visible
  if (!_editable || !isVisible() || !_boss->isVisible() || !_hasFocus)
    return;

  const Common::Rect& editRect = getEditRect();
  int x = editRect.x();
  int y = editRect.y();

  x += getCaretOffset();

  x += _x;
  y += _y;

  FBSurface& s = _boss->dialog().surface();
  s.vLine(x, y + 2, y + editRect.h() - 2, kTextColorHi);
  s.vLine(x-1, y + 2, y + editRect.h() - 2, kTextColorHi);

  if(_selectSize)
  {
    string text = selectString();
    x = editRect.x();
    y = editRect.y();
    int w = editRect.w();
    int h = editRect.h();
    int wt = int(text.length()) * _font.getMaxCharWidth() + 1;
    int dx = selectStartPos() * _font.getMaxCharWidth() - _editScrollOffset;

    if(dx < 0)
    {
      // selected text starts left of displayed rect
      text = text.substr(-(dx - 1) / _font.getMaxCharWidth());
      wt += dx;
      dx = 0;
    }
    else
      x += dx;
    // limit selection to the right of displayed rect
    w = std::min(w - dx + 1, wt);

    x += _x;
    y += _y;

    s.fillRect(x - 1, y + 1, w + 1, h - 3, kTextColorHi);
    s.drawString(_font, text, x, y + 1, w, h,
                 kTextColorInv, TextAlign::Left, 0, false);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::setCaretPos(int newPos)
{
  assert(newPos >= 0 && newPos <= int(_editString.size()));
  _caretPos = newPos;

  return adjustOffset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::moveCaretPos(int direction)
{
  if(setCaretPos(_caretPos + direction))
  {
    _selectSize -= direction;
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::adjustOffset()
{
  // check if the caret is still within the textbox; if it isn't,
  // adjust _editScrollOffset

  // For some reason (differences in ScummVM event handling??),
  // this method should always return true.

  int caretOfs = getCaretOffset();
  const int editWidth = getEditRect().w();

  if (caretOfs < 0)
  {
    // scroll left
    _editScrollOffset += caretOfs;
  }
  else if (caretOfs >= editWidth)
  {
    // scroll right
    _editScrollOffset -= (editWidth - caretOfs);
  }
  else if (_editScrollOffset > 0)
  {
    const int strWidth = _font.getStringWidth(_editString);
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
int EditableWidget::scrollOffset()
{
  return _editable ? -_editScrollOffset : 0;
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
      // remove selection for removed text
      if(_selectSize < 0)
        _selectSize = 0;
    }
  }
  else if(direction == 1)  // erase from current position to end of line
  {
    int count = int(_editString.size()) - _caretPos;
    if(count > 0)
    {
      for (int i = 0; i < count; i++)
        killChar(+1);

      handled = true;
      // remove selection for removed text
      if(_selectSize > 0)
        _selectSize = 0;
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
    // remove selection for removed word
    if(_selectSize < 0)
      _selectSize = std::min(_selectSize + count, 0);
  }

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::moveWord(int direction, bool select)
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
      if(select)
        _selectSize++;
    }
    _caretPos = currentPos;
    handled = true;
  }
  else if(direction == +1)  // move to first character of next word
  {
    while (currentPos < int(_editString.size()))
    {
      if (_editString[currentPos - 1] == ' ')
      {
        if (!space)
          break;
      }
      else
        space = false;

      currentPos++;
      if(select)
        _selectSize--;
    }
    _caretPos = currentPos;
    handled = true;
  }

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string EditableWidget::selectString() const
{
  if(_selectSize)
  {
    int caretPos = _caretPos;
    int selectSize = _selectSize;

    if(selectSize < 0)
    {
      caretPos += selectSize;
      selectSize = -selectSize;
    }
    return _editString.substr(caretPos, selectSize);
  }
  return EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int EditableWidget::selectStartPos()
{
  if(_selectSize < 0)
    return _caretPos + _selectSize;
  else
    return _caretPos;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int EditableWidget::selectEndPos()
{
  if(_selectSize > 0)
    return _caretPos + _selectSize;
  else
    return _caretPos;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::killSelectedText()
{
  if(_selectSize)
  {
    if(_selectSize < 0)
    {
      _caretPos += _selectSize;
      _selectSize = -_selectSize;
    }
    _editString.erase(_caretPos, _selectSize);
    _selectSize = 0;
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::cutSelectedText()
{
  string selected = selectString();

  // only cut and copy if anything is selected, else keep old cut text
  if(!selected.empty())
  {
    instance().eventHandler().copyText(selected);
    killSelectedText();
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::copySelectedText()
{
  string selected = selectString();

  // only copy if anything is selected, else keep old copied text
  if(!selected.empty())
  {
    instance().eventHandler().copyText(selected);
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::pasteSelectedText()
{
  bool selected = !selectString().empty();
  string pasted;

  // retrieve the pasted text
  instance().eventHandler().pasteText(pasted);
  // remove the currently selected text
  killSelectedText();
  // insert paste text instead
  _editString.insert(_caretPos, pasted);
  // position cursor at the end of pasted text
  _caretPos += int(pasted.length());

  return selected || !pasted.empty();
}
