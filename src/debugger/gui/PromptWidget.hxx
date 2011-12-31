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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
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

#ifndef PROMPT_WIDGET_HXX
#define PROMPT_WIDGET_HXX

#include <stdarg.h>

class ScrollBarWidget;

#include "Widget.hxx"
#include "Command.hxx"
#include "bspf.hxx"

class PromptWidget : public Widget, public CommandSender
{
  public:
    PromptWidget(GuiObject* boss, const GUI::Font& font,
                 int x, int y, int w, int h);
    virtual ~PromptWidget();

  public:
    int printf(const char *format, ...);
    int vprintf(const char *format, va_list argptr);
#undef putchar
    void putchar(int c);
    void print(const string& str);
    void printPrompt();
    bool saveBuffer(string& filename);

    // Clear screen and erase all history
    void clearScreen();

  protected:
    int& buffer(int idx) { return _buffer[idx % kBufferSize]; }

    void drawWidget(bool hilite);
    void drawCaret();
    void putcharIntern(int c);
    void insertIntoPrompt(const char *str);
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

    void handleMouseDown(int x, int y, int button, int clickCount);
    void handleMouseWheel(int x, int y, int direction);
    bool handleKeyDown(int ascii, int keycode, int modifiers);
    void handleCommand(CommandSender* sender, int cmd, int data, int id);

    // Account for the extra width of embedded scrollbar
    virtual int getWidth() const;

    virtual bool wantsFocus() { return true; }

    void loadConfig();

  private:
    // Get the longest prefix (initially 's') that is in every string in the list
    string getCompletionPrefix(const StringList& completions, string s);

  private:
    enum {
      kBufferSize     = 32768,
      kLineBufferSize = 256,
      kHistorySize    = 20
    };

    int  _buffer[kBufferSize];
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

    int _kConsoleCharWidth, _kConsoleCharHeight, _kConsoleLineHeight;

    bool _inverse;
    bool _makeDirty;
    bool _firstTime;
    bool _exitedEarly;

    int compareHistory(const char *histLine);
};

#endif
