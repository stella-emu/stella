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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: TabWidget.cxx,v 1.19 2005-12-21 01:50:16 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "GuiUtils.hxx"
#include "bspf.hxx"
#include "GuiObject.hxx"
#include "Widget.hxx"
#include "Dialog.hxx"
#include "TabWidget.hxx"

enum {
  kTabLeftOffset = 4,
  kTabSpacing = 2,
  kTabPadding = 3
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TabWidget::TabWidget(GuiObject* boss, int x, int y, int w, int h)
  : Widget(boss, x, y, w, h),
    CommandSender(boss),
    _tabWidth(40),
    _activeTab(-1),
    _firstTime(true)
{
  _flags = WIDGET_ENABLED | WIDGET_CLEARBG;
  _type = kTabWidget;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TabWidget::~TabWidget()
{
  for (unsigned int i = 0; i < _tabs.size(); ++i)
  {
    delete _tabs[i].firstWidget;
    _tabs[i].firstWidget = 0;
    // _tabs[i].parentWidget is deleted elsewhere
  }
  _tabs.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TabWidget::getChildY() const
{
  return getAbsY() + kTabHeight;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TabWidget::addTab(const string& title)
{
  // Add a new tab page
  Tab newTab;
  newTab.title = title;
  newTab.firstWidget = NULL;
  newTab.parentWidget = NULL;

  _tabs.push_back(newTab);

  int numTabs = _tabs.size();

  // Determine the new tab width
  int newWidth = _font->getStringWidth(title) + 2 * kTabPadding;
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
  assert(0 <= tabID && tabID < (int)_tabs.size());

  if (_activeTab != -1)
  {
    // Exchange the widget lists, and switch to the new tab
    _tabs[_activeTab].firstWidget = _firstWidget;
  }

  _activeTab = tabID;
  _firstWidget  = _tabs[tabID].firstWidget;

  // Let parent know about the tab change
  if(show)
    sendCommand(kTabChangedCmd, _activeTab, -1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::updateActiveTab()
{
  if(_activeTab < 0)
    return;

  if(_tabs[_activeTab].parentWidget)
    _tabs[_activeTab].parentWidget->loadConfig();

  setDirty(); draw();

  // Redraw focused areas
  _boss->redrawFocus();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::activateTabs()
{
  for(unsigned int i = 0; i <_tabs.size(); ++i)
    sendCommand(kTabChangedCmd, i-1, -1);
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
      tabID = (int)_tabs.size() - 1;
  }
  else if(direction == 1)  // Go to the next tab, wrap around at end
  {
    tabID++;
    if(tabID == (int)_tabs.size())
      tabID = 0;
  }

  // Finally, select the active tab
  setActiveTab(tabID, true);
  updateActiveTab();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::setParentWidget(int tabID, Widget* parent)
{
  assert(0 <= tabID && tabID < (int)_tabs.size());
  _tabs[tabID].parentWidget = parent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
  assert(y < kTabHeight);

  // Determine which tab was clicked
  int tabID = -1;
  x -= kTabLeftOffset;
  if (x >= 0 && x % (_tabWidth + kTabSpacing) < _tabWidth)
  {
    tabID = x / (_tabWidth + kTabSpacing);
    if (tabID >= (int)_tabs.size())
      tabID = -1;
  }

  // If a tab was clicked, switch to that pane
  if (tabID >= 0 && tabID != _activeTab)
  {
    setActiveTab(tabID, true);
    updateActiveTab();
  }
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
void TabWidget::box(int x, int y, int width, int height,
                    OverlayColor colorA, OverlayColor colorB, bool omitBottom)
{
  FrameBuffer& fb = _boss->instance()->frameBuffer();

  fb.hLine(x + 1, y, x + width - 2, colorA);
  fb.hLine(x, y + 1, x + width - 1, colorA);
  fb.vLine(x, y + 1, y + height - (omitBottom ? 1 : 2), colorA);
  fb.vLine(x + 1, y, y + height - (omitBottom ? 2 : 1), colorA);

  if (!omitBottom)
  {
    fb.hLine(x + 1, y + height - 2, x + width - 1, colorB);
    fb.hLine(x + 1, y + height - 1, x + width - 2, colorB);
  }
  fb.vLine(x + width - 1, y + 1, y + height - (omitBottom ? 1 : 2), colorB);
  fb.vLine(x + width - 2, y + 1, y + height - (omitBottom ? 2 : 1), colorB);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabWidget::drawWidget(bool hilite)
{
  // The tab widget is strange in that it acts as both a widget (obviously)
  // and a dialog (it contains other widgets).  Because of the latter,
  // it must assume responsibility for refreshing all its children.
  Widget::setDirtyInChain(_tabs[_activeTab].firstWidget);

  FrameBuffer& fb = instance()->frameBuffer();

  const int left1  = _x + 1;
  const int right1 = _x + kTabLeftOffset + _activeTab * (_tabWidth + kTabSpacing);
  const int left2  = right1 + _tabWidth;
  const int right2 = _x + _w - 2;
	
  // Draw horizontal line
  fb.hLine(left1, _y + kTabHeight - 2, right1, kShadowColor);
  fb.hLine(left2, _y + kTabHeight - 2, right2, kShadowColor);

  // Iterate over all tabs and draw them
  int i, x = _x + kTabLeftOffset;
  for (i = 0; i < (int)_tabs.size(); ++i)
  {
    OverlayColor color = (i == _activeTab) ? kColor : kShadowColor;
    int yOffset = (i == _activeTab) ? 0 : 2; 
    box(x, _y + yOffset, _tabWidth, kTabHeight - yOffset, color, color, (i == _activeTab));
    fb.drawString(_font, _tabs[i].title, x + kTabPadding,
                  _y + yOffset / 2 + (kTabHeight - kLineHeight - 1),
                  _tabWidth - 2 * kTabPadding, kTextColor, kTextAlignCenter);
    x += _tabWidth + kTabSpacing;
  }

  // Draw a frame around the widget area (belows the tabs)
  fb.hLine(left1, _y + kTabHeight - 1, right1, kColor);
  fb.hLine(left2, _y + kTabHeight - 1, right2, kColor);
  fb.hLine(_x+1, _y + _h - 2, _x + _w - 2, kShadowColor);
  fb.hLine(_x+1, _y + _h - 1, _x + _w - 2, kColor);
  fb.vLine(_x + _w - 2, _y + kTabHeight - 1, _y + _h - 2, kColor);
  fb.vLine(_x + _w - 1, _y + kTabHeight - 1, _y + _h - 2, kShadowColor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget *TabWidget::findWidget(int x, int y)
{
  if (y < kTabHeight)
  {
    // Click was in the tab area
    return this;
  }
  else
  {
    // Iterate over all child widgets and find the one which was clicked
    return Widget::findWidgetInChain(_firstWidget, x, y - kTabHeight);
  }
}
