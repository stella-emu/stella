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
// $Id: ByteGridWidget.hxx,v 1.1 2005-06-16 18:40:17 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef BYTE_GRID_WIDGET_HXX
#define BYTE_GRID_WIDGET_HXX

#include "GuiObject.hxx"
#include "Widget.hxx"
#include "Command.hxx"
#include "StringList.hxx"
#include "EditableWidget.hxx"
#include "Array.hxx"
#include "Rect.hxx"

typedef GUI::Array<int> ByteAddrList;
typedef GUI::Array<int> ByteValueList;

// Some special commands
enum {
  kBGItemDoubleClickedCmd = 'BGdb',
  kBGItemActivatedCmd     = 'BGac',
  kBGItemDataChangedCmd   = 'BGch',
  kBGSelectionChangedCmd  = 'BGsc'
};

/* ByteGridWidget */
class ByteGridWidget : public EditableWidget, public CommandSender
{
  public:
    ByteGridWidget(GuiObject* boss, int x, int y, int w, int h,
                   int cols, int rows);
    virtual ~ByteGridWidget();

    void setList(const ByteAddrList& alist, const ByteValueList& vlist);

//    int getSelectedAddr() const   { return _addrList[_selectedItem]; }
//    int getSelectedValue() const  { return _valueList[_selectedItem]; }

    bool isEditable() const	         { return _editable; }
    void setEditable(bool editable)  { _editable = editable; }
	
    virtual void handleMouseDown(int x, int y, int button, int clickCount);
    virtual void handleMouseUp(int x, int y, int button, int clickCount);
    virtual bool handleKeyDown(int ascii, int keycode, int modifiers);
    virtual bool handleKeyUp(int ascii, int keycode, int modifiers);
    virtual void handleCommand(CommandSender* sender, int cmd, int data);

    virtual bool wantsFocus() { return true; }

    void startEditMode();
    void endEditMode();

  protected:
    void drawWidget(bool hilite);

    int findItem(int x, int y);

    void abortEditMode();

    GUI::Rect getEditRect() const;

    void lostFocusWidget();

    bool tryInsertChar(char c, int pos);

  protected:
    int  _rows;
    int  _cols;
    int  _currentRow;
    int  _currentCol;

    ByteAddrList  _addrList;
    ByteValueList _valueList;
    StringList    _addrStringList;
    StringList    _valueStringList;

    bool    _editable;
    bool    _editMode;
    int     _currentPos;
    int     _entriesPerPage;
    int     _selectedItem;
    int     _currentKeyDown;
    string  _backupString;

    string  _quickSelectStr;
    int     _quickSelectTime;
};

#endif
