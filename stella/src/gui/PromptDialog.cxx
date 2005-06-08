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
// $Id: PromptDialog.cxx,v 1.5 2005-06-08 21:16:06 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "ScrollBarWidget.hxx"
#include "FrameBuffer.hxx"
#include "EventHandler.hxx"
#include "Version.hxx"

#include "PromptDialog.hxx"

#define PROMPT  "> "

/* TODO:
 * - it is very inefficient to redraw the full thingy when just one char is added/removed.
 *   Instead, we could just copy the GFX of the blank console (i.e. after the transparent
 *   background is drawn, before any text is drawn). Then using that, it becomes trivial
 *   to erase a single character, do scrolling etc.
 * - a *lot* of others things, this code is in no way complete and heavily under progress
 */

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PromptDialog::PromptDialog(OSystem* osystem, DialogContainer* parent,
                           int x, int y, int w, int h)
    : Dialog(osystem, parent, x, y, w, h)
{
  _kConsoleCharWidth  = instance()->consoleFont().getMaxCharWidth();
  _kConsoleLineHeight = instance()->consoleFont().getFontHeight() + 2;

  // Calculate depending values
  _lineWidth = (_w - kScrollBarWidth - 2) / _kConsoleCharWidth;
  _linesPerPage = (_h - 2) / _kConsoleLineHeight;

  memset(_buffer, ' ', kBufferSize);
  _linesInBuffer = kBufferSize / _lineWidth;

  _currentPos = 0;
  _scrollLine = _linesPerPage - 1;
  _firstLineInBuffer = 0;

  // Add scrollbar
  _scrollBar = new ScrollBarWidget(this, _w - kScrollBarWidth - 1, 0, kScrollBarWidth, _h);
  _scrollBar->setTarget(this);

  // Init callback
  _callbackProc = 0;
  _callbackRefCon = 0;

  // Init History
  _historyIndex = 0;
  _historyLine = 0;
  _historySize = 0;
  for (int i = 0; i < kHistorySize; i++)
    _history[i][0] = '\0';

  _promptStartPos = _promptEndPos = -1;

  // Display greetings & prompt
  string version = string("Stella version ") + STELLA_VERSION + "\n";
  print(version.c_str());
  print("Debugger is ready\n");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PromptDialog::~PromptDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptDialog::loadConfig()
{
  if (_promptStartPos == -1)
  {
    print(PROMPT);
    _promptStartPos = _promptEndPos = _currentPos;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptDialog::drawDialog()
{
  FrameBuffer& fb = instance()->frameBuffer();

  // Fill the background
  fb.fillRect(_x, _y, _w, _h, kBGColor);

  // Draw text
  int start = _scrollLine - _linesPerPage + 1;
  int y = _y + 2;

  for (int line = 0; line < _linesPerPage; line++)
  {
    int x = _x + 1;
    for (int column = 0; column < _lineWidth; column++) {
#if 0
      int l = (start + line) % _linesInBuffer;
      char c = buffer(l * _lineWidth + column);
#else
      char c = buffer((start + line) * _lineWidth + column);
#endif
      fb.drawChar(instance()->consoleFont(), c, x, y, kTextColor);
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
void PromptDialog::handleMouseWheel(int x, int y, int direction)
{
  _scrollBar->handleMouseWheel(x, y, direction);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptDialog::handleKeyDown(int ascii, int keycode, int modifiers)
{
  int i;
	
  switch (keycode)
  {
    case '\n': // enter/return
    case '\r':
    {
      nextLine();

      assert(_promptEndPos >= _promptStartPos);
      int len = _promptEndPos - _promptStartPos;
      bool keepRunning = true;

      if (len > 0)
      {
        // We have to allocate the string buffer with new, since VC++ sadly does not
        // comply to the C++ standard, so we can't use a dynamic sized stack array.
        char *str = new char[len + 1];

        // Copy the user input to str
        for (i = 0; i < len; i++)
          str[i] = buffer(_promptStartPos + i);
        str[len] = '\0';

        // Add the input to the history
        addToHistory(str);

        // Pass it to the input callback, if any
        if (_callbackProc)
          keepRunning = (*_callbackProc)(this, str, _callbackRefCon);

cerr << "Command entered: \'" << str << "\'\n"; // FIXME - tie this into DebuggerParser
        // Get rid of the string buffer
        delete [] str;
      }

      print(PROMPT);
      _promptStartPos = _promptEndPos = _currentPos;

      draw();
      instance()->frameBuffer().refresh();
      break;
    }

    case 8:  // backspace
      if (_currentPos > _promptStartPos)
        killChar(-1);

      scrollToCurrent();
      draw();  // FIXME - not nice to redraw the full console just for one char!
      instance()->frameBuffer().refresh();
      break;

#if 0 // FIXME - this may not be included in the 2.0 release
    case 9:  // tab
    {
      if (_completionCallbackProc)
      {
        int len = _currentPos - _promptStartPos;
        assert(len >= 0);
        char *str = new char[len + 1];

        // Copy the user input to str
        for (i = 0; i < len; i++)
          str[i] = buffer(_promptStartPos + i);
        str[len] = '\0';

        char *completion = 0;
        if ((*_completionCallbackProc)(this, str, completion, _callbackRefCon))
        {
          insertIntoPrompt(completion);
          scrollToCurrent();
          draw();
          instance()->frameBuffer().refresh();
          delete[] completion;
        }
        delete[] str;
      }
      break;
    }
#endif
    case 127:
      killChar(+1);
      draw();
      instance()->frameBuffer().refresh();
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
        draw();
        instance()->frameBuffer().refresh();
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
        draw();
        instance()->frameBuffer().refresh();
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

      draw();
      instance()->frameBuffer().refresh();
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

      draw();
      instance()->frameBuffer().refresh();
      break;

    case 273:  // cursor up
      historyScroll(+1);
      break;

    case 274:  // cursor down
      historyScroll(-1);
      break;

    case 275:  // cursor right
      if (_currentPos < _promptEndPos)
        _currentPos++;
      draw();
      instance()->frameBuffer().refresh();
      break;

    case 276:  // cursor left
      if (_currentPos > _promptStartPos)
        _currentPos--;
      draw();
      instance()->frameBuffer().refresh();
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
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptDialog::insertIntoPrompt(const char* str)
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
void PromptDialog::handleCommand(CommandSender* sender, int cmd, int data)
{
  switch (cmd)
  {
    case kSetPositionCmd:
      int newPos = (int)data + _linesPerPage - 1 + _firstLineInBuffer;
      if (newPos != _scrollLine)
      {
        _scrollLine = newPos;
        draw();
        instance()->frameBuffer().refresh();
      }
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptDialog::specialKeys(int keycode)
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
    instance()->frameBuffer().refresh();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptDialog::killChar(int direction)
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
void PromptDialog::killLine(int direction)
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
void PromptDialog::killLastWord()
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
void PromptDialog::addToHistory(const char *str)
{
  strcpy(_history[_historyIndex], str);
  _historyIndex = (_historyIndex + 1) % kHistorySize;
  _historyLine = 0;

  if (_historySize < kHistorySize)
    _historySize++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptDialog::historyScroll(int direction)
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
  instance()->frameBuffer().refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptDialog::nextLine()
{
  int line = _currentPos / _lineWidth;
  if (line == _scrollLine)
    _scrollLine++;

  _currentPos = (line + 1) * _lineWidth;

  updateScrollBuffer();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Call this (at least) when the current line changes or when a new line is added
void PromptDialog::updateScrollBuffer()
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
int PromptDialog::printf(const char *format, ...)
{
  va_list argptr;

  va_start(argptr, format);
  int count = this->vprintf(format, argptr);
  va_end (argptr);
  return count;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int PromptDialog::vprintf(const char *format, va_list argptr)
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
void PromptDialog::putchar(int c)
{
  putcharIntern(c);

  draw();  // FIXME - not nice to redraw the full console just for one char!
  instance()->frameBuffer().refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptDialog::putcharIntern(int c)
{
  if (c == '\n')
    nextLine();
  else
  {
    buffer(_currentPos) = (char)c;
    _currentPos++;
    if ((_scrollLine + 1) * _lineWidth == _currentPos)
    {
      _scrollLine++;
      updateScrollBuffer();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptDialog::print(const char *str)
{
  while (*str)
    putcharIntern(*str++);

  draw();
  instance()->frameBuffer().refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptDialog::drawCaret()
{
  FrameBuffer& fb = instance()->frameBuffer();

  int line = _currentPos / _lineWidth;

  // Don't draw the cursor if it's not in the current view
  if(_scrollLine < line)
    return;

  int displayLine = line - _scrollLine + _linesPerPage - 1;
  int x = _x + 1 + (_currentPos % _lineWidth) * _kConsoleCharWidth;
  int y = _y + displayLine * _kConsoleLineHeight;

  char c = buffer(_currentPos);
  fb.fillRect(x, y, _kConsoleCharWidth, _kConsoleLineHeight, kTextColor);
  fb.drawChar(instance()->consoleFont(), c, x, y + 2, kBGColor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PromptDialog::scrollToCurrent()
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
