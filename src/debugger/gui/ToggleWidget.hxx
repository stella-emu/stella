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

#ifndef TOGGLE_WIDGET_HXX
#define TOGGLE_WIDGET_HXX

#include "Widget.hxx"
#include "Command.hxx"
#include "Array.hxx"

// Some special commands
enum {
  kTWItemDataChangedCmd   = 'TWch',
  kTWSelectionChangedCmd  = 'TWsc'
};

/* ToggleWidget */
class ToggleWidget : public Widget, public CommandSender
{
  public:
    ToggleWidget(GuiObject* boss, const GUI::Font& font,
                 int x, int y, int cols, int rows);
    virtual ~ToggleWidget();

    const BoolArray& getState()    { return _stateList; }
    bool getSelectedState() const  { return _stateList[_selectedItem]; }

    virtual void handleMouseDown(int x, int y, int button, int clickCount);
    virtual void handleMouseUp(int x, int y, int button, int clickCount);
    virtual bool handleKeyDown(StellaKey key, StellaMod mod, char ascii);
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

    virtual bool wantsFocus() { return true; }

    int colWidth() const { return _colWidth; }
    void setEditable(bool editable) { _editable = editable; }

  protected:
    void drawWidget(bool hilite) = 0;
    int findItem(int x, int y);

  protected:
    int  _rows;
    int  _cols;
    int  _currentRow;
    int  _currentCol;
    int  _rowHeight;
    int  _colWidth;
    int  _selectedItem;
    bool _editable;

    BoolArray  _stateList;
    BoolArray  _changedList;
};

#endif
