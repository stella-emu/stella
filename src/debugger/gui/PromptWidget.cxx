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

#include <iostream>
#include <fstream>
#include <algorithm>

#include "ScrollBarWidget.hxx"
#include "FrameBuffer.hxx"
#include "EventHandler.hxx"
#include "Version.hxx"
#include "Debugger.hxx"
#include "DebuggerDialog.hxx"
#include "DebuggerParser.hxx"
#include "StringList.hxx"

#include "PromptWidget.hxx"
#include "CartDebug.hxx"

#define PROMPT  "> "

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PromptWidget::PromptWidget(GuiObject* boss, const GUI::Font& font,
                           int x, int y, int w, int h)
  : Widget(boss, font, x, y, w - kScrollBarWidth, h),
    CommandSender(boss),
    _makeDirty(false),
    _firstTime(true),
    _exitedEarly(false)
{
  _flags = WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS |
           WIDGET_WANTS_TAB | WIDGET_WANTS_RAWDATA;
  _type = kPromptWidget;
  _textcolor = kTextColor;
  _bgcolor = kWidColor;

  _kConsoleCharWidth  = font.getMaxCharWidth();
  _kConsoleCharHeight = font.getFontHeight();
  _kConsoleLineHeight = _kConsoleCharHeight + 2;

  // Calculate depending values
  _lineWidth = (_w - kScrollBarWidth - 2) / _kConsoleCharWidth;
  _linesPerPage = (_h - 2) / _kConsoleLineHeight;
  _linesInBuffer = kBufferSize / _lineWidth;

  // Add scrollbar
  _scrollBar = new ScrollBarWidget(boss, font, _x + _w, _y, kScrollBarWidth, _h);
  _scrollBar->setTarget(this);

  // Init colors
  _inverse = false;

  clearScreen();

  addFocusWidget(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PromptWidget::~PromptWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::drawWidget(bool hilite)
{
//cerr << "PromptWidget::drawWidget\n";
  uInt32 fgcolor, bgcolor;

  FBSurface& s = _boss->dialog().surface();

  // Draw text
  int start = _scrollLine - _linesPerPage + 1;
  int y = _y + 2;

  for (int line = 0; line < _linesPerPage; line++)
  {
    int x = _x + 1;
    for (int column = 0; column < _lineWidth; column++) {
      int c = buffer((start + line) * _lineWidth + column);

      if(c & (1 << 17)) { // inverse video flag
        fgcolor = _bgcolor;
        bgcolor = (c & 0x1ffff) >> 8;
        s.fillRect(x, y, _kConsoleCharWidth, _kConsoleCharHeight, bgcolor);
      } else {
        fgcolor = c >> 8;
      }
      s.drawChar(&instance().consoleFont(), c & 0x7f, x, y, fgcolor);
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
void PromptWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
//  cerr << "PromptWidget::handleMouseDown\n";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::handleMouseWheel(int x, int y, int direction)
{
  _scrollBar->handleMouseWheel(x, y, direction);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::printPrompt()
{
  string watches = instance().debugger().showWatches();
  if(watches.length() > 0)
    print(watches);

  print(PROMPT);
  _promptStartPos = _promptEndPos = _currentPos;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PromptWidget::handleKeyDown(int ascii, int keycode, int modifiers)
{
  int i;
  bool handled = true;
  bool dirty = false;
  
  switch(ascii)
  {
    case '\n': // enter/return
    case '\r':
    {
      nextLine();

      assert(_promptEndPos >= _promptStartPos);
      int len = _promptEndPos - _promptStartPos;

      if (len > 0)
      {
        // Copy the user input to command
        string command;
        for (i = 0; i < len; i++)
          command += buffer(_promptStartPos + i) & 0x7f;

        // Add the input to the history
        addToHistory(command.c_str());

        // Pass the command to the debugger, and print the result
        string result = instance().debugger().run(command);

        // This is a bit of a hack
        // Certain commands remove the debugger dialog from underneath us,
        // so we shouldn't print any messages
        // Those commands will return '_EXIT_DEBUGGER' as their result
        if(result == "_EXIT_DEBUGGER")
        {
          _exitedEarly = true;
          return true;
        }
        else if(result != "")
          print(result + "\n");
      }

      printPrompt();
      dirty = true;
      break;
    }

    case '\t':   // tab
    {
      // Tab completion: we complete either commands or labels, but not
      // both at once.

      if(_currentPos <= _promptStartPos)
        break;

      scrollToCurrent();
      int len = _promptEndPos - _promptStartPos;
      if(len > 256) len = 256;

      int lastDelimPos = -1;
      char delimiter = '\0';

      char str[256];
      for (i = 0; i < len; i++)
      {
        str[i] = buffer(_promptStartPos + i) & 0x7f;
        if(strchr("{*@<> ", str[i]) != NULL )
        {
          lastDelimPos = i;
          delimiter = str[i];
        }
      }
      str[len] = '\0';

      StringList list;
      string completionList;
      string prefix;

      if(lastDelimPos < 0)
      {
        // no delimiters, do command completion:
        const DebuggerParser& parser = instance().debugger().parser();
        parser.getCompletions(str, list);

        if(list.size() < 1)
          break;

        sort(list.begin(), list.end());
        completionList = list[0];
        for(uInt32 i = 1; i < list.size(); ++i)
          completionList += " " + list[i];
        prefix = getCompletionPrefix(list, str);
      }
      else
      {
        // we got a delimiter, so this must be a label or a function
        const Debugger& dbg = instance().debugger();

        dbg.cartDebug().getCompletions(str + lastDelimPos + 1, list);
        dbg.getCompletions(str + lastDelimPos + 1, list);

        if(list.size() < 1)
          break;

        sort(list.begin(), list.end());
        completionList = list[0];
        for(uInt32 i = 1; i < list.size(); ++i)
          completionList += " " + list[i];
        prefix = getCompletionPrefix(list, str + lastDelimPos + 1);
      }

      if(list.size() == 1)
      {
        // add to buffer as though user typed it (plus a space)
        _currentPos = _promptStartPos + lastDelimPos + 1;
        const char* clptr = completionList.c_str();
        while(*clptr != '\0')
          putcharIntern(*clptr++);

        putcharIntern(' ');
        _promptEndPos = _currentPos;
      }
      else
      {
        nextLine();
        // add to buffer as-is, then add PROMPT plus whatever we have so far
        _currentPos = _promptStartPos + lastDelimPos + 1;

        print("\n");
        print(completionList);
        print("\n");
        print(PROMPT);

        _promptStartPos = _currentPos;

        for(i=0; i<lastDelimPos; i++)
          putcharIntern(str[i]);

        if(lastDelimPos > 0)
          putcharIntern(delimiter);

        print(prefix);
        _promptEndPos = _currentPos;
      }
      dirty = true;
      break;
    }

    case 8:  // backspace
      if (_currentPos > _promptStartPos)
        killChar(-1);

      scrollToCurrent();
      dirty = true;
      break;

    case 127:
      killChar(+1);
      dirty = true;
      break;

    case 256 + 24:  // pageup
      if (instance().eventHandler().kbdShift(modifiers))
      {
        // Don't scroll up when at top of buffer
        if(_scrollLine < _linesPerPage)
          break;

        _scrollLine -= _linesPerPage - 1;
        if (_scrollLine < _firstLineInBuffer + _linesPerPage - 1)
          _scrollLine = _firstLineInBuffer + _linesPerPage - 1;
        updateScrollBuffer();

        dirty = true;
      }
      break;

    case 256 + 25:  // pagedown
      if (instance().eventHandler().kbdShift(modifiers))
      {
        // Don't scroll down when at bottom of buffer
        if(_scrollLine >= _promptEndPos / _lineWidth)
          break;

        _scrollLine += _linesPerPage - 1;
        if (_scrollLine > _promptEndPos / _lineWidth)
          _scrollLine = _promptEndPos / _lineWidth;
        updateScrollBuffer();

        dirty = true;
      }
      break;

    case 256 + 22:  // home
      if (instance().eventHandler().kbdShift(modifiers))
      {
        _scrollLine = _firstLineInBuffer + _linesPerPage - 1;
        updateScrollBuffer();
      }
      else
        _currentPos = _promptStartPos;

      dirty = true;
      break;

    case 256 + 23:  // end
      if (instance().eventHandler().kbdShift(modifiers))
      {
        _scrollLine = _promptEndPos / _lineWidth;
        if (_scrollLine < _linesPerPage - 1)
          _scrollLine = _linesPerPage - 1;
        updateScrollBuffer();
      }
      else
        _currentPos = _promptEndPos;

      dirty = true;
      break;

    case 273:  // cursor up
      if (instance().eventHandler().kbdShift(modifiers))
      {
        if(_scrollLine <= _firstLineInBuffer + _linesPerPage - 1)
          break;

        _scrollLine -= 1;
        updateScrollBuffer();

        dirty = true;
      }
      else
        historyScroll(+1);
      break;

    case 274:  // cursor down
      if (instance().eventHandler().kbdShift(modifiers))
      {
        // Don't scroll down when at bottom of buffer
        if(_scrollLine >= _promptEndPos / _lineWidth)
          break;

        _scrollLine += 1;
        updateScrollBuffer();

        dirty = true;
      }
      else
        historyScroll(-1);
      break;

    case 275:  // cursor right
      if (_currentPos < _promptEndPos)
        _currentPos++;

      dirty = true;
      break;

    case 276:  // cursor left
      if (_currentPos > _promptStartPos)
        _currentPos--;

      dirty = true;
      break;

    default:
      if (instance().eventHandler().kbdControl(modifiers))
      {
        specialKeys(keycode);
      }
      else if (instance().eventHandler().kbdAlt(modifiers))
      {
      }
      else if (isprint(ascii))
      {
        for (i = _promptEndPos - 1; i >= _currentPos; i--)
          buffer(i + 1) = buffer(i);
        _promptEndPos++;
        putchar(ascii);
        scrollToCurrent();
      }
      else
        handled = false;
      break;
  }

  // Take care of changes made above
  if(dirty)
  {
    setDirty(); draw();
  }

  // There are times when we want the prompt and scrollbar to be marked
  // as dirty *after* they've been drawn above.  One such occurrence is
  // when we issue a command that indirectly redraws the entire parent
  // dialog (such as 'scanline' or 'frame').
  // In those cases, the return code of the command must be shown, but the
  // entire dialog contents are redrawn at a later time.  So the prompt and
  // scrollbar won't be redrawn unless they're dirty again.
  if(_makeDirty)
  {
    setDirty();
    _scrollBar->setDirty();
    _makeDirty = false;
  }

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::insertIntoPrompt(const char* str)
{
  unsigned int l = strlen(str);

  for (int i = _promptEndPos - 1; i >= _currentPos; i--)
    buffer(i + l) = buffer(i);

  for (unsigned int j = 0; j < l; ++j)
  {
    _promptEndPos++;
    putcharIntern(str[j]);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::handleCommand(CommandSender* sender, int cmd,
                                 int data, int id)
{
  switch (cmd)
  {
    case kSetPositionCmd:
      int newPos = (int)data + _linesPerPage - 1 + _firstLineInBuffer;
      if (newPos != _scrollLine)
      {
        _scrollLine = newPos;
        setDirty(); draw();
      }
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::loadConfig()
{
  // See logic at the end of handleKeyDown for an explanation of this
  _makeDirty = true;

  // Show the prompt the first time we draw this widget
  if(_firstTime)
  {
    // Display greetings & prompt
    string version = string("Stella ") + STELLA_VERSION + "\n";
    print(version.c_str());
    print(PROMPT);

    // Take care of one-time debugger stuff
    print(instance().debugger().autoExec().c_str());
    print(instance().debugger().cartDebug().loadConfigFile() + "\n");
    print(instance().debugger().cartDebug().loadSymbolFile() + "\n");
    print(PROMPT);

    _promptStartPos = _promptEndPos = _currentPos;
    _firstTime = false;
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
  return _w + kScrollBarWidth;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::specialKeys(int keycode)
{
  bool handled = false;

  switch (keycode)
  {
    case 'a':
      _currentPos = _promptStartPos;
      handled = true;
      break;
    case 'd':
      killChar(+1);
      handled = true;
      break;
    case 'e':
      _currentPos = _promptEndPos;
      handled = true;
      break;
    case 'k':
      killLine(+1);
      handled = true;
      break;
    case 'u':
      killLine(-1);
      handled = true;
      break;
    case 'w':
      killLastWord();
      handled = true;
      break;
  }

  if(handled)
  {
    setDirty(); draw();
  }
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
    int count = _currentPos - _promptStartPos;
    if(count > 0)
      for (int i = 0; i < count; i++)
       killChar(-1);
  }
  else if(direction == 1)  // erase from current position to end of line
  {
    for (int i = _currentPos; i < _promptEndPos; i++)
      buffer(i) = ' ';

    _promptEndPos = _currentPos;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::killLastWord()
{
  int cnt = 0;
  bool space = true;
  while (_currentPos > _promptStartPos)
  {
    if ((buffer(_currentPos - 1) & 0xff) == ' ')
    {
      if (!space)
        break;
    }
    else
      space = false;

    _currentPos--;
    cnt++;
  }

  for (int i = _currentPos; i < _promptEndPos; i++)
    buffer(i) = buffer(i + cnt);

  buffer(_promptEndPos) = ' ';
  _promptEndPos -= cnt;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::addToHistory(const char *str)
{
  strcpy(_history[_historyIndex], str);
  _historyIndex = (_historyIndex + 1) % kHistorySize;
  _historyLine = 0;

  if (_historySize < kHistorySize)
    _historySize++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int PromptWidget::compareHistory(const char *histLine) {
  return 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::historyScroll(int direction)
{
  if (_historySize == 0)
    return;

  if (_historyLine == 0 && direction > 0)
  {
    int i;
    for (i = 0; i < _promptEndPos - _promptStartPos; i++)
      _history[_historyIndex][i] = buffer(_promptStartPos + i);

    _history[_historyIndex][i] = '\0';
  }

  // Advance to the next line in the history
  int line = _historyLine + direction;
  if ((direction < 0 && line < 0) || (direction > 0 && line > _historySize))
    return;

  // If they press arrow-up with anything in the buffer, search backwards
  // in the history.
  /*
  if(direction < 0 && _currentPos > _promptStartPos) {
    for(;line > 0; line--) {
      if(compareHistory(_history[line]) == 0)
        break;
    }
  }
  */

  _historyLine = line;

  // Remove the current user text
  _currentPos = _promptStartPos;
  killLine(1);  // to end of line

  // ... and ensure the prompt is visible
  scrollToCurrent();

  // Print the text from the history
  int idx;
  if (_historyLine > 0)
    idx = (_historyIndex - _historyLine + _historySize) % _historySize;
  else
    idx = _historyIndex;

  for (int i = 0; i < kLineBufferSize && _history[idx][i] != '\0'; i++)
    putcharIntern(_history[idx][i]);

  _promptEndPos = _currentPos;

  // Ensure once more the caret is visible (in case of very long history entries)
  scrollToCurrent();

  setDirty(); draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::nextLine()
{
  // Reset colors every line, so I don't have to remember to do it myself
  _textcolor = kTextColor;
  _inverse = false;

  int line = _currentPos / _lineWidth;
  if (line == _scrollLine)
    _scrollLine++;

  _currentPos = (line + 1) * _lineWidth;

  updateScrollBuffer();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Call this (at least) when the current line changes or when a new line is added
void PromptWidget::updateScrollBuffer()
{
  int lastchar = BSPF_max(_promptEndPos, _currentPos);
  int line = lastchar / _lineWidth;
  int numlines = (line < _linesInBuffer) ? line + 1 : _linesInBuffer;
  int firstline = line - numlines + 1;

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
int PromptWidget::printf(const char *format, ...)
{
  va_list argptr;

  va_start(argptr, format);
  int count = this->vprintf(format, argptr);
  va_end (argptr);
  return count;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int PromptWidget::vprintf(const char *format, va_list argptr)
{
  char buf[2048];
  int count = BSPF_vsnprintf(buf, sizeof(buf), format, argptr);

  print(buf);
  return count;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::putchar(int c)
{
  putcharIntern(c);

  setDirty(); draw();  // FIXME - not nice to redraw the full console just for one char!
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::putcharIntern(int c)
{
  if (c == '\n')
    nextLine();
  else if(c & 0x80) { // set foreground color to TIA color
                      // don't print or advance cursor
                      // there are only 128 TIA colors, but
                      // OverlayColor contains 256 of them
    _textcolor = (c & 0x7f) << 1;
  }
  else if(c < 0x1e) { // first actual character is large dash
    // More colors (the regular GUI ones)
    _textcolor = c + 0x100;
  }
  else if(c == 0x7f) { // toggle inverse video (DEL char)
    _inverse = !_inverse;
  }
  else
  {
    buffer(_currentPos) = c | (_textcolor << 8) | (_inverse << 17);
    _currentPos++;
    if ((_scrollLine + 1) * _lineWidth == _currentPos)
    {
      _scrollLine++;
      updateScrollBuffer();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::print(const string& str)
{
  const char* c = str.c_str();
  while(*c)
    putcharIntern(*c++);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::drawCaret()
{
//cerr << "PromptWidget::drawCaret()\n";
  FBSurface& s = _boss->dialog().surface();

  int line = _currentPos / _lineWidth;

  // Don't draw the cursor if it's not in the current view
  if(_scrollLine < line)
    return;

  int displayLine = line - _scrollLine + _linesPerPage - 1;
  int x = _x + 1 + (_currentPos % _lineWidth) * _kConsoleCharWidth;
  int y = _y + displayLine * _kConsoleLineHeight;

  char c = buffer(_currentPos);
  s.fillRect(x, y, _kConsoleCharWidth, _kConsoleLineHeight, kTextColor);
  s.drawChar(&_boss->instance().consoleFont(), c, x, y + 2, kBGColor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::scrollToCurrent()
{
  int line = _promptEndPos / _lineWidth;

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
bool PromptWidget::saveBuffer(string& filename)
{
  ofstream out(filename.c_str());
  if(!out.is_open())
    return false;

  for(int start=0; start<_promptStartPos; start+=_lineWidth) {
    int end = start+_lineWidth-1;

    // look for first non-space, printing char from end of line
    while( char(_buffer[end] & 0xff) <= ' ' && end >= start)
      end--;

    // spit out the line minus its trailing junk.
    // Strip off any color/inverse bits
    for(int j=start; j<=end; j++)
      out << char(_buffer[j] & 0xff);

    // add a \n
    out << endl;
  }

  out.close();
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string PromptWidget::getCompletionPrefix(const StringList& completions, string prefix)
{
  // Search for prefix in every string, progressively growing it
  // Once a mismatch is found or length is past one of the strings, we're done
  // We *could* use the longest common string algorithm, but for the lengths
  // of the strings we're dealing with, it's probably not worth it
  for(;;)
  {
    for(uInt32 i = 0; i < completions.size(); ++i)
    {
      const string& s = completions[i];
      if(s.length() < prefix.length())
        return prefix;  // current prefix is the best we're going to get
      else if(!BSPF_startsWithIgnoreCase(s, prefix))
      {
        prefix.erase(prefix.length()-1);
        return prefix;
      }
    }
    if(completions[0].length() > prefix.length())
      prefix = completions[0].substr(0, prefix.length() + 1);
    else
      return prefix;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::clearScreen()
{
  // Initialize start position and history
  _currentPos = 0;
  _scrollLine = _linesPerPage - 1;
  _firstLineInBuffer = 0;
  _promptStartPos = _promptEndPos = -1;
  memset(_buffer, 0, kBufferSize * sizeof(int));

  _historyIndex = 0;
  _historyLine = 0;
  _historySize = 0;
  for (int i = 0; i < kHistorySize; i++)
    _history[i][0] = '\0';

  if(!_firstTime)
    updateScrollBuffer();
}
