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

#ifndef POPUP_WIDGET_HXX
#define POPUP_WIDGET_HXX

class GUIObject;
class ContextMenu;

#include "bspf.hxx"
#include "Variant.hxx"
#include "Command.hxx"
#include "EditableWidget.hxx"

/**
 * Popup or dropdown widget which, when clicked, "pop up" a list of items and
 * lets the user pick on of them.
 *
 * Implementation wise, when the user selects an item, then a kPopUpItemSelectedCmd
 * is broadcast, with data being equal to the tag value of the selected entry.
 */
class PopUpWidget : public EditableWidget
{
  public:
    /**
      Size me from my own items: the value box is as wide as the widest of them
      and I am as tall as my font — so a dialog with a list of things to offer
      states only the list, and adding a longer entry simply widens me.

      ⚠ Only for a list that is FIXED for my lifetime.  A pop-up refilled in
      loadConfig() (the per-ROM controller and bankswitch lists) must be given an
      explicit width, or the dialog would change size as the user browses: how
      wide an entry it is prepared to show is then the DIALOG's decision.  Such a
      dialog can still say it in items rather than pixels — see calcWidth().
    */
    PopUpWidget(GuiObject* boss, const GUI::Font& font,
                const VariantList& items, GuiCmd::Code cmd = GuiCmd::None);

    /**
      Take this value-box width, but size your own height.  For the pop-up whose
      LIST is refilled at runtime (the per-ROM controller and bankswitch lists):
      it cannot size its box to items it does not have yet, and would resize under
      the user if it tried, so the DIALOG says how wide an entry it will show.
    */
    PopUpWidget(GuiObject* boss, const GUI::Font& font, int w,
                const VariantList& items, GuiCmd::Code cmd = GuiCmd::None);

    ~PopUpWidget() override = default;

    // Also sets the id on the underlying drop-down menu
    void setID(uInt32 id) override;

    // Set the total widget width (value box + drop-down arrow); also
    // resizes the drop-down menu so it tracks the value box
    void setWidth(int w) override;

    // Nudged 1px down from the base to clear the frame this widget draws around itself
    int getTop() const override { return _y + 1; }
    int getBottom() const override { return _y + 1 + getHeight(); }

    /** Add the given items to the widget. */
    void addItems(const VariantList& items);

    /** Various selection methods passed directly to the underlying menu
        See ContextMenu.hxx for more information. */
    void setSelected(const Variant& tag,
                     const Variant& def = EmptyVariant());
    void setSelectedIndex(int idx, bool changed = false);
    void setSelectedMax(bool changed = false);
    void clearSelection();

    // Query/set the selection by index, name, or tag; forwarded to the menu
    int getSelected() const;
    const string& getSelectedName() const;
    void setSelectedName(string_view name);
    const Variant& getSelectedTag() const;

    // Unlike EditableWidget's, always wants focus -- Tab must reach even a
    // pop-up whose value box isn't otherwise editable
    bool wantsFocus() const override { return true; }
    /**
      The drop-down arrow I draw at my right-hand end.  Odd, so it has a
      single-pixel tip, and derived from the font so it grows with the text
      rather than stepping between two hand-drawn bitmaps.  At the default
      9x18 font this is 9, the width the old small arrow bitmap had.
    */
    static int arrowWidth(const GUI::Font& font) {
      return font.getMaxCharWidth() | 1;
    }

    static int dropDownWidth(const GUI::Font& font) {
      // The arrow's box, plus the margin the value box keeps from it
      return arrowWidth(font) * 2 + 3;
    }

    /**
      The value-box width (the 'w' the full constructor takes, i.e. excluding the
      label and the drop-down arrow) needed to show the widest of these items.
      The self-sizing constructor above uses this; a dialog only needs it for a
      pop-up it must size itself — to give one a little more room than its items
      strictly need, or to size a *dynamic* list from a fixed set of specimen
      entries rather than a pixel literal.
    */
    static int calcWidth(const GUI::Font& font, const VariantList& items);

    // Opens the drop-down menu (or, on its editable text, positions the caret);
    // wheel/UI-navigation events move the selection up/down without opening it
    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseWheel(int x, int y, int direction) override;
    bool handleEvent(Event::Type e) override;

    /**
      My value box: the part between my left edge and my drop-down arrow.  I size
      it to my own items, but a COLUMN of pop-ups wants them all the same width —
      which none of us can know alone — so GUI::alignPopUps() equalizes them.
    */
    int boxWidth() const { return _w - dropDownWidth(_font); }
    void setBoxWidth(int w);

    // Re-picks the arrow bitmap/dimensions, restores the framed height, and
    // (if auto-sized) re-derives the box width, all for the live font
    void refreshFont() override;

  protected:
    // Redraws with the menu's current selection after it reports one was made
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;
    int caretOfs() const override { return _editScrollOffset; }

    // Picks the arrow bitmap dimensions and text inset from the current font
    void setArrow();
    // Draws the frame, value box, drop-down arrow, and selected entry's text
    void drawWidget(bool hilite) override;

    // No-ops: a pop-up's value box is always in edit mode (see EditTextWidget
    // for the same pattern); the drop-down menu has its own accept/cancel
    void endEditMode() override;
    void abortEditMode() override;

    Common::Rect getEditRect() const override;

  private:
    // The drop-down menu itself
    unique_ptr<ContextMenu> myMenu;

    // Did I size myself from my own items (the c'tor that states no width)?
    // If so my width is mine to re-derive on a font change; otherwise it is
    // the dialog's, and its layout() re-applies it
    bool myAutoWidth{false};
    // Vertical offset of the drop-down arrow, centered within the box
    int myArrowsY{0};

    // Highlighted in drawWidget() when true; set via setSelectedIndex/setSelectedMax
    bool   _changed{false};

    // Horizontal inset between the frame and the text (see EditableWidget::textInset)
    int _textOfs{0};
    // Drop-down arrow dimensions, font-derived (see setArrow())
    int _arrowWidth{0};
    int _arrowHeight{0};
    int _arrowThickness{0};

  private:
    // Following constructors and assignment operators not supported
    PopUpWidget() = delete;
    PopUpWidget(const PopUpWidget&) = delete;
    PopUpWidget(PopUpWidget&&) = delete;
    PopUpWidget& operator=(const PopUpWidget&) = delete;
    PopUpWidget& operator=(PopUpWidget&&) = delete;
};

#endif  // POPUP_WIDGET_HXX
