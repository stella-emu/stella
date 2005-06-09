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
// $Id: PromptDialog.hxx,v 1.5 2005-06-09 15:08:23 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef PROMPT_DIALOG_HXX
#define PROMPT_DIALOG_HXX

class CommandSender;
class DialogContainer;
class ScrollBarWidget;
class DebuggerParser;

#include <stdarg.h>
#include "Dialog.hxx"

enum {
  kBufferSize	= 32768,
  kLineBufferSize = 256,

  kHistorySize = 20
};

class PromptDialog : public Dialog
{
  public:
    PromptDialog(OSystem* osystem, DialogContainer* parent,
                 int x, int y, int w, int h);
    virtual ~PromptDialog();

  public:
    int printf(const char *format, ...);
    int vprintf(const char *format, va_list argptr);
#undef putchar
    void putchar(int c);

  protected:
    inline char &buffer(int idx) { return _buffer[idx % kBufferSize]; }

    void drawDialog();
    void drawCaret();
    void putcharIntern(int c);
    void insertIntoPrompt(const char *str);
    void print(const char *str);
    void print(string str);
    void updateScrollBuffer();
    void scrollToCurrent();

    // Line editing
    void specialKeys(int keycode);
    void nextLine();
    void killChar(int direction);
    void killLine(int direction);
    void killLastWord();

    // History
    void addToHistory(const char *str);
    void historyScroll(int direction);

  private:
    void handleMouseWheel(int x, int y, int direction);
    void handleKeyDown(int ascii, int keycode, int modifiers);
    void handleCommand(CommandSender* sender, int cmd, int data);
    void loadConfig();

  protected:
    char _buffer[kBufferSize];
    int  _linesInBuffer;

    int  _lineWidth;
    int  _linesPerPage;

    int  _currentPos;
    int  _scrollLine;
    int  _firstLineInBuffer;
	
    int  _promptStartPos;
    int  _promptEndPos;

    ScrollBarWidget* _scrollBar;

    char _history[kHistorySize][kLineBufferSize];
    int _historySize;
    int _historyIndex;
    int _historyLine;

    int _kConsoleCharWidth, _kConsoleLineHeight;
};

#endif
