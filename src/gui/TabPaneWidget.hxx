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

#ifndef TAB_PANE_WIDGET_HXX
#define TAB_PANE_WIDGET_HXX

#include "Widget.hxx"

namespace GUI {
  class Font;
  class BoxLayout;
}  // namespace GUI

/**
  The content of a single tab: a transparent container that a tab's controls are
  parented to, plus a layout builder.

  Whenever the tab is (re)sized, the pane rebuilds a vertical BoxLayout (inset by
  the standard borders) from the current font, runs the builder to fill it with
  the tab's rows, and lays it out.  So a dialog describes a tab's layout once,
  right where its controls are created, and writes no resize code at all — the
  TabWidget drives it (see TabWidget::layoutActivePane).

  @author  Stephen Anthony
*/
class TabPaneWidget : public Widget
{
  public:
    // Fills the (vertical, already-margined) box with the tab's rows
    using LayoutBuilder = std::function<void(GUI::BoxLayout&)>;
    // Optional: position cross-referencing/overlay widgets after the box has
    // been laid out (for the rare right-column-aligned-to-a-row cases)
    using PostLayout = std::function<void()>;

    TabPaneWidget(GuiObject* boss, const GUI::Font& font);
    ~TabPaneWidget() override = default;

    void setLayout(const LayoutBuilder& builder, const PostLayout& post = {})
      { myBuilder = builder; myPostLayout = post; }

    void setArea(int x, int y, int w, int h) override;
    Widget* findWidget(int x, int y) override;

  private:
    LayoutBuilder myBuilder;
    PostLayout myPostLayout;

  private:
    // Following constructors and assignment operators not supported
    TabPaneWidget() = delete;
    TabPaneWidget(const TabPaneWidget&) = delete;
    TabPaneWidget(TabPaneWidget&&) = delete;
    TabPaneWidget& operator=(const TabPaneWidget&) = delete;
    TabPaneWidget& operator=(TabPaneWidget&&) = delete;
};

#endif  // TAB_PANE_WIDGET_HXX
