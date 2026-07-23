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
    PopUpWidget(GuiObject* boss, const GUI::Font& font,
                int w, int h, const VariantList& items, int cmd = 0);

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
                const VariantList& items, int cmd = 0);

    /**
      Take this value-box width, but size your own height.  For the pop-up whose
      LIST is refilled at runtime (the per-ROM controller and bankswitch lists):
      it cannot size its box to items it does not have yet, and would resize under
      the user if it tried, so the DIALOG says how wide an entry it will show.
    */
    PopUpWidget(GuiObject* boss, const GUI::Font& font, int w,
                const VariantList& items, int cmd = 0);

    ~PopUpWidget() override = default;

    void setID(uInt32 id) override;

    // Set the total widget width (value box + drop-down arrow); also
    // resizes the drop-down menu so it tracks the value box
    void setWidth(int w) override;

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

    int getSelected() const;
    const string& getSelectedName() const;
    void setSelectedName(string_view name);
    const Variant& getSelectedTag() const;

    bool wantsFocus() const override { return true; }
    static int dropDownWidth(const GUI::Font& font) {
      return font.isLarge() ? (13 * 2 + 7) : (9 * 2 + 3);
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

    void refreshFontMetrics() override;

  protected:
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;
    int caretOfs() const override { return _editScrollOffset; }

    void setArrow();
    void drawWidget(bool hilite) override;

    void endEditMode() override;
    void abortEditMode() override;

    Common::Rect getEditRect() const override;

  private:
    unique_ptr<ContextMenu> myMenu;
    int myArrowsY{0};

    bool   _changed{false};

    int _textOfs{0};
    int _arrowWidth{0};
    int _arrowHeight{0};
    const uInt32* _arrowImg{nullptr};

  private:
    // Following constructors and assignment operators not supported
    PopUpWidget() = delete;
    PopUpWidget(const PopUpWidget&) = delete;
    PopUpWidget(PopUpWidget&&) = delete;
    PopUpWidget& operator=(const PopUpWidget&) = delete;
    PopUpWidget& operator=(PopUpWidget&&) = delete;
};

#endif  // POPUP_WIDGET_HXX
