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
// $Id: DataGridWidget.hxx,v 1.11 2005-08-10 18:44:37 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef DATA_GRID_WIDGET_HXX
#define DATA_GRID_WIDGET_HXX

#include "GuiObject.hxx"
#include "Widget.hxx"
#include "Command.hxx"
#include "Debugger.hxx"
#include "StringList.hxx"
#include "EditableWidget.hxx"
#include "Array.hxx"
#include "Rect.hxx"
#include "DataGridOpsWidget.hxx"

// Some special commands
enum {
  kDGItemDoubleClickedCmd = 'DGdb',
  kDGItemActivatedCmd     = 'DGac',
  kDGItemDataChangedCmd   = 'DGch',
  kDGSelectionChangedCmd  = 'DGsc'
};

/* DataGridWidget */
class DataGridWidget : public EditableWidget, public CommandSender
{
  public:
    DataGridWidget(GuiObject* boss, const GUI::Font& font,
                   int x, int y, int cols, int rows,
                   int colchars, int bits, BaseFormat format = kBASE_DEFAULT);
    virtual ~DataGridWidget();

    void setList(const IntArray& alist, const IntArray& vlist,
                 const BoolArray& changed);
    void setSelectedValue(int value);

    int getSelectedAddr() const   { return _addrList[_selectedItem]; }
    int getSelectedValue() const  { return _valueList[_selectedItem]; }

    virtual void handleMouseDown(int x, int y, int button, int clickCount);
    virtual void handleMouseUp(int x, int y, int button, int clickCount);
    virtual bool handleKeyDown(int ascii, int keycode, int modifiers);
    virtual bool handleKeyUp(int ascii, int keycode, int modifiers);
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

    virtual bool wantsFocus() { return true; }

    void startEditMode();
    void endEditMode();

    int colWidth() { return _colWidth; }

    void setOpsWidget(DataGridOpsWidget* w) { _opsWidget = w; }

  protected:
    void drawWidget(bool hilite);

    int findItem(int x, int y);

    void abortEditMode();

    GUI::Rect getEditRect() const;

    void receivedFocusWidget();
    void lostFocusWidget();

    bool tryInsertChar(char c, int pos);

  protected:
    int  _rows;
    int  _cols;
    int  _currentRow;
    int  _currentCol;
    int  _rowHeight;
    int  _colWidth;
    int  _bits;

    BaseFormat _base;

    IntArray    _addrList;
    IntArray    _valueList;
    StringList  _addrStringList;
    StringList  _valueStringList;
    BoolArray   _changedList;

    bool    _editMode;
    int     _selectedItem;
    int     _currentKeyDown;
    string  _backupString;

    DataGridOpsWidget* _opsWidget;

  private:
    /** Common operations on the currently selected cell */
    void negateCell();
    void invertCell();
    void decrementCell();
    void incrementCell();
    void lshiftCell();
    void rshiftCell();
    void zeroCell();
};

#endif
