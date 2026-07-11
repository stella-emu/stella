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

#include "bspf.hxx"
#include "Dialog.hxx"
#include "FBSurface.hxx"
#include "Font.hxx"
#include "GuiObject.hxx"
#include "Widget.hxx"
#include "TabWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TabWidget::TabWidget(GuiObject* boss, const GUI::Font& font,
                     int x, int y, int w, int h)
  : Widget(boss, font, x, y, w, h),
    CommandSender(boss),
    _tabHeight{font.getLineHeight() + 4}
{
  _id = 0;  // For dialogs with multiple tab widgets, they should specifically
            // call ::setID to differentiate among them
  _flags = Widget::FLAG_ENABLED | Widget::FLAG_CLEARBG;
  _bgcolor = kDlgColor;
  _bgcolorhi = kDlgColor;
  _textcolor = kTextColor;
  _textcolorhi = kTextColor;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TabWidget::getChildY() const
{
  return getAbsY() + _tabHeight;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TabWidget::addTab(string_view title, int tabWidth)
{
  // Add a new tab page.  AUTO_WIDTH tabs size to their title (recomputed from
  // the font in updateTabSizes()); NO_WIDTH tabs share the common _tabWidth.
  const bool autoWidth = tabWidth == AUTO_WIDTH;
  _tabs.emplace_back(title, autoWidth ? NO_WIDTH : tabWidth, autoWidth);

  updateTabSizes();

  // Activate the new tab
  setActiveTab(static_cast<int>(_tabs.size()) - 1);

  return _activeTab;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::updateTabSizes()
{
  // Tab-bar height and the AUTO-width tabs' widths follow the current font
  _tabHeight = _font.getLineHeight() + 4;

  // A common width is shared by the NO_WIDTH tabs (its floor is kTabMinWidth);
  // AUTO/fixed tabs contribute their own width to the fixed total
  int fixedWidth = 0, fixedTabs = 0, sharedWidth = kTabMinWidth;
  for(auto& tab: _tabs)
  {
    if(tab.autoWidth)
      tab.tabWidth = _font.getStringWidth(tab.title) + 2 * kTabPadding;

    if(tab.tabWidth != NO_WIDTH)
    {
      fixedWidth += tab.tabWidth;
      ++fixedTabs;
    }
    else
      sharedWidth = std::max(sharedWidth,
                             _font.getStringWidth(tab.title) + 2 * kTabPadding);
  }

  // Clamp the shared width so all the NO_WIDTH tabs still fit the current width
  _tabWidth = sharedWidth;
  const int varTabs = static_cast<int>(_tabs.size()) - fixedTabs;
  if(varTabs > 0)
  {
    const int maxWidth =
        (_w - kTabLeftOffset - fixedWidth) / varTabs - kTabLeftOffset;
    _tabWidth = std::min(_tabWidth, maxWidth);
  }

  // The content area (below the tab bar) has now changed, so re-lay the active
  // tab's content out — the container owns this, so dialogs need no such code
  layoutActivePane();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TabWidget::getMaxContentHeight() const
{
  int maxHeight = 0;

  for(const auto& tab: _tabs)
    if(tab.parentWidget != nullptr)
      maxHeight = std::max(maxHeight, tab.parentWidget->getContentHeight());

  return maxHeight;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::setActiveTab(int tabID, bool show)
{
  assert(0 <= tabID && std::cmp_less(tabID, _tabs.size()));

  if(_activeTab != -1)
  {
    // Exchange the widget lists, and switch to the new tab
    _tabs[_activeTab].children = std::move(_children);
  }

  if(_activeTab != tabID)
    setDirty();

  _activeTab = tabID;
  _children = std::move(_tabs[tabID].children);

  // As a container, lay the newly-active content out to the current size
  layoutActivePane();

  // Let parent know about the tab change
  if(show)
    sendCommand(TabWidget::kTabChangedCmd, _activeTab, _id);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::layoutActivePane()
{
  if(_activeTab < 0)
    return;

  // Only lay out tabs whose content was set via setParentWidget (composite or
  // pane).  Unconverted loose tabs keep a lazily-created 0-size dummy that the
  // dialog lays around; sizing it would make it a large invisible click-blocker
  if(!_tabs[_activeTab].sizeContent)
    return;
  Widget* content = _tabs[_activeTab].parentWidget;

  // Size the content to fill the area below the tab bar, inset by a small frame
  // border.  Skip while the tab widget is still at its placeholder size (a
  // negative content area would drive content widgets into degenerate states);
  // the owning dialog's layout() sizes us, then re-drives this via
  // updateTabSizes()
  constexpr int border = 2;
  const int w = _w - 2 * border, h = _h - _tabHeight - 2 * border;
  if(w <= 0 || h <= 0)
    return;

  content->setArea(border, border, w, h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::enableTab(int tabID, bool enable)
{
  assert(0 <= tabID && std::cmp_less(tabID, _tabs.size()));

  _tabs[tabID].enabled = enable;
  // Note: We do not have to disable the widgets because the tab is disabled
  //   and therefore cannot be selected.
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::updateActiveTab()
{
  if(_activeTab < 0)
    return;

  if(_tabs[_activeTab].parentWidget)
    _tabs[_activeTab].parentWidget->loadConfig();

  // Redraw focused areas
  _boss->redrawFocus(); // TJ: Does nothing!
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::activateTabs()
{
  for(uInt32 i = 0; i <_tabs.size(); ++i)
    sendCommand(TabWidget::kTabChangedCmd, i-1, _id);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::cycleTab(int direction)
{
  int tabID = _activeTab;

  // Don't do anything if no tabs have been defined
  if(tabID == -1)
    return;

  if(direction == -1)  // Go to the previous tab, wrap around at beginning
  {
    do {
      tabID--;
      if(tabID == -1)
        tabID = static_cast<int>(_tabs.size()) - 1;
    } while(!_tabs[tabID].enabled);
  }
  else if(direction == 1)  // Go to the next tab, wrap around at end
  {
    do {
      tabID++;
      if(std::cmp_equal(tabID, _tabs.size()))
        tabID = 0;
    } while(!_tabs[tabID].enabled);
  }

  // Finally, select the active tab
  setActiveTab(tabID, true);
  updateActiveTab();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::setParentWidget(int tabID, Widget* parent)
{
  assert(0 <= tabID && std::cmp_less(tabID, _tabs.size()));
  _tabs[tabID].parentWidget = parent;
  // This is real content, so the container lays it out (see layoutActivePane)
  _tabs[tabID].sizeContent = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget* TabWidget::parentWidget(int tabID)
{
  assert(0 <= tabID && std::cmp_less(tabID, _tabs.size()));

  if(!_tabs[tabID].parentWidget)
    // Create a 0-size dummy if none exists.  It is deliberately NOT set via
    // setParentWidget, so sizeContent stays false and layoutActivePane leaves
    // it alone (sizing it would make it a large invisible click-blocker)
    _tabs[tabID].parentWidget = new Widget(_boss, _font, 0, 0, 0, 0);

  return _tabs[tabID].parentWidget;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  assert(y < _tabHeight);

  // Determine which tab was clicked
  int tabID = -1;
  x -= kTabLeftOffset;

  for(int i = 0; std::cmp_less(i, _tabs.size()); ++i)
  {
    const int tabWidth = _tabs[i].tabWidth ? _tabs[i].tabWidth : _tabWidth;
    if(x >= 0 && x < tabWidth)
    {
      tabID = i;
      break;
    }
    x -= (tabWidth + kTabSpacing);
  }

  // If a tab was clicked, switch to that pane
  if(tabID >= 0 && _tabs[tabID].enabled)
  {
    setActiveTab(tabID, true);
    updateActiveTab();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  // Command is not inspected; simply forward it to the caller
  sendCommand(cmd, data, _id);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TabWidget::handleEvent(Event::Type event)
{
  bool handled = false;

  switch(event)
  {
    case Event::UIRight:
    case Event::UIPgDown:
      cycleTab(1);
      handled = true;
      break;
    case Event::UILeft:
    case Event::UIPgUp:
      cycleTab(-1);
      handled = true;
      break;
    default:
      break;
  }
  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::loadConfig()
{
  if(_firstTime)
  {
    setActiveTab(_activeTab, true);
    _firstTime = false;
  }

  updateActiveTab();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::drawWidget(bool hilite)
{
  // The tab widget is strange in that it acts as both a widget (obviously)
  // and a dialog (it contains other widgets).  Because of the latter,
  // it must assume responsibility for refreshing all its children.

  if(isDirty())
  {
    FBSurface& s = dialog().surface();

    // Iterate over all tabs and draw them
    int x = _x + kTabLeftOffset;
    for(int i = 0; std::cmp_less(i, _tabs.size()); ++i)
    {
      const int tabWidth = _tabs[i].tabWidth ? _tabs[i].tabWidth : _tabWidth;
      const ColorId fontcolor = _tabs[i].enabled ? kTextColor : kColor;
      const int yOffset = (i == _activeTab) ? 0 : 1;
      s.fillRect(x, _y + 1, tabWidth, _tabHeight - 1,
                 (i == _activeTab)
                 ? kDlgColor : kBGColorHi); // ? kWidColor : kDlgColor
      s.drawString(_font, _tabs[i].title, x + kTabPadding + yOffset,
                   _y + yOffset + (_tabHeight - _lineHeight - 1),
                   tabWidth - 2 * kTabPadding, fontcolor, TextAlign::Center);
      if(i == _activeTab)
      {
        s.hLine(x, _y, x + tabWidth - 1, kWidColor);
        s.vLine(x + tabWidth, _y + 1, _y + _tabHeight - 1, kBGColorLo);
      }
      else
        s.hLine(x, _y + _tabHeight, x + tabWidth, kWidColor);

      x += tabWidth + kTabSpacing;
    }

    // fill empty right space
    s.hLine(x - kTabSpacing + 1, _y + _tabHeight, _x + _w - 1, kWidColor);
    s.hLine(_x, _y + _h - 1, _x + _w - 1, kBGColorLo);

    clearDirty();
    // Make all child widgets of currently active tab dirty
    Widget::setDirtyInList(_children);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget* TabWidget::findWidget(int x, int y)
{
  if(y < _tabHeight)
  {
    // Click was in the tab area
    return this;
  }
  else
  {
    // Iterate over all child widgets and find the one which was clicked
    return Widget::findWidgetInList(_children, x, y - _tabHeight);
  }
}
