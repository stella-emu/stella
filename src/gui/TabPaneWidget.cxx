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
unique_ptr<GUI::BoxLayout> TabPaneWidget::buildLayout() const
{
  // The tree is throwaway, rebuilt from the current font on every use, so it
  // stays font-correct.  Spacing is 0 so the builder controls each gap (rows
  // rarely want a uniform one); the margins are the dialog's standard borders
  auto col = std::make_unique<GUI::BoxLayout>(
    GUI::BoxLayout::Dir::Vertical, 0, dialog().hBorder(), dialog().vBorder());

  if(myBuilder)
    myBuilder(*col);

  return col;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size TabPaneWidget::naturalSize() const
{
  // What the rows add up to, straight from the layout.  A fixed-size dialog can
  // therefore size itself to its tallest tab (see TabWidget::naturalSize) rather
  // than counting rows and gaps by hand, which no one keeps correct
  if(!myBuilder)
    return Widget::naturalSize();

  return buildLayout()->naturalSize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabPaneWidget::setArea(int x, int y, int w, int h)
{
  Widget::setArea(x, y, w, h);

  // Position the pane's children within the rebuilt layout.  They are parented
  // to the pane, so their coordinates are pane-local and the layout runs from
  // (0, 0)
  if(myBuilder)
    buildLayout()->doLayout(0, 0, _w, _h);

  // Overlay/cross-referencing widgets are placed once the box is resolved.  A
  // pane whose content a box cannot express (a dense, cross-referenced grid) may
  // supply only this
  if(myPostLayout)
    myPostLayout();

  // Everything moved, so repaint the whole pane (see drawWidget)
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TabPaneWidget::drawWidget(bool hilite)
{
  // The pane has nothing of its own to draw.  But drawing is incremental — a
  // widget is only repainted when it is itself dirty — so as a container it has
  // to pass a repaint of itself on to its children, which are not otherwise
  // re-dirtied.  Without this they would vanish whenever something forces a full
  // redraw of the dialog underneath them (e.g. closing a popup menu) and only
  // reappear as each was individually touched.  TabWidget does the same for the
  // widgets it holds directly
  Widget::setDirtyInList(_children);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget* TabPaneWidget::findWidget(int x, int y)
{
  // Route the point to the child under it (a plain Widget would return itself)
  return Widget::findWidgetInList(_children, x, y);
}
