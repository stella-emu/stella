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
// $Id: ListWidget.hxx,v 1.2 2005-05-10 19:20:44 stephena Exp $
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
    ListWidget(GuiObject* boss, Int32 x, Int32 y, Int32 w, Int32 h);
    virtual ~ListWidget();

    void setList(const StringList& list);
    const StringList& getList()	const          { return _list; }
    Int32 getSelected() const                  { return _selectedItem; }
    void setSelected(Int32 item);
    const string& getSelectedString() const    { return _list[_selectedItem]; }
    bool isEditable() const	                   { return _editable; }
    void setEditable(bool editable)            { _editable = editable; }
    void setNumberingMode(NumberingMode numberingMode)  { _numberingMode = numberingMode; }
    void scrollTo(Int32 item);
	
    virtual void handleTickle();
    virtual void handleMouseDown(Int32 x, Int32 y, Int32 button, Int32 clickCount);
    virtual void handleMouseUp(Int32 x, Int32 y, Int32 button, Int32 clickCount);
    virtual void handleMouseWheel(Int32 x, Int32 y, Int32 direction);
    virtual void handleMouseEntered(Int32 button) {};
    virtual void handleMouseLeft(Int32 button)    {};
    virtual bool handleKeyDown(uInt16 ascii, Int32 keycode, Int32 modifiers);
    virtual bool handleKeyUp(uInt16 ascii, Int32 keycode, Int32 modifiers);
    virtual void handleCommand(CommandSender* sender, uInt32 cmd, uInt32 data);

    virtual bool wantsFocus() { return true; };

    void scrollBarRecalc();

    void startEditMode();
    void abortEditMode();

  protected:
    void drawWidget(bool hilite);
	
    Int32 getCaretPos() const;
    void drawCaret(bool erase);

    void lostFocusWidget();
    void scrollToCurrent();

  protected:
    StringList       _list;
    bool             _editable;
    bool             _editMode;
    NumberingMode    _numberingMode;
    Int32            _currentPos;
    Int32            _entriesPerPage;
    Int32            _selectedItem;
    ScrollBarWidget* _scrollBar;
    Int32            _currentKeyDown;
    string           _backupString;

    bool             _caretVisible;
    uInt32           _caretTime;

    string           _quickSelectStr;
    uInt32           _quickSelectTime;
};

#endif
