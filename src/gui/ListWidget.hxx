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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef LIST_WIDGET_HXX
#define LIST_WIDGET_HXX

class GuiObject;
class ScrollBarWidget;

#include "Rect.hxx"
#include "Command.hxx"
#include "EditableWidget.hxx"

/** ListWidget */
class ListWidget : public EditableWidget
{
  public:
    struct Cmd {
      static constexpr GuiCmd::Code
        DoubleClicked    = GuiCmd::of("ListWidget.DoubleClicked"),
        LongButtonPress  = GuiCmd::of("ListWidget.LongButtonPress"),
        Activated        = GuiCmd::of("ListWidget.Activated"),
        DataChanged      = GuiCmd::of("ListWidget.DataChanged"),
        RightClicked     = GuiCmd::of("ListWidget.RightClicked"),
        SelectionChanged = GuiCmd::of("ListWidget.SelectionChanged"),
        Scrolled         = GuiCmd::of("ListWidget.Scrolled"),
        ParentDir        = GuiCmd::of("ListWidget.ParentDir");
    };

  public:
    // Brings Widget's setPos(int,int)/setPos(Point) into scope alongside our
    // own override below, which would otherwise hide them
    using Widget::setPos;

    ListWidget(GuiObject* boss, const GUI::Font& font,
               bool useScrollbar = true);
    ~ListWidget() override = default;

    int rows() const        { return _rows; }
    int currentPos() const  { return _currentPos; }
    // Also moves the sibling scrollbar, which tracks the list's position
    void setPos(const Common::Point& pos) override;
    // Reserves scrollbar room out of 'w' (only while the scrollbar is needed)
    void setWidth(int w) override;
    // Recomputes _rows from the new height, then recalc()s
    void setHeight(int h) override;

    int getSelected() const { return _selectedItem; }
    // Selects 'item' and scrolls it into view, centered where possible
    void setSelected(int item);
    // Selects the first row equal to 'item' (or row 0 if none matches)
    void setSelected(string_view item);

    int getHighlighted() const     { return _highlightedItem; }
    // Sets the highlighted (not necessarily selected) row, scrolling by a
    // page once it would otherwise cross the view's edge
    void setHighlighted(int item);

    const StringList& getList()	const { return _list; }
    const string& getSelectedString() const;

    // Scrolls so 'item' is the first visible row (clamped to the list's ends)
    void scrollTo(int item);
    void scrollToEnd() { scrollToCurrent(static_cast<int>(_list.size())); }

    // Account for the extra width of embedded scrollbar
    int getWidth() const override;

    // Total height of a list showing the given number of rows.  A list has no
    // height of its own (it stretches to whatever the dialog gives it), so this
    // is how a dialog states how many rows it means to show, rather than
    // arriving at a pixel height and hoping the rows come out even
    static int calcHeight(const GUI::Font& font, int rows)
    {
      return rows * font.getLineHeight() + 2;
    }

    // Selection/scrolling/edit-mode via mouse, wheel (forwarded to the
    // scrollbar), joystick, and UI navigation events (see handleEvent())
    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseWheel(int x, int y, int direction) override;
    bool handleText(char text) override;
    bool handleKeyDown(StellaKey key, StellaMod mod) override;
    void handleJoyDown(int stick, int button, bool longPress) override;
    void handleJoyUp(int stick, int button) override;
    bool handleEvent(Event::Type e) override;

  protected:
    // Reacts to the scrollbar reporting a new position
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

    // A concrete list decides how its own rows are drawn and where their text sits
    void drawWidget(bool hilite) override  = 0;
    Common::Rect getEditRect() const override = 0;

    // Maps a click at (x,y) to a row index (not bounds-checked against the list)
    int findItem(int x, int y) const;
    // Revalidates _currentPos/_selectedItem and the scrollbar after the list
    // (or the widget's size) changes, then aborts any in-progress edit
    void recalc();
    // Pushes _currentPos to the scrollbar and reports the new position via Cmd::Scrolled
    void scrollBarRecalc();

    /**
      Is there more in the list than fits the rows we have?  The scrollbar
      shows only while there is, and takes its room out of our footprint only
      while it shows.
    */
    bool scrollBarNeeded() const;

    /**
      Re-split our footprint between the list and the scrollbar, after the
      answer above has changed.  Goes through setWidth(), so a subclass whose
      content depends on its width (wrapped text) re-flows with it.
    */
    void updateScrollBarRoom();

    // Enters/leaves in-place editing of the selected row's text
    void startEditMode() override;
    // Writes the edited text back into the row and reports Cmd::DataChanged
    void endEditMode() override;
    void abortEditMode() override;

    void lostFocusWidget() override;
    // Scrolls the selected/highlighted row into view (see scrollToCurrent())
    void scrollToSelected()    { scrollToCurrent(_selectedItem);    }
    void scrollToHighlighted() { scrollToCurrent(_highlightedItem); }

  private:
    // Scrolls the minimum amount needed to bring 'item' into the view, then
    // clamps and pushes the result to the scrollbar
    void scrollToCurrent(int item);

  protected:
    // Number of rows currently visible, derived from height / line height
    int  _rows{0};
    // Index of the first visible row (scroll position)
    int  _currentPos{0};
    // Index of the selected row, or -1 for none
    int  _selectedItem{-1};
    // Index of the highlighted (not necessarily selected) row, or -1 for none
    int  _highlightedItem{-1};
    // Whether this list has a scrollbar at all
    bool _useScrollbar{true};

    // The footprint setWidth() was last given, which the scrollbar's room is
    // taken out of only while it is needed; and a guard against re-entering
    // the re-split, since re-flowing wrapped text lands us back in recalc()
    int  _fullWidth{0};
    bool _inScrollBarRoom{false};

    // Sibling scrollbar widget, if _useScrollbar
    ScrollBarWidget* _scrollBar{nullptr};

    // The row strings shown
    StringList _list;

  private:
    // Following constructors and assignment operators not supported
    ListWidget() = delete;
    ListWidget(const ListWidget&) = delete;
    ListWidget(ListWidget&&) = delete;
    ListWidget& operator=(const ListWidget&) = delete;
    ListWidget& operator=(ListWidget&&) = delete;
};

#endif  // LIST_WIDGET_HXX
