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
// $Id: ToggleBitWidget.hxx,v 1.1 2005-06-24 11:05:00 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef TOGGLE_BIT_WIDGET_HXX
#define TOGGLE_BIT_WIDGET_HXX

#include "GuiObject.hxx"
#include "Widget.hxx"
#include "Command.hxx"
#include "Debugger.hxx"
#include "StringList.hxx"
#include "Array.hxx"

typedef GUI::Array<bool> BoolList;

// Some special commands
enum {
  kTBItemDataChangedCmd   = 'TBch',
  kTBSelectionChangedCmd  = 'TBsc'
};

/* ToggleBitWidget */
class ToggleBitWidget : public Widget, public CommandSender
{
  public:
    ToggleBitWidget(GuiObject* boss, int x, int y, int cols, int rows,
                    int colchars = 1);
    virtual ~ToggleBitWidget();

    void setList(const StringList& off, const StringList& on);
    void setState(const BoolList& state);

    bool getSelectedState() const  { return _stateList[_selectedItem]; }

    virtual void handleMouseDown(int x, int y, int button, int clickCount);
    virtual void handleMouseUp(int x, int y, int button, int clickCount);
    virtual bool handleKeyDown(int ascii, int keycode, int modifiers);
    virtual void handleCommand(CommandSender* sender, int cmd, int data);

    virtual bool wantsFocus() { return true; }

    int colWidth() { return _colWidth; }

  protected:
    void drawWidget(bool hilite);
    int findItem(int x, int y);

  protected:
    int  _rows;
    int  _cols;
    int  _currentRow;
    int  _currentCol;
    int  _colWidth;
    int  _selectedItem;

    StringList  _offList;
    StringList  _onList;
    BoolList    _stateList;
};

#endif
