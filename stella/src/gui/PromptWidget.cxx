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
// $Id: PromptWidget.cxx,v 1.23 2005-07-02 15:36:44 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <iostream>
#include <fstream>

#include "ScrollBarWidget.hxx"
#include "FrameBuffer.hxx"
#include "EventHandler.hxx"
#include "Version.hxx"
#include "Debugger.hxx"
#include "DebuggerDialog.hxx"
#include "DebuggerParser.hxx"

#include "PromptWidget.hxx"
#include "EquateList.hxx"

#define PROMPT  "> "

/* TODO:
 * - it is very inefficient to redraw the full thingy when just one char is added/removed.
 *   Instead, we could just copy the GFX of the blank console (i.e. after the transparent
 *   background is drawn, before any text is drawn). Then using that, it becomes trivial
 *   to erase a single character, do scrolling etc.
 * - a *lot* of others things, this code is in no way complete and heavily under progress
 */

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PromptWidget::PromptWidget(GuiObject* boss, int x, int y, int w, int h)
    : Widget(boss, x, y, w - kScrollBarWidth, h),
      CommandSender(boss)
{
  _flags = WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS;
  _type = kPromptWidget;

  _kConsoleCharWidth  = instance()->consoleFont().getMaxCharWidth();
  _kConsoleCharHeight = instance()->consoleFont().getFontHeight();
  _kConsoleLineHeight = _kConsoleCharHeight + 2;

  // Calculate depending values
  _lineWidth = (_w - kScrollBarWidth - 2) / _kConsoleCharWidth;
  _linesPerPage = (_h - 2) / _kConsoleLineHeight;

  memset(_buffer, 0, kBufferSize * sizeof(int));
  _linesInBuffer = kBufferSize / _lineWidth;

  _currentPos = 0;
  _scrollLine = _linesPerPage - 1;
  _firstLineInBuffer = 0;

  // Add scrollbar
  _scrollBar = new ScrollBarWidget(boss, _x + _w, _y, kScrollBarWidth, _h);

  _scrollBar->setTarget(this);

  // Init colors
  defaultTextColor = kTextColor;
  defaultBGColor = kBGColor;
  textColor = defaultTextColor;
  bgColor = defaultBGColor;
  _inverse = false;

  // Init History
  _historyIndex = 0;
  _historyLine = 0;
  _historySize = 0;
  for (int i = 0; i < kHistorySize; i++)
    _history[i][0] = '\0';

  _promptStartPos = _promptEndPos = -1;

  // Display greetings & prompt
  string version = string("Stella ") + STELLA_VERSION + "\n";
  print(version.c_str());
  print("Debugger is ready\n");
  //print( instance()->debugger().state() + "\n"); // FIXME: this doesn't work yet
  print(PROMPT);
  _promptStartPos = _promptEndPos = _currentPos;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PromptWidget::~PromptWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::drawWidget(bool hilite)
{
  OverlayColor fgcolor, bgcolor;

  FrameBuffer& fb = _boss->instance()->frameBuffer();

  // Fill the background
  fb.fillRect(_x, _y, _w, _h, kBGColor);

  // Draw text
  int start = _scrollLine - _linesPerPage + 1;
  int y = _y + 2;

  for (int line = 0; line < _linesPerPage; line++)
  {
    int x = _x + 1;
    for (int column = 0; column < _lineWidth; column++) {
      int c = buffer((start + line) * _lineWidth + column);

      if(c & (1 << 17)) { // inverse video flag
        fgcolor = bgColor;
        bgcolor = (OverlayColor)((c & 0x1ffff) >> 8);
        fb.fillRect(x, y, _kConsoleCharWidth, _kConsoleCharHeight, bgcolor);
      } else {
        fgcolor = (OverlayColor)(c >> 8);
        bgcolor = bgColor;
      }
      fb.drawChar(&instance()->consoleFont(), c & 0x7f, x, y, fgcolor);
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
return;
  _scrollBar->handleMouseDown(x, y, button, clickCount);
cerr << "PromptWidget::handleMouseDown\n";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::handleMouseWheel(int x, int y, int direction)
{
  _scrollBar->handleMouseWheel(x, y, direction);
}

void PromptWidget::printPrompt() {
	print( instance()->debugger().showWatches() );
	print( instance()->debugger().state() );
	print(PROMPT);
	_promptStartPos = _promptEndPos = _currentPos;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PromptWidget::handleKeyDown(int ascii, int keycode, int modifiers)
{
  int i;
  bool handled = true;
  bool dirty = false;
	
  switch (keycode)
  {
    case '\n': // enter/return
    case '\r':
    {
      nextLine();

      assert(_promptEndPos >= _promptStartPos);
      int len = _promptEndPos - _promptStartPos;

      if (len > 0)
      {
        // We have to allocate the string buffer with new, since VC++ sadly does not
        // comply to the C++ standard, so we can't use a dynamic sized stack array.
        char *str = new char[len + 1];

        // Copy the user input to str
        for (i = 0; i < len; i++)
          str[i] = buffer(_promptStartPos + i) & 0x7f;
        str[len] = '\0';

        // Add the input to the history
        addToHistory(str);

        // Pass the command to the debugger, and print the result
        print( instance()->debugger().run(str) );

        // Get rid of the string buffer
        delete [] str;
      }

      printPrompt();

      dirty = true;
      break;
    }

    case 27:  // escape  FIXME - possibly remove this one?
    case 9:   // tab
      {
        // Tab completion: we complete either commands or labels, but not
        // both at once.

        if(_currentPos <= _promptStartPos)
          break;

        scrollToCurrent();
        int len = _promptEndPos - _promptStartPos;

        int lastDelimPos = -1;
        char delimiter = '\0';

        char *str = new char[len + 1];
        for (i = 0; i < len; i++) {
          str[i] = buffer(_promptStartPos + i) & 0x7f;
          if(strchr("*@<> ", str[i]) != NULL ) {
            lastDelimPos = i;
            delimiter = str[i];
          }
        }
        str[len] = '\0';

        const char *completionList;
        const char *prefix;
        int possibilities;

        if(lastDelimPos < 0) {
          // no delimiters, do command completion:
          DebuggerParser *parser = instance()->debugger().parser();
          possibilities = parser->countCompletions(str);

          if(possibilities < 1) {
            delete[] str;
            break;
          }

          completionList = parser->getCompletions();
          prefix = parser->getCompletionPrefix();
		  } else {
          // we got a delimiter, so this must be a label:
          EquateList *equates = instance()->debugger().equates();
          possibilities = equates->countCompletions(str + lastDelimPos + 1);

          if(possibilities < 1) {
            delete[] str;
            break;
          }

			  completionList = equates->getCompletions();
			  prefix = equates->getCompletionPrefix();
		  }

        if(possibilities == 1) {
          // add to buffer as though user typed it (plus a space)
          _currentPos = _promptStartPos + lastDelimPos + 1;
          while(*completionList != '\0') {
            putcharIntern(*completionList++);
          }
          putcharIntern(' ');
          _promptEndPos = _currentPos;
        } else {
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
        draw();
        instance()->frameBuffer().refreshOverlay();
        delete[] str;
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
      if (instance()->eventHandler().kbdShift(modifiers))
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
      if (instance()->eventHandler().kbdShift(modifiers))
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
      if (instance()->eventHandler().kbdShift(modifiers))
      {
        _scrollLine = _firstLineInBuffer + _linesPerPage - 1;
        updateScrollBuffer();
      }
      else
        _currentPos = _promptStartPos;

      dirty = true;
      break;

    case 256 + 23:  // end
      if (instance()->eventHandler().kbdShift(modifiers))
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
      if (instance()->eventHandler().kbdShift(modifiers))
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
      if (instance()->eventHandler().kbdShift(modifiers))
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
      if (instance()->eventHandler().kbdControl(modifiers))
      {
        specialKeys(keycode);
      }
      else if (instance()->eventHandler().kbdAlt(modifiers))
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

  if(dirty)
  {
    draw();
    // TODO - dirty rectangle
    instance()->frameBuffer().refreshOverlay();
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
void PromptWidget::handleCommand(CommandSender* sender, int cmd, int data)
{
  switch (cmd)
  {
    case kSetPositionCmd:
      int newPos = (int)data + _linesPerPage - 1 + _firstLineInBuffer;
      if (newPos != _scrollLine)
      {
        _scrollLine = newPos;
        draw();
        instance()->frameBuffer().refreshOverlay();
      }
      break;
  }
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
    draw();
    instance()->frameBuffer().refreshOverlay();
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
    if (buffer(_currentPos - 1) == ' ')
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

  draw();
  // TODO - dirty rectangle
  instance()->frameBuffer().refreshOverlay();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::nextLine()
{
  // reset colors every line, so I don't have to remember to do it myself
  textColor = defaultTextColor;
  bgColor = defaultBGColor;
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
  int lastchar = MAX(_promptEndPos, _currentPos);
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

#if defined(WIN32)
  int count = _vsnprintf(buf, sizeof(buf), format, argptr);
#else
  int count = vsnprintf(buf, sizeof(buf), format, argptr);
#endif
  print(buf);
  return count;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::putchar(int c)
{
  putcharIntern(c);

  draw();  // FIXME - not nice to redraw the full console just for one char!
  // TODO - dirty rectangle
  instance()->frameBuffer().refreshOverlay();
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
    textColor = (OverlayColor) ((c & 0x7f) << 1);
  }
  else if(c < ' ') { // More colors (the regular GUI ones)
    textColor = (OverlayColor) (c);
  }
  else if(c == 0x7f) { // toggle inverse video (DEL char)
    _inverse = !_inverse;
  }
  else
  {
    buffer(_currentPos) = c | (textColor << 8) | (_inverse << 17);
    _currentPos++;
    if ((_scrollLine + 1) * _lineWidth == _currentPos)
    {
      _scrollLine++;
      updateScrollBuffer();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::print(string str) // laziness/convenience method
{
  print(str.c_str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::print(const char *str)
{
  while (*str)
    putcharIntern(*str++);

  draw();
  // TODO - dirty rectangle
  instance()->frameBuffer().refreshOverlay();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::drawCaret()
{
  FrameBuffer& fb = _boss->instance()->frameBuffer();

  int line = _currentPos / _lineWidth;

  // Don't draw the cursor if it's not in the current view
  if(_scrollLine < line)
    return;

  int displayLine = line - _scrollLine + _linesPerPage - 1;
  int x = _x + 1 + (_currentPos % _lineWidth) * _kConsoleCharWidth;
  int y = _y + displayLine * _kConsoleLineHeight;

  char c = buffer(_currentPos);
  fb.fillRect(x, y, _kConsoleCharWidth, _kConsoleLineHeight, kTextColor);
  fb.drawChar(&_boss->instance()->consoleFont(), c, x, y + 2, kBGColor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptWidget::scrollToCurrent()
{
  int line = _promptEndPos / _lineWidth;

  if (line + _linesPerPage <= _scrollLine)
  {
    // TODO - this should only occur for loong edit lines, though
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

    // look for first non-space char from end of line
    while(_buffer[end] == ' ' && end >= start)
      end--;

    // spit out the line minus its trailing spaces.
    // Strip off any color/inverse bits
    for(int j=start; j<=end; j++)
      out << (_buffer[j] & 0xff);

    // add a \n
    out << endl;
  }

  out.close();
  return true;
}
