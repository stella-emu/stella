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
// $Id: CheckListWidget.hxx,v 1.1 2005-08-22 13:53:23 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef CHECK_LIST_WIDGET_HXX
#define CHECK_LIST_WIDGET_HXX

#include "GuiObject.hxx"
#include "Widget.hxx"
#include "Command.hxx"
#include "StringList.hxx"
#include "EditableWidget.hxx"
#include "Rect.hxx"

class ScrollBarWidget;

// Some special commands
enum {
  kListItemChecked = 'LIct',
  kListItemDoubleClickedCmd = 'LIdb',  // double click on item - 'data' will be item index
  kListItemActivatedCmd     = 'LIac',  // item activated by return/enter - 'data' will be item index
  kListItemDataChangedCmd   = 'LIch',  // item data changed - 'data' will be item index
  kListSelectionChangedCmd  = 'Lsch'   // selection changed - 'data' will be item index

};

/* CheckListWidget */
class CheckListWidget : public EditableWidget
{
  public:
    CheckListWidget(GuiObject* boss, const GUI::Font& font,
                    int x, int y, int cols, int rows);
    virtual ~CheckListWidget();

    void setList(const StringList& list);
    const StringList& getList()	const          { return _list; }
    int getSelected() const                    { return _selectedItem; }
    void setSelected(int item);
    const string& getSelectedString() const    { return _list[_selectedItem]; }
    void scrollTo(int item);
	
    virtual void handleMouseDown(int x, int y, int button, int clickCount);
    virtual void handleMouseUp(int x, int y, int button, int clickCount);
    virtual void handleMouseWheel(int x, int y, int direction);
    virtual bool handleKeyDown(int ascii, int keycode, int modifiers);
    virtual bool handleKeyUp(int ascii, int keycode, int modifiers);
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

    virtual bool wantsFocus() { return true; }

    void startEditMode();
    void endEditMode();

  protected:
    void drawWidget(bool hilite);

    int findItem(int x, int y) const;
    void scrollBarRecalc();

    void abortEditMode();

    GUI::Rect getEditRect() const;

    void lostFocusWidget();
    void scrollToCurrent();

  protected:
    int  _rows;
    int  _cols;
    int  _rowHeight;
    int  _colWidth;
    int  _currentPos;
    int  _selectedItem;


    StringList       _list;
    BoolArray        _checkList;

    bool             _editMode;
    ScrollBarWidget* _scrollBar;
    int              _currentKeyDown;
    string           _backupString;

    string           _quickSelectStr;
    int              _quickSelectTime;
};

#endif
