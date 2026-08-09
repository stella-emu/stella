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

#ifndef CONTEXT_MENU_HXX
#define CONTEXT_MENU_HXX

#include "bspf.hxx"
#include "Command.hxx"
#include "Dialog.hxx"
#include "Variant.hxx"

/**
 * Popup context menu which, when clicked, "pop up" a list of items and
 * lets the user pick on of them.
 *
 * Implementation wise, when the user selects an item, then the given 'cmd'
 * is broadcast, with data being equal to the tag value of the selected entry.
 *
 * There are also several utility methods (named as sendSelectionXXX) that
 * allow to cycle through the current items without actually opening the dialog.
 */
class ContextMenu : public Dialog, public CommandSender
{
  public:
    struct Cmd {
      static constexpr GuiCmd::Code
        ItemSelected = GuiCmd::of("ContextMenu.ItemSelected");
    };

  public:
    // 'width' is a minimum (the menu still grows to fit its widest entry); 'cmd'
    // is sent on selection, defaulting to Cmd::ItemSelected when left at 0
    ContextMenu(GuiObject* boss, const GUI::Font& font,
                const VariantList& items = VariantList{},
                GuiCmd::Code cmd = GuiCmd::None, int width = 0);
    ~ContextMenu() override = default;

    bool isShading() const override { return false; }

    /** Set the parent widget's ID */
    void setID(uInt32 id) { _id = id; }

    /** Add the given items to the widget. */
    void addItems(const VariantList& items);

    /** The items I currently hold, so a PopUpWidget above me can re-derive
        its width from them when the font changes. */
    const VariantList& entries() const { return _entries; }

    /** Set the minimum menu width (the owning PopUpWidget's value-box width);
        the menu still grows to fit its widest entry. */
    void setMaxWidth(int width);

    /** Also recompute our own cached font-derived state (row height, arrow
        size, entry widths) after the font object was mutated in place by a
        live font change.  Reached by the container's broadcast, like every
        other Dialog -- no owner has to forward it here. */
    void refreshFont() override;

    /** Enable or disable an item.  Disabled items are greyed out and
        cannot be selected. */
    void setEnabled(int index, bool enable);
    void setEnabled(const Variant& tag, bool enable);
    bool isEnabled(int index) const;

    /** Show context menu onscreen at the specified coordinates */
    void show(uInt32 x, uInt32 y, const Common::Rect& bossRect, int item = -1);

    /** Select the first entry matching the given tag. */
    void setSelected(const Variant& tag, const Variant& defaultTag);

    /** Select the entry at the given index. */
    void setSelectedIndex(int idx);

    /** Select the highest/last entry in the internal list. */
    void setSelectedMax();

    /** Clear selection (reset to default). */
    void clearSelection();

    /** Accessor methods for the currently selected item. */
    int getSelected() const;
    const string& getSelectedName() const;
    void setSelectedName(string_view name);
    const Variant& getSelectedTag() const;

    /** This dialog uses its own positioning, so we override Dialog::setPosition() */
    void setPosition() override;

    /** The following methods are used when we want to select *and*
        send a command for the new selection.  They are only to be used
        when the dialog *isn't* open, and are basically a shortcut so
        that a PopUpWidget has some basic functionality without forcing
        to open its associated ContextMenu. */
    bool sendSelectionUp();
    bool sendSelectionDown();
    bool sendSelectionFirst();
    bool sendSelectionLast();

    // Draws the frame, entries (highlighting the hovered/selected one), and
    // scroll arrows if _showScroll; this dialog handles its own drawing
    // rather than going through the usual widget-tree draw() path
    void drawDialog() override;

  protected:
    // Click selects the item under the mouse (or a scroll arrow), or closes
    // the menu if outside it; move/wheel update the hover highlight/scroll;
    // keyboard/joystick route through handleEvent()
    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseMoved(int x, int y) override;
    bool handleMouseClicks(int x, int y, MouseButton b) override;
    void handleMouseWheel(int x, int y, int direction) override;
    void handleKeyDown(StellaKey key, StellaMod mod, bool repeated) override;
    void handleJoyUp(int stick, int button) override;
    void handleJoyAxis(int stick, JoyAxis axis, JoyDir adir, int button) override;
    bool handleJoyHat(int stick, int hat, JoyHatDir hdir, int button) override;
    // UI navigation events: move the selection, page, jump to an end, select, or cancel
    void handleEvent(Event::Type e) override;

  private:
    /**
      The scroll arrows I draw at my ends: filled triangles, square, and
      derived from the font so they follow the text.  At the default 9x18
      font this is 8, the size the old small bitmap had.
    */
    static int arrowSize(const GUI::Font& font) {
      return (font.getMaxCharWidth() * 8 / 9) & ~1;
    }

    /**
      The gap I keep between my frame and my entries' text -- tighter than a
      text field's, which is what a menu wants.  At the default 9x18 font this
      is 2, the value it used to hard-code.
    */
    static int textInset(const GUI::Font& font) {
      return font.getMaxCharWidth() / 4;
    }

    // Picks the arrow size and text inset from the current font
    void setArrows();

    // Decides whether the menu must scroll (more entries than 'image' has room
    // for) and sets _numEntries/_h/_showScroll accordingly
    void recalc(const Common::Rect& image);
    // Re-derives _w from the widest entry (and _maxWidth, if larger)
    void recalcWidth();

    // Row under (x,y), in this menu's own coordinates, or -1 if outside it
    int findItem(int x, int y) const;
    // Updates the hover highlight to 'item' (row offset within the visible window)
    void drawCurrentSelection(int item);

    // Move the highlighted row, scrolling the view as needed at either end
    void moveUp();
    void moveDown();
    void movePgUp();
    void movePgDown();
    void moveToFirst();
    void moveToLast();
    // Scrolls so the current _selectedItem is in view
    void moveToSelected();
    // Scrolls the view by 'distance' rows, clamped to the entry list's ends
    void scrollUp(int distance = 1);
    void scrollDown(int distance = 1);
    // Closes the menu and sends _cmd (or Cmd::ItemSelected) for the highlighted item
    void sendSelection();

  private:
    // The items shown, as (label, tag) pairs
    VariantList _entries;
    // Per-entry enabled state, parallel to _entries
    std::vector<bool> _enabled;

    int _rowHeight{0};
    // Index of the first entry currently scrolled into view, and how many are shown
    int _firstEntry{0}, _numEntries{0};
    // Highlighted row, relative to the visible window, and the selected
    // entry's index into _entries (-1 for none)
    int _selectedOffset{0}, _selectedItem{-1};
    // Whether there are more entries than fit, so scroll arrows are shown
    bool _showScroll{false};
    // True while a mouse-down is held on a scroll arrow
    bool _isScrolling{false};
    // Scroll arrow colors, greyed out at either end of the list
    ColorId _scrollUpColor{kColor}, _scrollDnColor{kColor};

    // Command sent on selection; Cmd::ItemSelected is used if this is 0
    GuiCmd::Code _cmd{GuiCmd::None};
    // The parent widget's id (see setID()), passed through to sent commands
    int _id{-1};

    // Screen position the menu was opened at (see show())
    uInt32 _xorig{0}, _yorig{0};
    // Minimum width imposed from outside (see setMaxWidth()); the menu still
    // grows to fit its widest entry
    int _maxWidth{0};

    // Font-derived dimensions (see setArrows())
    int _textOfs{0};
    int _arrowSize{0};

  private:
    // Following constructors and assignment operators not supported
    ContextMenu() = delete;
    ContextMenu(const ContextMenu&) = delete;
    ContextMenu(ContextMenu&&) = delete;
    ContextMenu& operator=(const ContextMenu&) = delete;
    ContextMenu& operator=(ContextMenu&&) = delete;
};

#endif  // CONTEXT_MENU_HXX
