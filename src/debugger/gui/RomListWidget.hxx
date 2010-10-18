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
// Copyright (c) 1995-2010 by Bradford W. Mott, Stephen Anthony
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

#ifndef ROM_LIST_WIDGET_HXX
#define ROM_LIST_WIDGET_HXX

class ContextMenu;
class ScrollBarWidget;
class PackedBitArray;
class CheckListWidget;

#include "Array.hxx"
#include "CartDebug.hxx"
#include "EditableWidget.hxx"

// Some special commands for this widget
enum {
  kRLBreakpointChangedCmd = 'RLbp',  // click on the checkbox for a breakpoint
  kRLRomChangedCmd        = 'RLpr'   // ROM item data changed - 'data' will be item index
};

/** RomListWidget */
class RomListWidget : public EditableWidget
{
  friend class RomWidget;

  public:
    RomListWidget(GuiObject* boss, const GUI::Font& font,
                    int x, int y, int w, int h);
    virtual ~RomListWidget();

    void setList(const CartDebug::Disassembly& disasm, const PackedBitArray& state);

    int getSelected() const        { return _selectedItem; }
    int getHighlighted() const     { return _highlightedItem; }
    void setSelected(int item);
    void setHighlighted(int item);

    const string& getEditString() const;
    void startEditMode();
    void endEditMode();

  protected:
    void handleMouseDown(int x, int y, int button, int clickCount);
    void handleMouseUp(int x, int y, int button, int clickCount);
    void handleMouseWheel(int x, int y, int direction);
    bool handleKeyDown(int ascii, int keycode, int modifiers);
    bool handleKeyUp(int ascii, int keycode, int modifiers);
    bool handleEvent(Event::Type e);
    void handleCommand(CommandSender* sender, int cmd, int data, int id);

    void drawWidget(bool hilite);
    GUI::Rect getLineRect() const;
    GUI::Rect getEditRect() const;

    int findItem(int x, int y) const;
    void recalc();

    bool tryInsertChar(char c, int pos);

    void abortEditMode();
    void lostFocusWidget();
    void scrollToSelected()    { scrollToCurrent(_selectedItem);    }
    void scrollToHighlighted() { scrollToCurrent(_highlightedItem); }

  private:
    void scrollToCurrent(int item);

  private:
    ContextMenu*     myMenu;
    ScrollBarWidget* myScrollBar;

    int  _labelWidth;
    int  _bytesWidth;
    int  _rows;
    int  _cols;
    int  _currentPos;
    int  _selectedItem;
    int  _highlightedItem;
    int  _currentKeyDown;
    bool _editMode;

    const CartDebug::Disassembly* myDisasm;
    const PackedBitArray* myBPState;
    Common::Array<CheckboxWidget*> myCheckList;
};

#endif
