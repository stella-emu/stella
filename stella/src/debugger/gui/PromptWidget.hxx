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
// $Id: PromptWidget.hxx,v 1.5 2006-03-25 00:34:17 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef PROMPT_WIDGET_HXX
#define PROMPT_WIDGET_HXX

class ScrollBarWidget;

#include <stdarg.h>

#include "GuiObject.hxx"
#include "Widget.hxx"
#include "Command.hxx"
#include "bspf.hxx"

enum {
  kBufferSize	= 32768,
  kLineBufferSize = 256,

  kHistorySize = 20
};

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

  protected:
    inline int &buffer(int idx) { return _buffer[idx % kBufferSize]; }

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

    virtual GUI::Rect getRect() const;
    virtual bool wantsFocus() { return true; }

    void loadConfig();

  protected:
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

    int  defaultTextColor;
    int  defaultBGColor;
    int  textColor;
    int  bgColor;
    bool _inverse;
    bool _makeDirty;
    bool _firstTime;

    int compareHistory(const char *histLine);
};

#endif
