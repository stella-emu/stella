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

#include "Dialog.hxx"
#include "Layout.hxx"
#include "TabPaneWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TabPaneWidget::TabPaneWidget(GuiObject* boss, const GUI::Font& font)
  : Widget(boss, font, 0, 0, 1, 1)
{
  // A transparent container: it draws nothing of its own, only its children
  _flags = Widget::FLAG_ENABLED;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabPaneWidget::setArea(int x, int y, int w, int h)
{
  Widget::setArea(x, y, w, h);

  if(!myBuilder)
    return;

  // Rebuild the tab's layout from the current font (throwaway, so it stays
  // font-correct) and position the pane's children within it.  Children use
  // pane-local coordinates, so the layout runs from (0, 0).  Spacing is 0 so the
  // builder controls each gap (rows rarely want a uniform gap)
  GUI::BoxLayout col(GUI::BoxLayout::Dir::Vertical,
                     0, dialog().hBorder(), dialog().vBorder());
  myBuilder(col);
  col.doLayout(0, 0, _w, _h);

  // Overlay/cross-referencing widgets are placed once the box is resolved
  if(myPostLayout)
    myPostLayout();

  // The pane is (re)laid out when it becomes the active tab, but a container
  // swapped into the active tab is not otherwise re-dirtied — so its children
  // would not repaint until each was individually touched.  Mark our subtree
  // for redraw (dirtying the children propagates the chain up to us)
  Widget::setDirtyInList(_children);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget* TabPaneWidget::findWidget(int x, int y)
{
  // Route the point to the child under it (a plain Widget would return itself)
  return Widget::findWidgetInList(_children, x, y);
}
