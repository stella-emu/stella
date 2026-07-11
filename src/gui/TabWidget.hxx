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

#ifndef TAB_WIDGET_HXX
#define TAB_WIDGET_HXX

#include "bspf.hxx"

#include "Command.hxx"
#include "Widget.hxx"

class TabWidget : public Widget, public CommandSender
{
  public:
    static constexpr int NO_WIDTH = 0;
    static constexpr int AUTO_WIDTH = -1;

    enum {
      kTabChangedCmd = 'TBCH'
    };

  public:
    TabWidget(GuiObject* boss, const GUI::Font& font, int x, int y, int w, int h);
    ~TabWidget() override = default;

// use Dialog::releaseFocus() when changing to another tab

// Problem: how to add items to a tab?
// First off, widget should allow non-dialog bosses, (i.e. also other widgets)
// Could add a common base class for Widgets and Dialogs.
// Then you add tabs using the following method, which returns a unique ID
    int addTab(string_view title, int tabWidth = NO_WIDTH);
// Maybe we need to remove tabs again? Hm
    //void removeTab(int tabID);
// Setting the active tab:
    void setActiveTab(int tabID, bool show = false);
    void enableTab(int tabID, bool enable = true);
    void activateTabs();
    void cycleTab(int direction);

    // Recompute the tab-bar geometry (height and per-tab widths) from the
    // current font and the widget's current width.  Called internally by
    // addTab(); a font-reactive/resizeable dialog also calls it from layout()
    // after (re)sizing the tab widget, so the tab bar reflows like everything
    // else (addTab() otherwise bakes these from the width at construction).
    void updateTabSizes();
// setActiveTab moves the active tab's widgets into _children. This means
// Widgets added afterwards will be added to the active tab.
    void setParentWidget(int tabID, Widget* parent);
    Widget* parentWidget(int tabID);

    int getTabWidth() const  { return _tabWidth;  }
    int getTabHeight() const { return _tabHeight; }
    int getActiveTab() const { return _activeTab; }

    // The largest recorded content height over all tabs; tabs whose content
    // simply fills the area (e.g. a list or prompt) report 0 and are ignored
    int getMaxContentHeight() const;

    void loadConfig() override;

    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseEntered() override {}
    void handleMouseLeft() override {}

    bool handleEvent(Event::Type event) override;

    int getChildY() const override;

  protected:
    void drawWidget(bool hilite) override;
    Widget* findWidget(int x, int y) override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    struct Tab {
      string title;
      // The tab's widgets; while the tab is active they live in _children
      // instead (see setActiveTab), so this list is then empty
      WidgetList children;
      Widget* parentWidget{nullptr};
      // TRANSITIONAL: true only when a real content widget was set via
      // setParentWidget (vs the lazily-created 0-size dummy), so the container
      // sizes only such content (see layoutActivePane).  Once every tab has a
      // real content widget/pane (no more dummies), this flag and its checks —
      // and the dummy itself — can be removed.  TODO: revisit after conversion
      bool sizeContent{false};
      bool enabled{true};
      int tabWidth{0};        // resolved width (0 = share the common _tabWidth)
      bool autoWidth{false};  // width tracks the (font-dependent) title width

      explicit Tab(string_view t, int tw = NO_WIDTH, bool aw = false)
        : title{t}, tabWidth{tw}, autoWidth{aw} { }
    };
    using TabList = vector<Tab>;

    TabList _tabs;
    int     _tabWidth{40};
    int     _tabHeight{1};
    int     _activeTab{-1};
    bool    _firstTime{true};

    enum: uInt8 {
      kTabLeftOffset = 0,
      kTabSpacing = 1,
      kTabPadding = 4,
      kTabMinWidth = 40
    };

  private:
    void updateActiveTab();
    // Lay the active tab's content widget out to fill the area below the tab
    // bar.  The tab widget is a container, so it owns this: a dialog just sizes
    // the tab widget and the content follows, needing no per-tab resize code
    void layoutActivePane();

  private:
    // Following constructors and assignment operators not supported
    TabWidget() = delete;
    TabWidget(const TabWidget&) = delete;
    TabWidget(TabWidget&&) = delete;
    TabWidget& operator=(const TabWidget&) = delete;
    TabWidget& operator=(TabWidget&&) = delete;
};

#endif  // TAB_WIDGET_HXX
