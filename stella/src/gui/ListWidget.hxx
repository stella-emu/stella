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
// $Id: ListWidget.hxx,v 1.3 2005-05-13 18:28:06 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef LIST_WIDGET_HXX
#define LIST_WIDGET_HXX

#include "GuiObject.hxx"
#include "Widget.hxx"
#include "ScrollBarWidget.hxx"
#include "Command.hxx"
#include "StringList.hxx"
#include "bspf.hxx"

enum NumberingMode {
  kListNumberingOff  = -1,
  kListNumberingZero = 0,
  kListNumberingOne  = 1
};

// Some special commands
enum {
  kListItemDoubleClickedCmd = 'LIdb',  // double click on item - 'data' will be item index
  kListItemActivatedCmd     = 'LIac',  // item activated by return/enter - 'data' will be item index
  kListSelectionChangedCmd  = 'Lsch'   // selection changed - 'data' will be item index
};


/* ListWidget */
class ListWidget : public Widget, public CommandSender
{
  public:
    ListWidget(GuiObject* boss, int x, int y, int w, int h);
    virtual ~ListWidget();

    void setList(const StringList& list);
    const StringList& getList()	const          { return _list; }
    int getSelected() const                    { return _selectedItem; }
    void setSelected(int item);
    const string& getSelectedString() const    { return _list[_selectedItem]; }
    bool isEditable() const	                   { return _editable; }
    void setEditable(bool editable)            { _editable = editable; }
    void setNumberingMode(NumberingMode numberingMode)  { _numberingMode = numberingMode; }
    void scrollTo(int item);
	
    virtual void handleTickle();
    virtual void handleMouseDown(int x, int y, int button, int clickCount);
    virtual void handleMouseUp(int x, int y, int button, int clickCount);
    virtual void handleMouseWheel(int x, int y, int direction);
    virtual void handleMouseEntered(int button) {};
    virtual void handleMouseLeft(int button)    {};
    virtual bool handleKeyDown(int ascii, int keycode, int modifiers);
    virtual bool handleKeyUp(int ascii, int keycode, int modifiers);
    virtual void handleCommand(CommandSender* sender, int cmd, int data);

    virtual bool wantsFocus() { return true; };

    void scrollBarRecalc();

    void startEditMode();
    void abortEditMode();

  protected:
    void drawWidget(bool hilite);
	
    int getCaretPos() const;
    void drawCaret(bool erase);

    void lostFocusWidget();
    void scrollToCurrent();

  protected:
    StringList       _list;
    bool             _editable;
    bool             _editMode;
    NumberingMode    _numberingMode;
    int              _currentPos;
    int              _entriesPerPage;
    int              _selectedItem;
    ScrollBarWidget* _scrollBar;
    int              _currentKeyDown;
    string           _backupString;

    bool             _caretVisible;
    int              _caretTime;

    string           _quickSelectStr;
    int              _quickSelectTime;
};

#endif
