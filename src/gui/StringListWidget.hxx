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

#ifndef STRING_LIST_WIDGET_HXX
#define STRING_LIST_WIDGET_HXX

#include "ListWidget.hxx"

/** StringListWidget */
class StringListWidget : public ListWidget
{
  public:
    StringListWidget(GuiObject* boss, const GUI::Font& font,
                     bool hilite = true,
                     bool useScrollbar = true);
    ~StringListWidget() override = default;

    void setList(const StringList& list);
    // Unlike EditableWidget's, always wants focus, since even a non-editable list
    // takes keyboard/joystick navigation
    bool wantsFocus() const override { return true; }

    // The hovered row's text, in full, when it doesn't fit its row (see getToolTipIndex)
    string getToolTip(const Common::Point& pos) const override;
    bool changedToolTip(const Common::Point& oldPos, const Common::Point& newPos) const override;

    // My text inset is font-derived, so it has to follow a live font change
    void refreshFont() override;

  protected:
    // The width a row's text is drawn in (0 at a placeholder width); measure text
    // against this, not _w, or the renderer ellipsizes what does not fit
    int textWidth() const { return std::max(_w - 2 * _textOfs, 0); }

    // display depends on _hasFocus so we have to redraw when focus changes
    void receivedFocusWidget() override { setDirty(); }
    void lostFocusWidget() override { setDirty(); }

    bool hasToolTip() const override { return true; }
    // Row index under 'pos' (in screen coordinates), or -1 if none
    int getToolTipIndex(const Common::Point& pos) const;

    void drawWidget(bool hilite) override;
    // Draws an optional per-row icon before the text and returns its width (0 for a
    // plain list); FileListWidget overrides it for folder/file icons
    virtual int drawIcon(int i, int x, int y, ColorId color) { return 0; }
    Common::Rect getEditRect() const override;

  protected:
    // Whether the selected row is drawn highlighted (some lists just frame it)
    bool _hilite{false};
    // Horizontal inset between the frame and row text (font-derived, see textInset())
    int  _textOfs{0};

  private:
    // Following constructors and assignment operators not supported
    StringListWidget() = delete;
    StringListWidget(const StringListWidget&) = delete;
    StringListWidget(StringListWidget&&) = delete;
    StringListWidget& operator=(const StringListWidget&) = delete;
    StringListWidget& operator=(StringListWidget&&) = delete;
};

#endif  // STRING_LIST_WIDGET_HXX
