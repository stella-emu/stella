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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef LIST_WIDGET_HXX
#define LIST_WIDGET_HXX

class GuiObject;

#include "Widget.hxx"
#include "Command.hxx"
#include "EditableWidget.hxx"
#include "ScrollBarWidget.hxx"
#include "Rect.hxx"

/** ListWidget */
class ListWidget : public EditableWidget
{
  public:
    enum {
      kDoubleClickedCmd    = 'LIdb',  // double click on item - 'data' will be item index
      kActivatedCmd        = 'LIac',  // item activated by return/enter - 'data' will be item index
      kDataChangedCmd      = 'LIch',  // item data changed - 'data' will be item index
      kRClickedCmd         = 'LIrc',  // right click on item - 'data' will be item index
      kSelectionChangedCmd = 'Lsch',  // selection changed - 'data' will be item index
      kScrolledCmd         = 'Lscl',  // list scrolled - 'data' will be current position
      kPrevDirCmd          = 'Lpdr'   // request to go to parent list, if applicable
  };

  public:
    ListWidget(GuiObject* boss, const GUI::Font& font,
               int x, int y, int w, int h, bool quickSelect);
    virtual ~ListWidget();

    int rows() const        { return _rows; }
    int currentPos() const  { return _currentPos; }

    int getSelected() const { return _selectedItem; }
    void setSelected(int item);
    void setSelected(const string& item);

    int getHighlighted() const     { return _highlightedItem; }
    void setHighlighted(int item);

    const StringList& getList()	const { return _list; }
    const string& getSelectedString() const;

    void scrollTo(int item);

    // Account for the extra width of embedded scrollbar
    int getWidth() const override { return _w + kScrollBarWidth; }

    static void setQuickSelectDelay(uInt64 time) { _QUICK_SELECT_DELAY = time; }

  protected:
    void handleMouseDown(int x, int y, int button, int clickCount) override;
    void handleMouseUp(int x, int y, int button, int clickCount) override;
    void handleMouseWheel(int x, int y, int direction) override;
    bool handleText(char text) override;
    bool handleKeyDown(StellaKey key, StellaMod mod) override;
    bool handleKeyUp(StellaKey key, StellaMod mod) override;
    bool handleEvent(Event::Type e) override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

    virtual void drawWidget(bool hilite) override  = 0;
    virtual GUI::Rect getEditRect() const override = 0;

    int findItem(int x, int y) const;
    void recalc();
    void scrollBarRecalc();

    void startEditMode() override;
    void endEditMode() override;
    void abortEditMode() override;

    void lostFocusWidget() override;
    void scrollToSelected()    { scrollToCurrent(_selectedItem);    }
    void scrollToHighlighted() { scrollToCurrent(_highlightedItem); }

  private:
    void scrollToCurrent(int item);

  protected:
    int  _rows;
    int  _cols;
    int  _currentPos;
    int  _selectedItem;
    int  _highlightedItem;
    bool _editMode;
    StellaKey _currentKeyDown;

    ScrollBarWidget* _scrollBar;

    StringList _list;
    string     _backupString;
    bool       _quickSelect;
    string     _quickSelectStr;
    uInt64     _quickSelectTime;

  private:
    static uInt64 _QUICK_SELECT_DELAY;

  private:
    // Following constructors and assignment operators not supported
    ListWidget() = delete;
    ListWidget(const ListWidget&) = delete;
    ListWidget(ListWidget&&) = delete;
    ListWidget& operator=(const ListWidget&) = delete;
    ListWidget& operator=(ListWidget&&) = delete;
};

#endif
