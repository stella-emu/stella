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

class TabPaneWidget;

/**
  A container managing a row of tabs and the single active tab's
  content; hidden tabs keep their widgets off to the side until
  reselected.

  @author  Stephen Anthony and Thomas Jentzsch
*/
class TabWidget : public Widget, public CommandSender
{
  public:
    // Sentinel tabWidth values for addTab(): NO_WIDTH shares the common
    // width, AUTO_WIDTH sizes the tab to its title
    static constexpr int NO_WIDTH = 0;
    static constexpr int AUTO_WIDTH = -1;

    // Sent (via CommandSender) when the active tab changes
    struct Cmd {
      static constexpr GuiCmd::Code
        TabChanged = GuiCmd::of("TabWidget.TabChanged");
    };

  public:
    // ID defaults to 0; a dialog with more than one tab widget must setID()
    // each explicitly to tell them apart
    TabWidget(GuiObject* boss, const GUI::Font& font);
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
    // Enables/disables a tab; a disabled tab cannot be selected, but keeps its widgets
    void enableTab(int tabID, bool enable = true);
    // Announces every tab's Cmd::TabChanged in turn, e.g. so a dialog's
    // handleCommand() sees each tab once while it is being built
    void activateTabs();
    // Switches to the previous/next enabled tab, wrapping around
    void cycleTab(int direction);

    // Recompute the tab-bar geometry (height and per-tab widths) from the
    // current font and the widget's current width.  Called internally by
    // addTab(); a font-reactive/resizeable dialog also calls it from layout()
    // after (re)sizing the tab widget, so the tab bar reflows like everything
    // else (addTab() otherwise bakes these from the width at construction).
    void updateTabSizes();
// setActiveTab moves the active tab's widgets into _children. This means
// Widgets added afterwards will be added to the active tab.
    // Registers 'parent' as this tab's content widget, for content parented
    // directly to us (see setPaneWidget() for the TabPaneWidget case)
    void setParentWidget(int tabID, Widget* parent);
    // Set a tab's content pane.  Called by TabPaneWidget's constructor, so a
    // pane is always registered with its tab (see also setParentWidget, for
    // composite content parented directly to us)
    void setPaneWidget(int tabID, TabPaneWidget* pane);
    // A tab's content widget, or null if the dialog registered none for it
    Widget* parentWidget(int tabID);

    // Current tab-bar dimensions/state
    int getTabWidth() const  { return _tabWidth;  }
    int getTabHeight() const { return _tabHeight; }
    int getActiveTab() const { return _activeTab; }
    int getTabCount() const  { return static_cast<int>(_tabs.size()); }

    // The frame inset between us and our active tab's content.  Public because
    // a dialog deriving its own minimum from getMaxContentHeight() has to add
    // back the insets that height was measured inside of
    static constexpr int CONTENT_BORDER = 2;

    // The height the tallest tab's content ASKS to be; tabs whose content simply
    // fills the area (e.g. a list or prompt) report 0 and are ignored.  Answered
    // from each content's own layout tree, so it holds before anything has been
    // laid out -- which is what lets a dialog derive its minimum at build time
    int getMaxContentHeight() const;

    // The size we need for our largest tab's content, plus the tab bar and the
    // content frame — so a fixed-size dialog sizes itself from the layout rather
    // than counting the rows and columns of its biggest tab.  Only content panes
    // report a size; a self-contained composite (a list, the event mapper) fills
    // whatever it is given and so does not constrain us.  Nor does the tab BAR:
    // a dialog too narrow for its tab titles would squeeze them, so keep a width
    // floor if the tabs are many or their titles long
    Common::Size naturalSize() const override;

    // Only the active tab's widgets live in _children (see setActiveTab), so
    // the base walk a caller does over _children reaches those; each hidden
    // tab keeps its widgets in its own list instead, invisible to that walk,
    // so refresh those here too -- otherwise a live font change leaves a
    // hidden tab's content sized/positioned from the OLD font until it is
    // next activated and reflowed, while it already draws with the new one
    void refreshFont() override;

    // Activates the initial tab on first call, then loads the active tab's config
    void loadConfig() override;

    // Switches to the tab clicked in the bar
    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;
    // Doesn't highlight on hover, unlike a plain Widget
    void handleMouseEntered() override {}
    void handleMouseLeft() override {}

    // Left/Right (or PgUp/PgDown) cycles tabs
    bool handleEvent(Event::Type event) override;

    // Children sit below the tab bar, not at our own origin
    int getChildY() const override;

  protected:
    // Redraws the tab bar; as a container, also re-dirties the active tab's children
    void drawWidget(bool hilite) override;
    // A click in the tab bar hits us; below it, routes to the active tab's children
    Widget* findWidget(int x, int y) override;
    // Forwards a tab content's command, unmodified, to our own target
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:
    struct Tab {
      string title;
      // The tab's widgets; while the tab is active they live in _children
      // instead (see setActiveTab), so this list is then empty
      WidgetList children;
      // The tab's content, which the container lays out and sizes itself from.
      // Null only between addTab() and the setParentWidget/setPaneWidget that
      // registers it
      Widget* parentWidget{nullptr};
      // True when the content is a TabPaneWidget, which parents the tab's
      // controls to itself.  Such a tab can be laid out while hidden; content
      // parented directly to us cannot (see layoutTabs)
      bool isPane{false};
      bool enabled{true};
      int tabWidth{0};        // resolved width (0 = share the common _tabWidth)
      bool autoWidth{false};  // width tracks the (font-dependent) title width

      explicit Tab(string_view t, int tw = NO_WIDTH, bool aw = false)
        : title{t}, tabWidth{tw}, autoWidth{aw} { }
    };
    using TabList = vector<Tab>;

    // The tab bar's height for the current font (what _tabHeight caches)
    int tabBarHeight() const { return _font.getLineHeight() + 4; }

    // One entry per tab, in display order
    TabList _tabs;
    // Shared width for NO_WIDTH tabs (see updateTabSizes())
    int     _tabWidth{40};
    // Tab-bar height for the current font (see tabBarHeight())
    int     _tabHeight{1};
    // Index of the currently active tab, or -1 if none yet
    int     _activeTab{-1};
    // True until loadConfig() has activated the initial tab once
    bool    _firstTime{true};

    // Tab-bar layout constants, in pixels
    enum: uInt8 {
      kTabLeftOffset = 0,
      kTabSpacing = 1,
      kTabPadding = 4,
      kTabMinWidth = 40
    };

  private:
    // Loads the active tab's content config; called after a tab switch
    void updateActiveTab();
    // Lay one tab's content widget out to fill the area below the tab bar.  The
    // tab widget is a container, so it owns this: a dialog just sizes the tab
    // widget and the content follows, needing no per-tab resize code
    void layoutContent(int tabID);
    // Lay out the content of the active tab, plus that of every (hidden) pane
    void layoutTabs();

  private:
    // Following constructors and assignment operators not supported
    TabWidget() = delete;
    TabWidget(const TabWidget&) = delete;
    TabWidget(TabWidget&&) = delete;
    TabWidget& operator=(const TabWidget&) = delete;
    TabWidget& operator=(TabWidget&&) = delete;
};

#endif  // TAB_WIDGET_HXX
