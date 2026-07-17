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

#include "Widget.hxx"
#include "Layout.hxx"

#include "ControllerWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ControllerWidget::setArea(int x, int y, int w, int h)
{
  Widget::setArea(x, y, w, h);
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ControllerWidget::createHeader()
{
  myHeaderName = new StaticTextWidget(_boss, _font, 0, 0, controller().name());
  myHeaderPort = new StaticTextWidget(_boss, _font, 0, 0,
                                      isLeftPort() ? "(Left)" : "(Right)");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<GUI::Layout> ControllerWidget::layoutCross(
    CheckboxWidget* up, CheckboxWidget* down,
    CheckboxWidget* left, CheckboxWidget* right)
{
  using GUI::GridLayout;
  using GUI::anchoredItem;

  const int VGAP = _font.getFontHeight() / 4;

  // The four directions on a 3x3 grid; sized only by the (empty) checkboxes, so
  // the cross is the same tight size for every controller that uses it
  auto cross = std::make_unique<GridLayout>(3, 3, VGAP, VGAP);
  for(int c = 0; c < 3; ++c)
    cross->columnAuto(c);
  for(int r = 0; r < 3; ++r)
    cross->rowAuto(r);

  cross->place(1, 0, anchoredItem(up));
  cross->place(0, 1, anchoredItem(left));
  cross->place(2, 1, anchoredItem(right));
  cross->place(1, 2, anchoredItem(down));
  return cross;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ControllerWidget::reflow()
{
  using GUI::BoxLayout;
  using GUI::centeredItem;

  // The name over its "(Left)"/"(Right)" port, then the controller's own content
  BoxLayout col(BoxLayout::Dir::Vertical, _font.getFontHeight() / 4);
  if(myHeaderName != nullptr)
  {
    col.addAuto(centeredItem(myHeaderName));
    col.addAuto(centeredItem(myHeaderPort));
  }
  layoutContent(col);

  // The content sizes the block: no wider than it needs, and at least eight
  // lines tall so the leaf controllers and the register rows beside them line
  // up -- but taller if the content (a QuadTari's two embedded controllers,
  // AtariVox's page list) needs it
  const Common::Size natural = col.naturalSize();
  _w = static_cast<int>(natural.w);
  _h = std::max(8 * _font.getLineHeight(), static_cast<int>(natural.h));
  col.doLayout(_x, _y, _w, _h);
}
