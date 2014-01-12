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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef ROM_LIST_WIDGET_HXX
#define ROM_LIST_WIDGET_HXX

class RomListSettings;
class ScrollBarWidget;
class PackedBitArray;
class CheckListWidget;

#include "Array.hxx"
#include "Base.hxx"
#include "CartDebug.hxx"
#include "EditableWidget.hxx"

/** RomListWidget */
class RomListWidget : public EditableWidget
{
  public:
    enum {
      kBPointChangedCmd  = 'RLbp',  // 'data' will be disassembly line number,
                                    // 'id' will be the checkbox state
      kRomChangedCmd     = 'RLpr',  // 'data' will be disassembly line number
                                    // 'id' will be the Base::Format of the data
      kSetPCCmd          = 'STpc',  // 'data' will be disassembly line number
      kRuntoPCCmd        = 'RTpc',  // 'data' will be disassembly line number
      kDisassembleCmd    = 'REds',
      kTentativeCodeCmd  = 'TEcd',  // 'data' will be boolean
      kPCAddressesCmd    = 'PCad',  // 'data' will be boolean
      kGfxAsBinaryCmd    = 'GFXb',  // 'data' will be boolean
      kAddrRelocationCmd = 'ADre'   // 'data' will be boolean
    };

  public:
    RomListWidget(GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
                    int x, int y, int w, int h);
    virtual ~RomListWidget();

    void setList(const CartDebug::Disassembly& disasm, const PackedBitArray& state);

    int getSelected() const        { return _selectedItem; }
    int getHighlighted() const     { return _highlightedItem; }
    void setSelected(int item);
    void setHighlighted(int item);

    const string& getText() const;
    void startEditMode();
    void endEditMode();

  protected:
    void handleMouseDown(int x, int y, int button, int clickCount);
    void handleMouseUp(int x, int y, int button, int clickCount);
    void handleMouseWheel(int x, int y, int direction);
    bool handleKeyDown(StellaKey key, StellaMod mod, char ascii);
    bool handleKeyUp(StellaKey key, StellaMod mod, char ascii);
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
    RomListSettings* myMenu;
    ScrollBarWidget* myScrollBar;

    int  _labelWidth;
    int  _bytesWidth;
    int  _rows;
    int  _cols;
    int  _currentPos;
    int  _selectedItem;
    int  _highlightedItem;
    bool _editMode;
    StellaKey  _currentKeyDown;
    Common::Base::Format _base;  // base used during editing

    const CartDebug::Disassembly* myDisasm;
    const PackedBitArray* myBPState;
    Common::Array<CheckboxWidget*> myCheckList;
};

#endif
