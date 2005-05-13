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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: TabWidget.cxx,v 1.2 2005-05-13 18:28:06 stephena Exp $
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
#include "TabWidget.hxx"

enum {
  kTabHeight = 16,

  kTabLeftOffset = 4,
  kTabSpacing = 2,
  kTabPadding = 3
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TabWidget::TabWidget(GuiObject *boss, int x, int y, int w, int h)
  : Widget(boss, x, y, w, h),
    CommandSender(boss)
{
  _flags = WIDGET_ENABLED;
  _type = kTabWidget;
  _activeTab = -1;

  _tabWidth = 40;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TabWidget::~TabWidget()
{
  for (unsigned int i = 0; i < _tabs.size(); ++i)
  {
    delete _tabs[i].firstWidget;
    _tabs[i].firstWidget = 0;
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
  FrameBuffer& fb = instance()->frameBuffer();

  // Add a new tab page
  Tab newTab;
  newTab.title = title;
  newTab.firstWidget = NULL;

  _tabs.push_back(newTab);

  int numTabs = _tabs.size();

  // Determine the new tab width
  int newWidth = fb.font().getStringWidth(title) + 2 * kTabPadding;
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
void TabWidget::setActiveTab(int tabID)
{
  assert(0 <= tabID && tabID < (int)_tabs.size());
  if (_activeTab != tabID)
  {
    // Exchange the widget lists, and switch to the new tab
    if (_activeTab != -1)
      _tabs[_activeTab].firstWidget = _firstWidget;

    _activeTab = tabID;
    _firstWidget = _tabs[tabID].firstWidget;
    _boss->draw();

    _boss->instance()->frameBuffer().refresh();
  }
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
  if (tabID >= 0)
    setActiveTab(tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TabWidget::handleKeyDown(int ascii, int keycode, int modifiers)
{
  // TODO: maybe there should be a way to switch between tabs
  // using the keyboard? E.g. Alt-Shift-Left/Right-Arrow or something
  // like that.
  return Widget::handleKeyDown(ascii, keycode, modifiers);
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
    fb.font().drawString(_tabs[i].title, x + kTabPadding, _y + yOffset / 2 + (kTabHeight - kLineHeight - 1), _tabWidth - 2 * kTabPadding, kTextColor, kTextAlignCenter);
    x += _tabWidth + kTabSpacing;
  }

  // Draw more horizontal lines
  fb.hLine(left1, _y + kTabHeight - 1, right1, kColor);
  fb.hLine(left2, _y + kTabHeight - 1, right2, kColor);
  fb.hLine(_x+1, _y + _h - 2, _x + _w - 2, kShadowColor);
  fb.hLine(_x+1, _y + _h - 1, _x + _w - 2, kColor);
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
