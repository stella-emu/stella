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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "ContextMenu.hxx"
#include "UndoHandler.hxx"
#include "ScrollBarWidget.hxx"
#include "FBSurface.hxx"
#include "Font.hxx"
#include "StellaKeys.hxx"
#include "Version.hxx"
#include "OSystem.hxx"
#include "Debugger.hxx"
#include "DebuggerDialog.hxx"
#include "DebuggerParser.hxx"
#include "EventHandler.hxx"

#include "PromptWidget.hxx"
#include "CartDebug.hxx"

static constexpr string_view PROMPT = "> ";

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PromptWidget::PromptWidget(GuiObject* boss, const GUI::Font& font,
                           int x, int y, int w, int h)
  : Widget(boss, font, x, y, w - ScrollBarWidget::scrollBarWidth(font), h),
    CommandSender(boss),
    _kConsoleCharWidth{font.getMaxCharWidth()},
    _kConsoleCharHeight{font.getFontHeight()},
    _kConsoleLineHeight{_kConsoleCharHeight + 2}
{
  _flags = Widget::FLAG_ENABLED | Widget::FLAG_CLEARBG | Widget::FLAG_RETAIN_FOCUS |
           Widget::FLAG_WANTS_TAB | Widget::FLAG_WANTS_RAWDATA |
           Widget::FLAG_TRACK_MOUSE;
  _textcolor = kTextColor;
  _bgcolor = kWidColor;
  _bgcolorlo = kDlgColor;

  recalcMetrics();

  // Add scrollbar
  // We want to initialize here, not in the member list
  // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
  _scrollBar = new ScrollBarWidget(boss, font, _x + _w, _y,
                                   ScrollBarWidget::scrollBarWidth(_font), _h);
  _scrollBar->setTarget(this);

  myUndoHandler = std::make_unique<UndoHandler>();

  clearScreen();

  addFocusWidget(this);
  setHelpAnchor("PromptTab", true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::recalcMetrics()
{
  const int lineWidth = _lineWidth;

  // The scrollbar is already excluded from _w (see the constructor), so the
  // text spans the full width less a one pixel margin on each side
  _lineWidth = std::max((_w - 2) / _kConsoleCharWidth, 1);
  _linesPerPage = std::max((_h - 2) / _kConsoleLineHeight, 1);
  _linesInBuffer = kBufferSize / _lineWidth;

  // The buffer holds its text wrapped at a fixed line width, so it cannot be
  // reinterpreted once that width changes
  _bufferStale |= _lineWidth != lineWidth;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::setPos(const Common::Point& pos)
{
  Widget::setPos(pos);
  // The scrollbar is a sibling widget, not a child, so it must be moved to
  // track the console (it sits flush against the console's right edge)
  _scrollBar->setPos(_x + _w, _y);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::setWidth(int w)
{
  // getWidth() reports the full footprint (console + scrollbar), so setWidth()
  // must subtract the scrollbar again to stay its inverse (mirrors the
  // constructor); the scrollbar stays flush against the right edge
  Widget::setWidth(w - ScrollBarWidget::scrollBarWidth(_font));
  _scrollBar->setPosX(_x + _w);

  recalcMetrics();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::setHeight(int h)
{
  Widget::setHeight(h);
  _scrollBar->setHeight(h);

  recalcMetrics();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::setArea(int x, int y, int w, int h)
{
  // Both of these recompute the metrics; only once the area is fully settled
  // can the buffer be judged against it
  Widget::setArea(x, y, w, h);

  if(_bufferStale && !_firstTime)
  {
    // A changed line width invalidates everything already printed.  The command
    // history survives, and so does whatever had been typed at the prompt
    const string input = getLine();

    clearScreen();
    printPrompt();

    if(!input.empty())
    {
      setLine(input);
      myUndoHandler->doo(input);
      scrollToCurrent();
    }
  }
  else
  {
    // A height-only change leaves the buffer valid, but the view may now reach
    // above the oldest line still held
    _bufferStale = false;
    _scrollLine = std::max(_scrollLine, _firstLineInBuffer + _linesPerPage - 1);
    updateScrollBuffer();
  }
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::refreshFontMetrics()
{
  Widget::refreshFontMetrics();

  _kConsoleCharWidth = _font.getMaxCharWidth();
  _kConsoleCharHeight = _font.getFontHeight();
  _kConsoleLineHeight = _kConsoleCharHeight + 2;

  // A new character width means a new line width; the setArea() that follows
  // from the ensuing relayout() acts on the stale buffer this leaves behind
  recalcMetrics();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::drawWidget(bool hilite)
{
//cerr << "PromptWidget::drawWidget\n";
  ColorId fgcolor{}, bgcolor{};
  FBSurface& s = _boss->dialog().surface();

  // Draw text
  const int start = _scrollLine - _linesPerPage + 1;
  const int selStart = selectStartPos();
  const int selEnd = selectEndPos();
  int y = _y + 2;

  for (int line = 0; line < _linesPerPage; ++line)
  {
    int x = _x + 1;
    for (int column = 0; column < _lineWidth; ++column) {
      const int bufPos = (start + line) * _lineWidth + column;
      const int c = buffer(bufPos);
      const bool isSelected = (_selectSize != 0) && (bufPos >= selStart) && (bufPos < selEnd);

      if(isSelected)
      {
        s.fillRect(x, y, _kConsoleCharWidth, _kConsoleCharHeight, kTextColorHi);
        s.drawChar(_font, c & 0x7f, x, y, kTextColorInv);
      }
      else if(c & (1 << 17))  // inverse video flag
      {
        fgcolor = _bgcolor;
        bgcolor = static_cast<ColorId>((c & 0x1ffff) >> 8);
        s.fillRect(x, y, _kConsoleCharWidth, _kConsoleCharHeight, bgcolor);
        s.drawChar(_font, c & 0x7f, x, y, fgcolor);
      }
      else
      {
        fgcolor = static_cast<ColorId>(c >> 8);
        s.drawChar(_font, c & 0x7f, x, y, fgcolor);
      }
      x += _kConsoleCharWidth;
    }
    y += _kConsoleLineHeight;
  }

  // Draw the caret
  drawCaret();

  // Draw the scrollbar
  _scrollBar->draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  if(b == MouseButton::RIGHT && isEnabled() && !mouseMenu().isVisible())
  {
    VariantList items;
  #ifndef BSPF_MACOS
    VarList::push_back(items, " Cut     Ctrl+X ", "cut");
    VarList::push_back(items, " Copy    Ctrl+C ", "copy");
    VarList::push_back(items, " Paste   Ctrl+V ", "paste");
  #else
    VarList::push_back(items, " Cut      Cmd+X ", "cut");
    VarList::push_back(items, " Copy     Cmd+C ", "copy");
    VarList::push_back(items, " Paste    Cmd+V ", "paste");
  #endif
    ContextMenu& menu = mouseMenu();
    menu.addItems(items);

    // Enable/disable items based on the current state
    const bool hasSelection = _selectSize != 0;
    menu.setEnabled("cut",   hasSelection);
    menu.setEnabled("copy",  hasSelection);
    menu.setEnabled("paste", instance().eventHandler().hasClipboardText());

    menu.show(x + getAbsX(), y + getAbsY(), dialog().surface().dstRect());
    return;
  }

  if(b == MouseButton::LEFT && _promptStartPos >= 0)
  {
    const int start = _scrollLine - _linesPerPage + 1;
    const int screenLine = (y - 2) / _kConsoleLineHeight;
    const int col = (x - 1) / _kConsoleCharWidth;
    const int newPos = std::clamp((start + screenLine) * _lineWidth + col,
                                  _firstLineInBuffer * _lineWidth, _promptEndPos);
    _currentPos = newPos;
    _selectSize = 0;

    if(clickCount == 2)
    {
      markWord();
      setDirty();
      return;
    }

    _isDragging = true;
    setDirty();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::handleMouseUp(int x, int y, MouseButton b, int clickCount)
{
  _isDragging = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::handleMouseMoved(int x, int y)
{
  if(!_isDragging || _promptStartPos < 0)
    return;

  const int start = _scrollLine - _linesPerPage + 1;
  const int screenLine = (y - 2) / _kConsoleLineHeight;
  const int col = (x - 1) / _kConsoleCharWidth;
  const int newPos = std::clamp((start + screenLine) * _lineWidth + col,
                                _firstLineInBuffer * _lineWidth, _promptEndPos);

  if(newPos != _currentPos)
  {
    _selectSize -= (newPos - _currentPos);
    _currentPos = newPos;
    setDirty();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::handleMouseWheel(int x, int y, int direction)
{
  _scrollBar->handleMouseWheel(x, y, direction);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::printPrompt()
{
  const string watches = instance().debugger().showWatches();
  if(!watches.empty())
    print(watches);

  print(PROMPT);
  _promptStartPos = _promptEndPos = _currentPos;
  (*myUndoHandler).reset();  // Make sure to call ::reset, not smartptr reset
  myUndoHandler->doo("");

  resetFunctions();
  scrollToCurrent();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PromptWidget::handleText(char text)
{
  if(text >= 0)
  {
    if(_currentPos < _promptStartPos)
    {
      _currentPos = _promptEndPos;
      _selectSize = 0;
    }
    if(_selectSize != 0)
      killSelectedText();
    for(int i = _promptEndPos - 1; i >= _currentPos; i--)
      buffer(i + 1) = buffer(i);
    _promptEndPos++;
    putcharIntern(text);
    myUndoHandler->doChar();
    scrollToCurrent();
    resetFunctions();
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PromptWidget::handleKeyDown(StellaKey key, StellaMod mod)
{
  bool handled = true,
    dirty = true,
    changeInput = false,
    resetAutoComplete = true,
    resetHistoryScroll = true;

  // Uses normal edit events + special prompt events
  Event::Type event = instance().eventHandler().eventForKey(EventMode::kEditMode, key, mod);
  if(event == Event::NoType)
    event = instance().eventHandler().eventForKey(EventMode::kPromptMode, key, mod);

  // Finalize any aggregated char group before processing this key
  myUndoHandler->endChars(getLine());

  // If the cursor is in scrollback (from a mouse selection), snap it back to
  // the prompt on any actual edit/cursor command, with two exceptions:
  //  - Copy still operates on the scrollback selection
  //  - NoType, e.g. a bare modifier like Ctrl pressed before C in Ctrl+C, must
  //    not clear the selection or the following Copy would have nothing to copy
  if(_currentPos < _promptStartPos &&
     event != Event::Copy && event != Event::NoType)
  {
    _currentPos = _promptEndPos;
    _selectSize = 0;
  }

  switch(event)
  {
    case Event::EndEdit:
    {
      if(_scrollLine < _currentPos / _lineWidth)
      {
        // Scroll page by page when not at cursor position:
        _scrollLine = std::min(_scrollLine + _linesPerPage,
                               _promptEndPos / _lineWidth);
        updateScrollBuffer();
        break;
      }
      if(execute())
        return true;
      printPrompt();
      break;
    }

    // special events (auto complete & history scrolling)
    case Event::UINavNext:
      dirty = changeInput = autoComplete(+1);
      resetAutoComplete = false;
      break;

    case Event::UINavPrev:
      dirty = changeInput = autoComplete(-1);
      resetAutoComplete = false;
      break;

    case Event::UILeft: // mapped to KBDK_DOWN by default
      dirty = changeInput = historyScroll(-1);
      resetHistoryScroll = false;
      break;

    case Event::UIRight: // mapped to KBDK_UP by default
      dirty = changeInput = historyScroll(+1);
      resetHistoryScroll = false;
      break;

    // input modifying events
    case Event::Backspace:
      if(_selectSize != 0)
      {
        killSelectedText();
        changeInput = true;
      }
      else if(_currentPos > _promptStartPos)
      {
        killChar(-1);
        changeInput = true;
      }
      if(changeInput) myUndoHandler->doo(getLine());
      scrollToCurrent();
      break;

    case Event::Delete:
      if(_selectSize != 0)
        killSelectedText();
      else
        killChar(+1);
      changeInput = true;
      myUndoHandler->doo(getLine());
      break;

    case Event::DeleteEnd:
      killLine(+1);
      changeInput = true;
      myUndoHandler->doo(getLine());
      break;

    case Event::DeleteHome:
      killLine(-1);
      changeInput = true;
      myUndoHandler->doo(getLine());
      break;

    case Event::DeleteLeftWord:
      killWord(-1);
      changeInput = true;
      myUndoHandler->doo(getLine());
      break;

    case Event::DeleteRightWord:
      killWord(+1);
      changeInput = true;
      myUndoHandler->doo(getLine());
      break;

    case Event::Cut:
      textCut();
      changeInput = true;
      break;

    case Event::Copy:
      textCopy();
      break;

    case Event::Paste:
      textPaste();
      changeInput = true;
      break;

    // cursor events
    case Event::MoveHome:
      _currentPos = _promptStartPos;
      _selectSize = 0;
      break;

    case Event::MoveEnd:
      _currentPos = _promptEndPos;
      _selectSize = 0;
      break;

    case Event::MoveRightChar:
      if(_selectSize)
      {
        _currentPos = selectEndPos();
        _selectSize = 0;
      }
      else if(_currentPos < _promptEndPos)
        _currentPos++;
      else
        handled = false;
      break;

    case Event::MoveLeftChar:
      if(_selectSize)
      {
        _currentPos = selectStartPos();
        _selectSize = 0;
      }
      else if(_currentPos > _promptStartPos)
        _currentPos--;
      else
        handled = false;
      break;

    // word movement events
    case Event::MoveLeftWord:
      moveWord(-1, false);
      _selectSize = 0;
      break;

    case Event::MoveRightWord:
      moveWord(+1, false);
      _selectSize = 0;
      break;

    // selection events
    case Event::SelectLeftChar:
      if(_currentPos > _promptStartPos)
      {
        _currentPos--;
        _selectSize++;
      }
      break;

    case Event::SelectRightChar:
      if(_currentPos < _promptEndPos)
      {
        _currentPos++;
        _selectSize--;
      }
      break;

    case Event::SelectHome:
      _selectSize += _currentPos - _promptStartPos;
      _currentPos = _promptStartPos;
      break;

    case Event::SelectEnd:
      _selectSize -= _promptEndPos - _currentPos;
      _currentPos = _promptEndPos;
      break;

    case Event::SelectLeftWord:
      moveWord(-1, true);
      break;

    case Event::SelectRightWord:
      moveWord(+1, true);
      break;

    case Event::SelectAll:
      _currentPos = _promptEndPos;
      _selectSize = -(_promptEndPos - _promptStartPos);
      break;

    case Event::Undo:
    case Event::Redo:
    {
      const string oldLine = getLine();
      string newLine;
      const bool ok = (event == Event::Redo)
        ? myUndoHandler->redo(newLine)
        : myUndoHandler->undo(newLine);
      if(ok)
      {
        setLine(newLine);
        const auto diffPos = static_cast<int>(UndoHandler::lastDiff(newLine, oldLine));
        _currentPos = std::clamp(_promptStartPos + diffPos, _promptStartPos, _promptEndPos);
        scrollToCurrent();
        changeInput = true;
      }
      else
        dirty = false;
      break;
    }

    // scrolling events
    case Event::UIUp:
      if(_scrollLine <= _firstLineInBuffer + _linesPerPage - 1)
        break;

      _scrollLine -= 1;
      updateScrollBuffer();
      break;

    case Event::UIDown:
      // Don't scroll down when at bottom of buffer
      if(_scrollLine >= _promptEndPos / _lineWidth)
        break;

      _scrollLine += 1;
      updateScrollBuffer();
      break;

    case Event::UIPgUp:
      // Don't scroll up when at top of buffer
      if(_scrollLine < _linesPerPage)
        break;

      _scrollLine = std::max(_scrollLine - (_linesPerPage - 1),
                             _firstLineInBuffer + _linesPerPage - 1);
      updateScrollBuffer();
      break;

    case Event::UIPgDown:
      // Don't scroll down when at bottom of buffer
      if(_scrollLine >= _promptEndPos / _lineWidth)
        break;

      _scrollLine = std::min(_scrollLine + (_linesPerPage - 1),
                             _promptEndPos / _lineWidth);
      updateScrollBuffer();
      break;

    case Event::UIHome:
      _scrollLine = _firstLineInBuffer + _linesPerPage - 1;
      updateScrollBuffer();
      break;

    case Event::UIEnd:
      _scrollLine = std::max(_promptEndPos / _lineWidth, _linesPerPage - 1);
      updateScrollBuffer();
      break;

    default:
      handled = false;
      dirty = false;
      break;
  }

  // Take care of changes made above
  if(dirty)
    setDirty();

  // Reset special event handling if input has changed
  // We assume that non-handled events will modify the input too
  if(!handled || (resetAutoComplete && changeInput))
    _tabCount = -1;
  if(!handled || (resetHistoryScroll && changeInput))
    _historyLine = 0;

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ContextMenu& PromptWidget::mouseMenu()
{
  if(myMouseMenu == nullptr)
    myMouseMenu = std::make_unique<ContextMenu>(this, _font);
  return *myMouseMenu;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::handleCommand(CommandSender* sender, int cmd,
                                 int data, int id)
{
  if(cmd == ContextMenu::kItemSelectedCmd)
  {
    const string_view sel = mouseMenu().getSelectedTag().toString();
    if(sel == "cut")
      textCut();
    else if(sel == "copy")
      textCopy();
    else if(sel == "paste")
      textPaste();
    setDirty();
  }
  else if(cmd == GuiObject::kSetPositionCmd)
  {
    const int newPos = data + _linesPerPage - 1 + _firstLineInBuffer;
    if (newPos != _scrollLine)
    {
      _scrollLine = newPos;
      setDirty();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::loadConfig()
{
  // Show the prompt the first time we draw this widget
  if(_firstTime)
  {
    _firstTime = false;

    // Display greetings & prompt
    const string version = string{STELLA_FULL_TITLE} + "\n";
    print(version);
    print(PROMPT);

    print(instance().debugger().cartDebug().loadConfigFile() + "\n");
    print(instance().debugger().cartDebug().loadListFile() + "\n");
    print(instance().debugger().cartDebug().loadSymbolFile() + "\n");

    // Take care of one-time debugger stuff
    // fill the history from the saved breaks, traps and watches commands
    StringList history;
    print(instance().debugger().autoExec(&history));
    for(const auto& h: history)
      addToHistory(h);

    history.clear();

    bool extra = false;
    if(instance().settings().getBool("dbg.autosave"))
    {
      print(DebuggerParser::inverse(" autoSave enabled "));
      print("\177 "); // must switch inverse here!
      extra = true;
    }
    if(instance().settings().getBool("dbg.logbreaks"))
    {
      print(DebuggerParser::inverse(" logBreaks enabled "));
      extra = true;
    }
    if(instance().settings().getBool("dbg.logtrace"))
    {
      print(DebuggerParser::inverse(" logTrace enabled "));
      extra = true;
    }
    if(extra)
      print("\n");

    print(PROMPT);

    _promptStartPos = _promptEndPos = _currentPos;
    (*myUndoHandler).reset();  // Make sure to call ::reset, not smartptr reset
    myUndoHandler->doo("");
    _exitedEarly = false;
  }
  else if(_exitedEarly)
  {
    printPrompt();
    _exitedEarly = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int PromptWidget::getWidth() const
{
  return _w + ScrollBarWidget::scrollBarWidth(_font);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::killChar(int direction)
{
  if(direction == -1)    // Delete previous character (backspace)
  {
    if(_currentPos <= _promptStartPos)
      return;

    _currentPos--;
    for (int i = _currentPos; i < _promptEndPos; i++)
      buffer(i) = buffer(i + 1);

    buffer(_promptEndPos) = ' ';
    _promptEndPos--;
  }
  else if(direction == 1)    // Delete next character (delete)
  {
    if(_currentPos >= _promptEndPos)
      return;

    // There are further characters to the right of cursor
    if(_currentPos + 1 <= _promptEndPos)
    {
      for (int i = _currentPos; i < _promptEndPos; i++)
        buffer(i) = buffer(i + 1);

      buffer(_promptEndPos) = ' ';
      _promptEndPos--;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::killLine(int direction)
{
  if(direction == -1)  // erase from current position to beginning of line
  {
    const int count = _currentPos - _promptStartPos;
    if(count > 0)
    {
      const int remaining = _promptEndPos - _currentPos;
      for(int i = 0; i < remaining; i++)
        buffer(_promptStartPos + i) = buffer(_currentPos + i);
      for(int i = _promptStartPos + remaining; i < _promptEndPos; i++)
        buffer(i) = ' ';
      _promptEndPos -= count;
      _currentPos = _promptStartPos;
    }
  }
  else if(direction == 1)  // erase from current position to end of line
  {
    for (int i = _currentPos; i < _promptEndPos; i++)
      buffer(i) = ' ';

    _promptEndPos = _currentPos;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::killWord(int direction)
{
  bool space = true;

  if(direction == -1)
  {
    int cnt = 0;
    while(_currentPos > _promptStartPos)
    {
      if((buffer(_currentPos - 1) & 0xff) == ' ')
      {
        if(!space) break;
      }
      else
        space = false;
      _currentPos--;
      cnt++;
    }
    if(cnt > 0)
    {
      for(int i = _currentPos; i < _promptEndPos; i++)
        buffer(i) = buffer(i + cnt);
      buffer(_promptEndPos) = ' ';
      _promptEndPos -= cnt;
    }
  }
  else if(direction == +1)
  {
    int cnt = 0;
    int pos = _currentPos;
    while(pos < _promptEndPos)
    {
      if(pos > _promptStartPos && (buffer(pos - 1) & 0xff) == ' ')
      {
        if(!space) break;
      }
      else
        space = false;
      pos++;
      cnt++;
    }
    if(cnt > 0)
    {
      for(int i = _currentPos; i < _promptEndPos; i++)
        buffer(i) = buffer(i + cnt);
      buffer(_promptEndPos) = ' ';
      _promptEndPos -= cnt;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::moveWord(int direction, bool select)
{
  bool space = true;
  int pos = _currentPos;

  if(direction == -1)
  {
    while(pos > _promptStartPos)
    {
      if((buffer(pos - 1) & 0xff) == ' ')
      {
        if(!space) break;
      }
      else
        space = false;
      pos--;
      if(select) _selectSize++;
    }
  }
  else if(direction == +1)
  {
    while(pos < _promptEndPos)
    {
      if(pos > _promptStartPos && (buffer(pos - 1) & 0xff) == ' ')
      {
        if(!space) break;
      }
      else
        space = false;
      pos++;
      if(select) _selectSize--;
    }
  }
  _currentPos = pos;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::markWord()
{
  const int bufStart = _firstLineInBuffer * _lineWidth;
  _selectSize = 0;

  // Extend selection rightward while non-space
  while(_currentPos + _selectSize < _promptEndPos &&
        (buffer(_currentPos + _selectSize) & 0xff) != ' ')
    _selectSize++;

  // Walk cursor leftward while non-space, growing selection to cover the whole word
  while(_currentPos > bufStart &&
        (buffer(_currentPos - 1) & 0xff) != ' ')
  {
    _currentPos--;
    _selectSize++;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::setLine(string_view text)
{
  for(int i = _promptStartPos; i < _promptEndPos; i++)
    buffer(i) = ' ';
  _currentPos = _promptStartPos;
  _textcolor = kTextColor;
  _inverse = false;
  for(const char c: text)
    putcharIntern(c);
  _promptEndPos = _currentPos;
  _selectSize = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::killSelectedText()
{
  if(_selectSize == 0) return;
  // Clamp to the editable prompt area — scrollback content is read-only
  const int start = std::max(selectStartPos(), _promptStartPos);
  const int end = std::min(selectEndPos(), _promptEndPos);
  _selectSize = 0;
  if(start >= end)
    return;
  const int count = end - start;
  const int remaining = _promptEndPos - end;
  for(int i = 0; i < remaining; i++)
    buffer(start + i) = buffer(end + i);
  for(int i = _promptEndPos - count; i < _promptEndPos; i++)
    buffer(i) = ' ';
  _promptEndPos -= count;
  _currentPos = start;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string PromptWidget::getLine() const
{
  assert(_promptEndPos >= _promptStartPos);
  string text;
  text.reserve(_promptEndPos - _promptStartPos);
  for(int i = _promptStartPos; i < _promptEndPos; i++)
    text += static_cast<char>(buffer(i) & 0x7f);
  return text;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string PromptWidget::selectedText() const
{
  const int start = selectStartPos();
  const int end = selectEndPos();
  string text;
  text.reserve(end - start);

  if(start >= _promptStartPos)
  {
    // Entirely within the prompt area — preserve as-is (may include intentional spaces)
    for(int i = start; i < end; i++)
      text += static_cast<char>(buffer(i) & 0x7f);
  }
  else
  {
    // Includes scrollback — strip trailing blanks per line and join with newlines.
    // Unwritten cells hold NUL (0), not space, so treat both as blank; an embedded
    // NUL would otherwise truncate the clipboard text at the end of the first line.
    const auto isBlank = [&](int idx) {
      const int ch = buffer(idx) & 0x7f;
      return ch == ' ' || ch == '\0';
    };
    int lineEnd = ((start / _lineWidth) + 1) * _lineWidth;
    for(int i = start; i < end; )
    {
      const int segEnd = std::min(lineEnd, end);
      int lastNonBlank = segEnd - 1;
      while(lastNonBlank >= i && isBlank(lastNonBlank))
        lastNonBlank--;
      for(int j = i; j <= lastNonBlank; j++)
        text += static_cast<char>(buffer(j) & 0x7f);
      i = segEnd;
      lineEnd += _lineWidth;
      if(i < end)
        text += '\n';
    }
  }
  return text;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::textCut()
{
  // Selection-only; nothing to cut without a selection
  if(_selectSize == 0)
    return;

  textCopy();
  killSelectedText();
  if(_currentPos < _promptStartPos)
    _currentPos = _promptEndPos;
  myUndoHandler->doo(getLine());
  resetFunctions();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::textCopy()
{
  // Selection-only; nothing to copy without a selection
  if(_selectSize == 0)
    return;

  const string text = selectedText();
  if(!text.empty())
    instance().eventHandler().copyText(text);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::textPaste()
{
  if(_selectSize != 0)
    killSelectedText();

  string text;
  instance().eventHandler().pasteText(text);
  for(const char c: text)
  {
    if(c >= 0 && isprint(c))
    {
      for(int i = _promptEndPos - 1; i >= _currentPos; i--)
        buffer(i + 1) = buffer(i);
      _promptEndPos++;
      putcharIntern(c);
    }
  }
  scrollToCurrent();
  myUndoHandler->doo(getLine());
  resetFunctions();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int PromptWidget::historyDir(int& index, int direction)
{
  index += direction;
  if(index < 0)
    index += static_cast<int>(_history.size());
  else
    index %= static_cast<int>(_history.size());

  return index;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::historyAdd(string_view entry)
{
  if(std::cmp_greater_equal(_historyIndex, _history.size()))
    _history.emplace_back(entry);
  else
    _history[_historyIndex] = entry;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::addToHistory(string_view str)
{
  // Do not add duplicates, remove old duplicate
  if(!_history.empty())
  {
    int i = _historyIndex;
    const int historyEnd = _historyIndex % _history.size();

    do
    {
      historyDir(i, -1);

      if(!BSPF::compareIgnoreCase(_history[i], str))
      {
        int j = i;

        do
        {
          const int prevJ = j;
          historyDir(j, +1);
          _history[prevJ] = _history[j];
        }
        while(j != historyEnd);

        historyDir(_historyIndex, -1);
        break;
      }
    }
    while(i != historyEnd);
  }
  historyAdd(str);
  _historyLine = 0; // reset history scroll
  _historyIndex = (_historyIndex + 1) % kHistorySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PromptWidget::historyScroll(int direction)
{
  if(_history.empty())
    return false;

  // add current input temporarily to history
  if(_historyLine == 0)
    historyAdd(getLine());

  // Advance to the next/prev line in the history
  historyDir(_historyLine, direction);

  // Search the history using the original input
  do
  {
    const int idx = _historyLine
      ? (_historyIndex - _historyLine + _history.size()) % static_cast<int>(_history.size())
      : _historyIndex;

    if(BSPF::startsWithIgnoreCase(_history[idx], _history[_historyIndex]))
      break;

    // Advance to the next/prev line in the history
    historyDir(_historyLine, direction);
  }
  while(_historyLine); // If _historyLine == 0, nothing was found

  // Remove the current user text
  _currentPos = _promptStartPos;
  killLine(1);  // to end of line

  // ... and ensure the prompt is visible
  scrollToCurrent();

  // Print the text from the history
  const int idx = _historyLine
    ? (_historyIndex - _historyLine + _history.size()) % static_cast<int>(_history.size())
    : _historyIndex;

  _selectSize = 0;
  for(const char c: _history[idx])
    putcharIntern(c);
  _promptEndPos = _currentPos;
  myUndoHandler->doo(getLine());

  // Ensure once more the caret is visible (in case of very long history entries)
  scrollToCurrent();

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PromptWidget::execute()
{
  nextLine();

  assert(_promptEndPos >= _promptStartPos);
  const int len = _promptEndPos - _promptStartPos;

  if(len > 0)
  {
    // Copy the user input to command
    const string command = getLine();

    // Add the input to the history
    addToHistory(command);

    // Pass the command to the debugger, and print the result
    const string result = instance().debugger().run(command);

    // Certain commands remove the debugger dialog from underneath us,
    // so we shouldn't print any messages
    if(result == DebuggerParser::kExitDebugger)
    {
      _exitedEarly = true;
      return true;
    }
    else if(result == DebuggerParser::kNoPrompt)
      return true;
    else if(!result.empty())
      print(result + "\n");
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PromptWidget::autoComplete(int direction)
{
  // Tab completion: we complete either commands or labels, but not
  // both at once.

  if(_currentPos <= _promptStartPos)
    return false; // no input

  scrollToCurrent();

  int len = _promptEndPos - _promptStartPos;

  if(_tabCount != -1)
    len = static_cast<int>(strlen(_inputStr));
  len = std::min(len, kLineBufferSize - 1);

  int lastDelimPos = -1;
  char delimiter = '\0';

  for(int i = 0; i < len; i++)
  {
    // copy the input at first tab press only
    if(_tabCount == -1)
      _inputStr[i] = buffer(_promptStartPos + i) & 0x7f;
    if(string_view{"{*@<> =[]()+-/&|!^~%"}.contains(_inputStr[i]))
    {
      lastDelimPos = i;
      delimiter = _inputStr[i];
    }
  }
  if(_tabCount == -1)
    _inputStr[len] = '\0';

  StringList lst;

  if(lastDelimPos == -1)
    // no delimiters, do only command completion:
    DebuggerParser::getCompletions(_inputStr, lst);
  else
  {
    const size_t strLen = len - lastDelimPos - 1;
    // do not show ALL commands/labels without any filter as it makes no sense
    if(strLen > 0)
    {
      // Special case for 'help' command
      if(BSPF::startsWithIgnoreCase(_inputStr, "help"))
        DebuggerParser::getCompletions(_inputStr + lastDelimPos + 1, lst);
      else
      {
        // we got a delimiter, so this must be a label or a function
        const Debugger& dbg = instance().debugger();

        dbg.cartDebug().getCompletions(_inputStr + lastDelimPos + 1, lst);
        dbg.getCompletions(_inputStr + lastDelimPos + 1, lst);
      }
    }

  }
  if(lst.empty())
    return false;
  std::ranges::sort(lst);

  if(direction < 0)
  {
    if(--_tabCount < 0)
      _tabCount = static_cast<int>(lst.size()) - 1;
  }
  else
    _tabCount = (_tabCount + 1) % lst.size();

  nextLine();
  _currentPos = _promptStartPos;
  _selectSize = 0;
  killLine(1);  // kill whole line

  // start with-autocompleted, fixed string...
  for(int i = 0; i < lastDelimPos; i++)
    putcharIntern(_inputStr[i]);
  if(lastDelimPos > 0)
    putcharIntern(delimiter);

  // ...and add current autocompletion string
  print(lst[_tabCount]);
  putcharIntern(' ');
  _promptEndPos = _currentPos;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::nextLine()
{
  // Reset colors every line, so I don't have to remember to do it myself
  _textcolor = kTextColor;
  _inverse = false;

  const int line = _currentPos / _lineWidth;
  if (line == _scrollLine && _scrollLine < _scrollStopLine)
    _scrollLine++;

  _currentPos = (line + 1) * _lineWidth;

  updateScrollBuffer();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Call this (at least) when the current line changes or when a new line is added
void PromptWidget::updateScrollBuffer()
{
  const int lastchar = std::max(_promptEndPos, _currentPos),
            line = lastchar / _lineWidth,
            numlines = (line < _linesInBuffer) ? line + 1 : _linesInBuffer,
            firstline = line - numlines + 1;

  if (firstline > _firstLineInBuffer)
  {
    // clear old line from buffer
    for (int i = lastchar; i < (line+1) * _lineWidth; ++i)
      buffer(i) = ' ';

    _firstLineInBuffer = firstline;
  }

  _scrollBar->_numEntries = numlines;
  _scrollBar->_currentPos = _scrollBar->_numEntries - (line - _scrollLine + _linesPerPage);
  _scrollBar->_entriesPerPage = _linesPerPage;
  _scrollBar->recalc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::putcharIntern(int c)
{
  if (c == '\n')
    nextLine();
  else if(c & 0x80) { // set foreground color to TIA color
                      // don't print or advance cursor
    _textcolor = static_cast<ColorId>((c & 0x7f) << 1);
  }
  else if(c && c < 0x1e) { // first actual character is large dash
    // More colors (the regular GUI ones)
    _textcolor = static_cast<ColorId>(c + 0x100);
  }
  else if(c == 0x7f) { // toggle inverse video (DEL char)
    _inverse = !_inverse;
  }
  else if(isprint(c) || c == 0x1e || c == 0x1f) // graphic bits chars
  {
    buffer(_currentPos) = c | (_textcolor << 8) | (_inverse << 17);
    _currentPos++;
    if ((_scrollLine + 1) * _lineWidth == _currentPos
        && _scrollLine < _scrollStopLine)
    {
      _scrollLine++;
      updateScrollBuffer();
    }
  }
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::print(string_view str)
{
  // limit scrolling of long text output
  _scrollStopLine = _currentPos / _lineWidth + _linesPerPage - 1;
  for(const auto c : str)
    putcharIntern(c);
  _scrollStopLine = INT_MAX;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::drawCaret()
{
//cerr << "PromptWidget::drawCaret()\n";
  if(_currentPos < _promptStartPos)
    return;

  FBSurface& s = _boss->dialog().surface();
  const int line = _currentPos / _lineWidth;

  // Don't draw the cursor if it's not in the current view
  if(_scrollLine < line)
    return;

  const int displayLine = line - _scrollLine + _linesPerPage - 1,
                          x = _x + 1 + (_currentPos % _lineWidth) * _kConsoleCharWidth,
                          y = _y + displayLine * _kConsoleLineHeight;

  const char c = buffer(_currentPos); //FIXME: int to char??
  s.fillRect(x, y, _kConsoleCharWidth, _kConsoleLineHeight, kTextColor);
  s.drawChar(_font, c, x, y + 2, kBGColor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::scrollToCurrent()
{
  const int line = _promptEndPos / _lineWidth;

  if (line + _linesPerPage <= _scrollLine)
  {
    // TODO - this should only occur for long edit lines, though
  }
  else if (line > _scrollLine)
  {
    _scrollLine = line;
    updateScrollBuffer();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string PromptWidget::saveBuffer(const FSNode& file)
{
  // Positions grow without bound while the buffer recycles, so only the lines
  // it still holds can be saved; buffer() maps a position onto the cell that
  // currently backs it
  const int first = _firstLineInBuffer * _lineWidth;
  const int last = std::max(_promptStartPos, first);

  string out;
  out.reserve(last - first);  // reasonable upper bound

  for(int start = first; start < last; start += _lineWidth)
  {
    // Clamp end to the last valid position in the buffer
    int end = std::min(start + _lineWidth, last) - 1;

    // Look for first non-space, printing char from end of line
    while(end >= start && static_cast<char>(buffer(end) & 0xff) <= ' ')
      end--;

    // Skip entirely blank lines rather than letting end stay < start
    if(end < start)
    {
      out += '\n';
      continue;
    }

    // Spit out the line minus its trailing junk
    // Strip off any color/inverse bits
    for(int j = start; j <= end; ++j)
      out += static_cast<char>(buffer(j) & 0xff);

    out += '\n';
  }

  try
  {
    if(file.write(out) > 0)
      return "saved " + file.getShortPath() + " OK";
    else
      return "unable to save session";
  }
  catch(...)
  {
    return "unable to save session";
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::clearScreen()
{
  // Initialize start position
  _currentPos = 0;
  _scrollLine = _linesPerPage - 1;
  _firstLineInBuffer = 0;
  _promptStartPos = _promptEndPos = -1;
  _buffer.fill(0);
  _bufferStale = false;

  if(!_firstTime)
    updateScrollBuffer();

  if(myUndoHandler)
    (*myUndoHandler).reset();  // Make sure to call ::reset, not smartptr reset

  resetFunctions();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::clearHistory()
{
  _history.clear();
  _historyIndex = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::resetFunctions()
{
  // reset special functions
  _tabCount = -1;
  _historyLine = 0;
}
