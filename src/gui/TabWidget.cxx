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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
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
#include "OSystem.hxx"
#include "Widget.hxx"
#include "TabWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TabWidget::TabWidget(GuiObject* boss, const GUI::Font& font,
                     int x, int y, int w, int h)
  : Widget(boss, font, x, y, w, h),
    CommandSender(boss),
    _tabWidth(40),
    _activeTab(-1),
    _firstTime(true)
{
  _id = 0;  // For dialogs with multiple tab widgets, they should specifically
            // call ::setID to differentiate among them
  _flags = WIDGET_ENABLED | WIDGET_CLEARBG;
  _bgcolor = kDlgColor;
  _bgcolorhi = kDlgColor;
  _textcolor = kTextColor;
  _textcolorhi = kTextColor;

  _tabHeight = font.getLineHeight() + 4;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TabWidget::~TabWidget()
{
  for(auto& tab: _tabs)
  {
    delete tab.firstWidget;
    tab.firstWidget = nullptr;
    // _tabs[i].parentWidget is deleted elsewhere
  }
  _tabs.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TabWidget::getChildY() const
{
  return getAbsY() + _tabHeight;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TabWidget::addTab(const string& title)
{
  // Add a new tab page
  _tabs.push_back(Tab(title));
  int numTabs = int(_tabs.size());

  // Determine the new tab width
  int newWidth = _font.getStringWidth(title) + 2 * kTabPadding;
  if (_tabWidth < newWidth)
    _tabWidth = newWidth;

  int maxWidth = (_w - kTabLeftOffset) / numTabs - kTabLeftOffset;
  if (_tabWidth > maxWidth)
    _tabWidth = maxWidth;

  // Activate the new tab
  setActiveTab(numTabs - 1);

  return _activeTab;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::setActiveTab(int tabID, bool show)
{
  assert(0 <= tabID && tabID < int(_tabs.size()));

  if (_activeTab != -1)
  {
    // Exchange the widget lists, and switch to the new tab
    _tabs[_activeTab].firstWidget = _firstWidget;
  }

  _activeTab = tabID;
  _firstWidget  = _tabs[tabID].firstWidget;

  // Let parent know about the tab change
  if(show)
    sendCommand(TabWidget::kTabChangedCmd, _activeTab, _id);
}

#if 0 // FIXME
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::disableTab(int tabID)
{
  assert(0 <= tabID && tabID < int(_tabs.size()));

  _tabs[tabID].enabled = false;
  // TODO - also disable all widgets belonging to this tab
}
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::updateActiveTab()
{
  if(_activeTab < 0)
    return;

  if(_tabs[_activeTab].parentWidget)
    _tabs[_activeTab].parentWidget->loadConfig();

  setDirty();

  // Redraw focused areas
  _boss->redrawFocus();
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
    tabID--;
    if(tabID == -1)
      tabID = int(_tabs.size()) - 1;
  }
  else if(direction == 1)  // Go to the next tab, wrap around at end
  {
    tabID++;
    if(tabID == int(_tabs.size()))
      tabID = 0;
  }

  // Finally, select the active tab
  setActiveTab(tabID, true);
  updateActiveTab();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::setParentWidget(int tabID, Widget* parent)
{
  assert(0 <= tabID && tabID < int(_tabs.size()));
  _tabs[tabID].parentWidget = parent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  assert(y < _tabHeight);

  // Determine which tab was clicked
  int tabID = -1;
  x -= kTabLeftOffset;
  if (x >= 0 && x % (_tabWidth + kTabSpacing) < _tabWidth)
  {
    tabID = x / (_tabWidth + kTabSpacing);
    if (tabID >= int(_tabs.size()))
      tabID = -1;
  }

  // If a tab was clicked, switch to that pane
  if (tabID >= 0)
  {
    setActiveTab(tabID, true);
    updateActiveTab();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::handleMouseEntered()
{
  setFlags(WIDGET_HILITED);
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::handleMouseLeft()
{
  clearFlags(WIDGET_HILITED);
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    default:
      sendCommand(cmd, data, _id);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TabWidget::handleEvent(Event::Type event)
{
  bool handled = false;

  switch (event)
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
  Widget::setDirtyInChain(_tabs[_activeTab].firstWidget);

  FBSurface& s = dialog().surface();

  // Iterate over all tabs and draw them
  int i, x = _x + kTabLeftOffset;
  for (i = 0; i < int(_tabs.size()); ++i)
  {
    uInt32 fontcolor = _tabs[i].enabled ? kTextColor : kColor;
    int yOffset = (i == _activeTab) ? 0 : 1;
    s.fillRect(x, _y + 1, _tabWidth, _tabHeight - 1, (i == _activeTab)
               ? kDlgColor : kBGColorHi); // ? kWidColor : kDlgColor
    s.drawString(_font, _tabs[i].title, x + kTabPadding + yOffset,
                 _y + yOffset + (_tabHeight - _fontHeight - 1),
                 _tabWidth - 2 * kTabPadding, fontcolor, TextAlign::Center);
    if(i == _activeTab)
    {
      s.hLine(x, _y, x + _tabWidth - 1, kWidColor);
      s.vLine(x + _tabWidth, _y + 1, _y + _tabHeight - 1, kBGColorLo);
    }
    else
      s.hLine(x, _y + _tabHeight, x + _tabWidth, kWidColor);

    x += _tabWidth + kTabSpacing;
  }

  // fill empty right space
  s.hLine(x - kTabSpacing + 1, _y + _tabHeight, _x + _w - 1, kWidColor);
  s.hLine(_x, _y + _h - 1, _x + _w - 1, kBGColorLo);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget* TabWidget::findWidget(int x, int y)
{
  if (y < _tabHeight)
  {
    // Click was in the tab area
    return this;
  }
  else
  {
    // Iterate over all child widgets and find the one which was clicked
    return Widget::findWidgetInChain(_firstWidget, x, y - _tabHeight);
  }
}
