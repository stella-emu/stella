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

// Uncomment the following to give full-line cut/copy/paste
// Note that this will be removed eventually, when we implement proper cut/copy/paste
#define PSEUDO_CUT_COPY_PASTE

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

  bool handled = true;
  bool dirty = false;

  switch(key)
  {
    case KBDK_RETURN:
    case KBDK_KP_ENTER:
      // confirm edit and exit editmode
      endEditMode();
      sendCommand(EditableWidget::kAcceptCmd, 0, _id);
      dirty = true;
      break;

    case KBDK_ESCAPE:
      abortEditMode();
      sendCommand(EditableWidget::kCancelCmd, 0, _id);
      dirty = true;
      break;

    case KBDK_BACKSPACE:
      dirty = killChar(-1);
      if(dirty)  sendCommand(EditableWidget::kChangedCmd, key, _id);
      break;

    case KBDK_DELETE:
    case KBDK_KP_PERIOD:
      if(StellaModTest::isShift(mod))
      {
        cutSelectedText();
        dirty = true;
      }
      else
        dirty = killChar(+1);
      if(dirty)  sendCommand(EditableWidget::kChangedCmd, key, _id);
      break;

    case KBDK_LEFT:
      if(StellaModTest::isControl(mod))
        dirty = specialKeys(key);
      else if(_caretPos > 0)
        dirty = setCaretPos(_caretPos - 1);
      break;

    case KBDK_RIGHT:
      if(StellaModTest::isControl(mod))
        dirty = specialKeys(key);
      else if(_caretPos < int(_editString.size()))
        dirty = setCaretPos(_caretPos + 1);
      break;

    case KBDK_HOME:
      dirty = setCaretPos(0);
      break;

    case KBDK_END:
      dirty = setCaretPos(int(_editString.size()));
      break;

    case KBDK_INSERT:
      if(StellaModTest::isControl(mod))
      {
        copySelectedText();
        dirty = true;
      }
      else if(StellaModTest::isShift(mod))
      {
        pasteSelectedText();
        dirty = true;
      }
      else
        handled = false;
      break;

    default:
      if (StellaModTest::isControl(mod))
      {
        dirty = specialKeys(key);
      }
      else
        handled = false;
  }

  if (dirty)
    setDirty();

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int EditableWidget::getCaretOffset() const
{
  int caretpos = 0;
  for (int i = 0; i < _caretPos; i++)
    caretpos += _font.getCharWidth(_editString[i]);

  caretpos -= _editScrollOffset;

  return caretpos;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditableWidget::drawCaret()
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
  s.vLine(x, y+2, y + editRect.h() - 2, kTextColorHi);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EditableWidget::setCaretPos(int newPos)
{
  assert(newPos >= 0 && newPos <= int(_editString.size()));
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
  const int editWidth = getEditRect().w();

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
bool EditableWidget::specialKeys(StellaKey key)
{
  bool handled = true;

  switch(key)
  {
    case KBDK_A:
      setCaretPos(0);
      break;

    case KBDK_C:
      copySelectedText();
      break;

    case KBDK_E:
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
      pasteSelectedText();
      sendCommand(EditableWidget::kChangedCmd, key, _id);
      break;

    case KBDK_W:
      handled = killLastWord();
      if(handled) sendCommand(EditableWidget::kChangedCmd, key, _id);
      break;

    case KBDK_X:
      cutSelectedText();
      sendCommand(EditableWidget::kChangedCmd, key, _id);
      break;

    case KBDK_LEFT:
      handled = moveWord(-1);
      break;

    case KBDK_RIGHT:
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
    int count = int(_editString.size()) - _caretPos;
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
    }
    _caretPos = currentPos;
    handled = true;
  }

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditableWidget::cutSelectedText()
{
#if defined(PSEUDO_CUT_COPY_PASTE)
  instance().eventHandler().cutText(_editString);
  _caretPos = 0;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditableWidget::copySelectedText()
{
#if defined(PSEUDO_CUT_COPY_PASTE)
  instance().eventHandler().copyText(_editString);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditableWidget::pasteSelectedText()
{
#if defined(PSEUDO_CUT_COPY_PASTE)
  instance().eventHandler().pasteText(_editString);
  _caretPos = int(_editString.length());
#endif
}
