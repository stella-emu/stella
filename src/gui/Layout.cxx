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

  // Anchored items keep their own size and are only moved to the cell origin
  if(!myFill)
  {
    myWidget->setPos(x, y);
    return;
  }

  // setArea() forwards to the virtual setWidth()/setHeight() so composite
  // widgets that override them re-flow their content (e.g. ListWidget
  // recomputes its visible rows, NavigationWidget its path field), while
  // widgets that override setArea() itself (e.g. RomImageWidget, which rescales
  // its image) react to the whole geometry change
  myWidget->setArea(x, y, std::max(w, 0), std::max(h, 0));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BoxLayout& BoxLayout::add(unique_ptr<Layout> child, SizePolicy policy, int value,
                          int maxMain, int minMain)
{
  myItems.push_back(Item{std::move(child), policy, value, maxMain, minMain});
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
      case SizePolicy::Fixed:
        ext[i] = it.value;
        break;
      case SizePolicy::Percent:
        ext[i] = it.value * std::max(mainLen, 0) / 100;
        break;
      case SizePolicy::Stretch:
        totalWeight += it.value;
        // A stretch cell is never smaller than its base size, so that is spoken
        // for before any leftover is shared out
        used += it.minMain;
        continue;  // sized in the second pass
    }
    if(it.maxMain > 0)
      ext[i] = std::min(ext[i], it.maxMain);
    used += ext[i];
  }

  // Second pass: stretch items take their base size, then share what is left
  // over by weight
  const int remaining = std::max(mainLen - used, 0);
  int distributed = 0, lastStretch = -1;
  for(int i = 0; i < n; ++i)
  {
    if(myItems[i].policy != SizePolicy::Stretch)
      continue;
    int e = myItems[i].minMain
          + (totalWeight > 0 ? remaining * myItems[i].value / totalWeight : 0);
    if(myItems[i].maxMain > 0)
      e = std::min(e, myItems[i].maxMain);
    ext[i] = e;
    distributed += e - myItems[i].minMain;
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
    // A fixed cell can never be smaller than its fixed size — unless the
    // dialog declared a compression floor (minMain), promising to recompute
    // the fixed value down to that floor as the available space shrinks
    if(it.policy == SizePolicy::Fixed)
      childMain = std::max(childMain, it.minMain > 0 ? it.minMain : it.value);
    else if(it.policy == SizePolicy::Stretch)
      childMain = std::max(childMain, it.minMain);
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<Layout> hCentered(Widget* widget, int w, int minH)
{
  auto row = std::make_unique<BoxLayout>(BoxLayout::Dir::Horizontal);
  row->addStretch(std::make_unique<WidgetLayout>(nullptr));
  row->addFixed(widgetItem(widget, w, minH), w);
  row->addStretch(std::make_unique<WidgetLayout>(nullptr));
  return row;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<Layout> indentedItem(Widget* widget, int indent, int minW)
{
  auto row = std::make_unique<BoxLayout>(BoxLayout::Dir::Horizontal);
  row->addSpace(indent);
  row->addStretch(anchoredItem(widget, minW));
  return row;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<Layout> labelColumn(Widget* label, Widget* control)
{
  // A label draws its text at its top edge, whereas a control frames its own
  // text and insets it; drop the label by that inset so the two texts, rather
  // than the two boxes, sit on the same line
  auto column = std::make_unique<BoxLayout>(BoxLayout::Dir::Vertical);
  column->addSpace(std::max(control->textOffsetY() - label->textOffsetY(), 0));
  column->addFixed(anchoredItem(label), label->getHeight());
  column->addStretch(std::make_unique<WidgetLayout>(nullptr));
  return column;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<Layout> labeledRow(Widget* label, Widget* control,
                              int labelW, int indent)
{
  auto row = std::make_unique<BoxLayout>(BoxLayout::Dir::Horizontal);
  if(indent > 0)
    row->addSpace(indent);
  row->addFixed(labelColumn(label, control),
                labelW > 0 ? labelW : label->getWidth());
  row->addStretch(anchoredItem(control));
  return row;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GridLayout::GridLayout(int cols, int rows, int hSpacing, int vSpacing,
                       int marginH, int marginV)
  : myHSpacing{hSpacing}, myVSpacing{vSpacing},
    myMarginH{marginH}, myMarginV{marginV}
{
  myColumns.resize(std::max(cols, 0));
  myRows.resize(std::max(rows, 0));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GridLayout& GridLayout::column(int idx, SizePolicy policy, int value, int maxSize)
{
  assert(idx >= 0 && std::cmp_less(idx, myColumns.size()));
  myColumns[idx] = Track{policy, value, maxSize};
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GridLayout& GridLayout::row(int idx, SizePolicy policy, int value, int maxSize)
{
  assert(idx >= 0 && std::cmp_less(idx, myRows.size()));
  myRows[idx] = Track{policy, value, maxSize};
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GridLayout& GridLayout::place(int col, int row, unique_ptr<Layout> child,
                              int colspan, int rowspan)
{
  assert(col >= 0 && row >= 0 && colspan >= 1 && rowspan >= 1);
  assert(col + colspan <= static_cast<int>(myColumns.size()));
  assert(row + rowspan <= static_cast<int>(myRows.size()));
  myCells.push_back(Cell{std::move(child), col, row, colspan, rowspan});
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GridLayout::resolveTracks(const vector<Track>& tracks, int avail,
                               IntArray& ext)
{
  const int n = static_cast<int>(tracks.size());
  ext.assign(n, 0);

  // First pass: fixed and percentage tracks (and tally stretch weights)
  int used = 0, totalWeight = 0;
  for(int i = 0; i < n; ++i)
  {
    switch(tracks[i].policy)
    {
      case SizePolicy::Fixed:
        ext[i] = tracks[i].value;
        break;
      case SizePolicy::Percent:
        ext[i] = tracks[i].value * std::max(avail, 0) / 100;
        break;
      case SizePolicy::Stretch:
        totalWeight += tracks[i].value;
        continue;  // sized in the second pass
    }
    if(tracks[i].maxSize > 0)
      ext[i] = std::min(ext[i], tracks[i].maxSize);
    used += ext[i];
  }

  // Second pass: stretch tracks share the remaining length by weight
  const int remaining = std::max(avail - used, 0);
  int distributed = 0, lastStretch = -1;
  for(int i = 0; i < n; ++i)
  {
    if(tracks[i].policy != SizePolicy::Stretch)
      continue;
    int e = totalWeight > 0 ? remaining * tracks[i].value / totalWeight : 0;
    if(tracks[i].maxSize > 0)
      e = std::min(e, tracks[i].maxSize);
    ext[i] = e;
    distributed += e;
    lastStretch = i;
  }
  // Hand any rounding leftover to the last stretch track
  if(lastStretch >= 0)
    ext[lastStretch] += remaining - distributed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GridLayout::growSpan(IntArray& mins, int start, int span, int need,
                          int spacing)
{
  int have = spacing * (span - 1);
  for(int i = 0; i < span; ++i)
    have += mins[start + i];

  const int deficit = need - have;
  if(deficit <= 0)
    return;

  // Distribute the shortfall across the spanned tracks, remainder to the last
  const int per = deficit / span;
  for(int i = 0; i < span; ++i)
    mins[start + i] += per;
  mins[start + span - 1] += deficit - per * span;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GridLayout::doLayout(int x, int y, int w, int h)
{
  if(myCells.empty())
    return;

  const int cols = static_cast<int>(myColumns.size());
  const int rows = static_cast<int>(myRows.size());

  // Inset by the margins; the space the tracks share on each axis excludes the
  // inter-track spacing
  const int cx = x + myMarginH, cy = y + myMarginV;
  const int availW = (w - 2 * myMarginH) - myHSpacing * std::max(cols - 1, 0);
  const int availH = (h - 2 * myMarginV) - myVSpacing * std::max(rows - 1, 0);

  IntArray colExt, rowExt;
  resolveTracks(myColumns, availW, colExt);
  resolveTracks(myRows, availH, rowExt);

  // Start offset of each column and row
  IntArray colPos(cols, 0), rowPos(rows, 0);
  int px = cx;
  for(int c = 0; c < cols; ++c) { colPos[c] = px; px += colExt[c] + myHSpacing; }
  int py = cy;
  for(int r = 0; r < rows; ++r) { rowPos[r] = py; py += rowExt[r] + myVSpacing; }

  // Position each cell; a spanned cell also covers the spacing it subsumes
  for(const auto& cell: myCells)
  {
    int cw = myHSpacing * (cell.colspan - 1);
    for(int i = 0; i < cell.colspan; ++i)
      cw += colExt[cell.col + i];
    int chh = myVSpacing * (cell.rowspan - 1);
    for(int i = 0; i < cell.rowspan; ++i)
      chh += rowExt[cell.row + i];
    cell.layout->doLayout(colPos[cell.col], rowPos[cell.row], cw, chh);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size GridLayout::minSize() const
{
  const int cols = static_cast<int>(myColumns.size());
  const int rows = static_cast<int>(myRows.size());
  IntArray colMin(cols, 0), rowMin(rows, 0);

  // A track can never be smaller than its own fixed size
  for(int c = 0; c < cols; ++c)
    if(myColumns[c].policy == SizePolicy::Fixed)
      colMin[c] = myColumns[c].value;
  for(int r = 0; r < rows; ++r)
    if(myRows[r].policy == SizePolicy::Fixed)
      rowMin[r] = myRows[r].value;

  // Non-spanning cells constrain their own column/row directly
  for(const auto& cell: myCells)
  {
    const Common::Size cs = cell.layout->minSize();
    if(cell.colspan == 1)
      colMin[cell.col] = std::max(colMin[cell.col], static_cast<int>(cs.w));
    if(cell.rowspan == 1)
      rowMin[cell.row] = std::max(rowMin[cell.row], static_cast<int>(cs.h));
  }
  // Spanning cells grow their tracks only when the combined min is too small
  for(const auto& cell: myCells)
  {
    const Common::Size cs = cell.layout->minSize();
    if(cell.colspan > 1)
      growSpan(colMin, cell.col, cell.colspan, static_cast<int>(cs.w), myHSpacing);
    if(cell.rowspan > 1)
      growSpan(rowMin, cell.row, cell.rowspan, static_cast<int>(cs.h), myVSpacing);
  }

  int mw = 2 * myMarginH, mh = 2 * myMarginV;
  for(int c = 0; c < cols; ++c) mw += colMin[c];
  for(int r = 0; r < rows; ++r) mh += rowMin[r];
  if(cols > 0) mw += myHSpacing * (cols - 1);
  if(rows > 0) mh += myVSpacing * (rows - 1);

  return Common::Size(static_cast<uInt32>(mw), static_cast<uInt32>(mh));
}

}  // namespace GUI
