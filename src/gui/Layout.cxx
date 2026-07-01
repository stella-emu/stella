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

namespace GUI {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WidgetLayout::doLayout(int x, int y, int w, int h)
{
  if(myWidget == nullptr)
    return;

  // setArea() forwards to the virtual setWidth()/setHeight() so composite
  // widgets that override them re-flow their content (e.g. ListWidget
  // recomputes its visible rows, NavigationWidget its path field), while
  // widgets that override setArea() itself (e.g. RomImageWidget, which rescales
  // its image) react to the whole geometry change
  myWidget->setArea(x, y, std::max(w, 0), std::max(h, 0));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BoxLayout& BoxLayout::add(unique_ptr<Layout> child, Policy policy, int value,
                          int maxMain)
{
  myItems.push_back(Item{std::move(child), policy, value, maxMain});
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BoxLayout::doLayout(int x, int y, int w, int h)
{
  const int n = static_cast<int>(myItems.size());
  if(n == 0)
    return;

  // Inset by the margins, then work within the remaining content rectangle
  const int cx = x + myMarginH, cy = y + myMarginV;
  const int cw = w - 2 * myMarginH, ch = h - 2 * myMarginV;

  const bool horiz = myDir == Dir::Horizontal;
  const int mainLen = (horiz ? cw : ch) - mySpacing * (n - 1);
  const int crossLen = horiz ? ch : cw;

  // First pass: fixed and percentage sizes (and tally stretch weights)
  IntArray ext(n, 0);
  int used = 0, totalWeight = 0;
  for(int i = 0; i < n; ++i)
  {
    const Item& it = myItems[i];
    switch(it.policy)
    {
      case Policy::Fixed:
        ext[i] = it.value;
        break;
      case Policy::Percent:
        ext[i] = it.value * std::max(mainLen, 0) / 100;
        break;
      case Policy::Stretch:
        totalWeight += it.value;
        continue;  // sized in the second pass
    }
    if(it.maxMain > 0)
      ext[i] = std::min(ext[i], it.maxMain);
    used += ext[i];
  }

  // Second pass: stretch items share the remaining length by weight
  const int remaining = std::max(mainLen - used, 0);
  int distributed = 0, lastStretch = -1;
  for(int i = 0; i < n; ++i)
  {
    if(myItems[i].policy != Policy::Stretch)
      continue;
    int e = totalWeight > 0 ? remaining * myItems[i].value / totalWeight : 0;
    if(myItems[i].maxMain > 0)
      e = std::min(e, myItems[i].maxMain);
    ext[i] = e;
    distributed += e;
    lastStretch = i;
  }
  // Hand any rounding leftover to the last stretch item
  if(lastStretch >= 0)
    ext[lastStretch] += remaining - distributed;

  // Position the children sequentially along the main axis
  int pos = horiz ? cx : cy;
  const int crossStart = horiz ? cy : cx;
  for(int i = 0; i < n; ++i)
  {
    if(horiz)
      myItems[i].layout->doLayout(pos, crossStart, ext[i], crossLen);
    else
      myItems[i].layout->doLayout(crossStart, pos, crossLen, ext[i]);
    pos += ext[i] + mySpacing;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size BoxLayout::minSize() const
{
  const bool horiz = myDir == Dir::Horizontal;
  int mainMin = 0, crossMin = 0;
  for(const auto& it: myItems)
  {
    const Common::Size cs = it.layout->minSize();
    int childMain = static_cast<int>(horiz ? cs.w : cs.h);
    const int childCross = static_cast<int>(horiz ? cs.h : cs.w);
    // A fixed cell can never be smaller than its fixed size
    if(it.policy == Policy::Fixed)
      childMain = std::max(childMain, it.value);
    mainMin += childMain;
    crossMin = std::max(crossMin, childCross);
  }
  const int n = static_cast<int>(myItems.size());
  if(n > 0)
    mainMin += mySpacing * (n - 1);

  // Main axis gets that axis' margin; cross axis gets the other
  const int wMargin = 2 * myMarginH, hMargin = 2 * myMarginV;
  return horiz
    ? Common::Size(static_cast<uInt32>(mainMin + wMargin),
                   static_cast<uInt32>(crossMin + hMargin))
    : Common::Size(static_cast<uInt32>(crossMin + wMargin),
                   static_cast<uInt32>(mainMin + hMargin));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<Layout> vCentered(Widget* widget, int h, int minW)
{
  auto col = std::make_unique<BoxLayout>(BoxLayout::Dir::Vertical);
  col->addStretch(std::make_unique<WidgetLayout>(nullptr));
  col->addFixed(widgetItem(widget, minW, h), h);
  col->addStretch(std::make_unique<WidgetLayout>(nullptr));
  return col;
}

}  // namespace GUI
