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
#include "PopUpWidget.hxx"
#include "Layout.hxx"

namespace GUI {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size WidgetLayout::naturalSize() const
{
  if(myWidget == nullptr)
    return {};

  // An axis the widget FILLS has no size of its own: the cell decides it.  The
  // widget's extent along that axis is merely what the last layout gave it, and
  // reporting that would feed back — a field would drive the width of the column
  // that is supposed to be sizing the field.  What it CAN say is how small it is
  // prepared to be, and that is what it wants when nothing else is pushing: so a
  // filled axis reports the minimum the dialog declared for it, if any.  This is
  // how a dialog whose fields simply stretch still has a width of its own
  const Common::Size natural = myWidget->naturalSize();

  return Common::Size(myHAlign == HAlign::Fill ? std::max(myMinW, 0) : natural.w,
                      myVAlign == VAlign::Fill ? std::max(myMinH, 0) : natural.h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int WidgetLayout::firstTextY() const
{
  return myWidget != nullptr ? myWidget->firstTextY() : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WidgetLayout::doLayout(int x, int y, int w, int h)
{
  if(myWidget == nullptr)
    return;

  w = std::max(w, 0);
  h = std::max(h, 0);

  // An axis the widget does not fill leaves it at the size it wants to be, and
  // positions that within the cell.  A baseline item has already been dropped
  // onto its row's baseline by the enclosing box, so here it places like Top
  const Common::Size natural = myWidget->naturalSize();
  const int aw = myHAlign == HAlign::Fill ? w : static_cast<int>(natural.w);
  const int ah = myVAlign == VAlign::Fill ? h : static_cast<int>(natural.h);
  int ax = x, ay = y;

  switch(myHAlign)
  {
    case HAlign::Center:  ax = x + (w - aw) / 2;  break;
    case HAlign::Right:   ax = x + w - aw;        break;
    case HAlign::Fill:
    case HAlign::Left:                            break;
  }
  switch(myVAlign)
  {
    case VAlign::Center:  ay = y + (h - ah) / 2;  break;
    case VAlign::Bottom:  ay = y + h - ah;        break;
    case VAlign::Fill:
    case VAlign::Top:
    case VAlign::Baseline:                        break;
  }

  // setArea() forwards to the virtual setWidth()/setHeight() so composite
  // widgets that override them re-flow their content (e.g. ListWidget
  // recomputes its visible rows, NavigationWidget its path field), while
  // widgets that override setArea() itself (e.g. RomImageWidget, which rescales
  // its image) react to the whole geometry change
  myWidget->setArea(ax, ay, aw, ah);
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

  // First pass: the sizes that do not depend on what is left over (and tally the
  // stretch weights)
  IntArray ext(n, 0);
  int used = 0, totalWeight = 0;
  for(int i = 0; i < n; ++i)
  {
    const Item& it = myItems[i];
    switch(it.policy)
    {
      case SizePolicy::Auto:
      {
        const Common::Size natural = it.layout->naturalSize();
        ext[i] = static_cast<int>(horiz ? natural.w : natural.h);
        break;
      }
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

  // In a row, the items asking to sit on the baseline share the lowest first
  // line of text among them, so each is dropped by the difference: a label
  // (whose text is centered in its own small height) lands on the first row of
  // the data grid beside it, which centering could not express
  int baseline = 0;
  if(horiz)
    for(const auto& it: myItems)
      if(it.layout->onBaseline())
        baseline = std::max(baseline, it.layout->firstTextY());

  // Position the children sequentially along the main axis
  int pos = horiz ? cx : cy;
  const int crossStart = horiz ? cy : cx;
  for(int i = 0; i < n; ++i)
  {
    if(horiz)
    {
      const int drop = myItems[i].layout->onBaseline()
        ? baseline - myItems[i].layout->firstTextY() : 0;
      myItems[i].layout->doLayout(pos, crossStart + drop, ext[i],
                                  crossLen - drop);
    }
    else
      myItems[i].layout->doLayout(crossStart, pos, crossLen, ext[i]);
    pos += ext[i] + mySpacing;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size BoxLayout::naturalSize() const
{
  const bool horiz = myDir == Dir::Horizontal;
  int mainNat = 0, crossNat = 0;
  for(const auto& it: myItems)
  {
    const Common::Size cs = it.layout->naturalSize();
    // A cell sized in pixels wants exactly those; one that stretches wants what
    // its content wants, but never less than the base size it was promised
    int childMain = static_cast<int>(horiz ? cs.w : cs.h);
    if(it.policy == SizePolicy::Fixed)
      childMain = it.value;
    else if(it.policy == SizePolicy::Stretch)
      childMain = std::max(childMain, it.minMain);
    mainNat += childMain;
    crossNat = std::max(crossNat, static_cast<int>(horiz ? cs.h : cs.w));
  }
  const int n = static_cast<int>(myItems.size());
  if(n > 0)
    mainNat += mySpacing * (n - 1);

  // Main axis gets that axis' margin; cross axis gets the other
  const int wMargin = 2 * myMarginH, hMargin = 2 * myMarginV;
  return horiz
    ? Common::Size(static_cast<uInt32>(mainNat + wMargin),
                   static_cast<uInt32>(crossNat + hMargin))
    : Common::Size(static_cast<uInt32>(crossNat + wMargin),
                   static_cast<uInt32>(mainNat + hMargin));
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
    else if(it.policy == SizePolicy::Auto)
    {
      // A cell sized by its content cannot be squeezed below it (a row of
      // controls does not get shorter than the controls).  Note this is the
      // only place the two queries meet: content that CAN be squeezed — a list,
      // an image — stretches instead, and keeps a small minSize of its own
      const Common::Size natural = it.layout->naturalSize();
      childMain = std::max(childMain,
                           static_cast<int>(horiz ? natural.w : natural.h));
    }
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
void alignLabels(std::span<const LabeledControl> controls)
{
  // The column has to hold the longest label, and an indented control's label
  // starts that much further in
  int width = 0;
  for(const auto& c: controls)
    width = std::max(width, c.control->naturalLabelWidth()
                            + c.control->font().getMaxCharWidth() + c.indent);

  // Each control's column ends at the same place, so the tracks beside them
  // start there too
  for(const auto& c: controls)
    c.control->setLabelWidth(width - c.indent);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void alignLabels(std::initializer_list<LabeledControl> controls)
{
  alignLabels(std::span{controls.begin(), controls.size()});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {
  // Each button has already sized itself to its own label, so the group's width
  // is simply the widest of them.  Re-running this is harmless: a button that has
  // been widened to the group reports that width, which is still the maximum
  template<typename Buttons>
  void alignButtonGroup(const Buttons& buttons, int minWidth)
  {
    int width = minWidth;
    for(const auto* b: buttons)
      if(b != nullptr)
        width = std::max(width, static_cast<int>(b->naturalSize().w));

    for(auto* b: buttons)
      if(b != nullptr)
        b->setWidth(width);
  }
}  // namespace

void alignButtons(std::initializer_list<Widget*> buttons, int minWidth)
{
  alignButtonGroup(buttons, minWidth);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void alignButtons(std::span<ButtonWidget* const> buttons, int minWidth)
{
  alignButtonGroup(buttons, minWidth);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void alignPopUps(std::initializer_list<PopUpWidget*> popups)
{
  int width = 0;
  for(const auto* p: popups)
    if(p != nullptr)
      width = std::max(width, p->boxWidth());

  for(auto* p: popups)
    if(p != nullptr)
      p->setBoxWidth(width);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int flushSpan(const PopUpWidget* popup, const Widget* popupLabel, int indent)
{
  if(popup == nullptr)
    return 0;

  // The pop-up's own width is just its value box and arrow (it carries no
  // label of its own), so its paired label's width has to be added back to
  // recover the full distance from the pop-up ROW's start; 'indent' then
  // carries that back to THIS row's own start
  const int popupLabelW = popupLabel != nullptr ? popupLabel->getWidth() : 0;
  return popupLabelW + popup->getWidth() - indent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void alignTracks(std::initializer_list<SliderWidget*> sliders,
                 const PopUpWidget* popup, int indent,
                 const Widget* sliderLabel, const Widget* popupLabel)
{
  if(popup == nullptr)
    return;

  // The track is the flush span, minus whatever the sliders' own label
  // column (a different alignLabels() group, if any) takes off the front
  const int sliderLabelW = sliderLabel != nullptr ? sliderLabel->getWidth() : 0;
  const int track = flushSpan(popup, popupLabel, indent) - sliderLabelW;

  for(auto* s: sliders)
    if(s != nullptr)
      s->setTrackWidth(track);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void alignTracks(std::initializer_list<SliderWidget*> sliders,
                 std::initializer_list<const Widget*> labels,
                 int span, int spacing)
{
  const int count = static_cast<int>(sliders.size());
  if(count == 0)
    return;

  // What in the span is NOT track: the gaps between them, every slider's label
  // (now a separate widget, so the CALLER names it -- a slider with none passes
  // nullptr), and every readout but the LAST, which hangs past the end
  int overhead = spacing * (count - 1);
  int idx = 0;
  auto label = labels.begin();

  for(auto* s: sliders)
  {
    const Widget* l = label != labels.end() ? *label++ : nullptr;
    if(s == nullptr)
      continue;

    if(l != nullptr)
      overhead += l->getWidth();
    if(++idx < count)  // not the last: its readout sits between the tracks
      overhead += s->getWidth() - s->trackWidth();
  }

  const int track = (span - overhead) / count;

  for(auto* s: sliders)
    if(s != nullptr)
      s->setTrackWidth(track);
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
unique_ptr<Layout> indentedFill(Widget* widget, int indent, int width)
{
  auto row = std::make_unique<BoxLayout>(BoxLayout::Dir::Horizontal);
  row->addSpace(indent);

  // A stated width is what makes the widget end flush with a SIBLING; without
  // one it simply fills the row it is given
  if(width > 0)
    row->addFixed(stretchedItem(widget), width);
  else
    row->addStretch(stretchedItem(widget));

  return row;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<Layout> labeledRow(Widget* label, Widget* control,
                              int labelW, int indent, bool fill)
{
  // Nothing here knows what kind of control it was handed: both items are
  // centered in the row, and each centers its own text, so the two texts meet
  auto row = std::make_unique<BoxLayout>(BoxLayout::Dir::Horizontal);
  if(indent > 0)
    row->addSpace(indent);
  row->addFixed(anchoredItem(label), labelW > 0 ? labelW : label->getWidth());
  if(fill)
    row->addStretch(alignedItem(control, HAlign::Fill, VAlign::Center));
  else
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
GridLayout& GridLayout::column(int idx, SizePolicy policy, int value, int maxSize,
                               int minSize)
{
  assert(idx >= 0 && std::cmp_less(idx, myColumns.size()));
  myColumns[idx] = Track{policy, value, maxSize, minSize};
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GridLayout& GridLayout::row(int idx, SizePolicy policy, int value, int maxSize,
                            int minSize)
{
  assert(idx >= 0 && std::cmp_less(idx, myRows.size()));
  myRows[idx] = Track{policy, value, maxSize, minSize};
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
void GridLayout::trackNaturals(bool horiz, IntArray& naturals) const
{
  const auto& tracks = horiz ? myColumns : myRows;
  naturals.assign(tracks.size(), 0);

  // A track is as large as the largest thing in it wants to be...
  for(const auto& cell: myCells)
  {
    const Common::Size cs = cell.layout->naturalSize();
    const int idx  = horiz ? cell.col : cell.row;
    const int span = horiz ? cell.colspan : cell.rowspan;
    if(span == 1)
      naturals[idx] = std::max(naturals[idx], static_cast<int>(horiz ? cs.w : cs.h));
  }
  // ...and a spanning cell grows its tracks when they cannot hold it between
  // them.  The growth goes to the FLEXIBLE tracks of the span if it has any,
  // since those are the ones that take up slack; a content-sized track must not
  // be widened by something that merely LIES ACROSS it (the wide detected-bezel
  // note, spanning the field and button columns, would otherwise make the button
  // column as wide as itself).  With no flexible track in the span, they all
  // grow — the cell has to fit somewhere
  const int spacing = horiz ? myHSpacing : myVSpacing;

  for(const auto& cell: myCells)
  {
    const int idx  = horiz ? cell.col : cell.row;
    const int span = horiz ? cell.colspan : cell.rowspan;
    if(span <= 1)
      continue;

    int have = spacing * (span - 1);
    for(int i = 0; i < span; ++i)
      have += naturals[idx + i];

    const Common::Size cs = cell.layout->naturalSize();
    const int deficit = static_cast<int>(horiz ? cs.w : cs.h) - have;
    if(deficit <= 0)
      continue;

    IntArray targets;
    for(int i = 0; i < span; ++i)
      if(tracks[idx + i].policy == SizePolicy::Stretch ||
         tracks[idx + i].policy == SizePolicy::Percent)
        targets.push_back(idx + i);
    if(targets.empty())
      for(int i = 0; i < span; ++i)
        targets.push_back(idx + i);

    // Share the shortfall out, remainder to the last of them
    const int n = static_cast<int>(targets.size());
    const int per = deficit / n;
    for(const int t: targets)
      naturals[t] += per;
    naturals[targets.back()] += deficit - per * n;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GridLayout::resolveTracks(const vector<Track>& tracks, int avail,
                               const IntArray& naturals, IntArray& ext)
{
  const int n = static_cast<int>(tracks.size());
  ext.assign(n, 0);

  // First pass: the tracks that do not depend on what is left over (and tally
  // the stretch weights)
  int used = 0, totalWeight = 0;
  for(int i = 0; i < n; ++i)
  {
    switch(tracks[i].policy)
    {
      case SizePolicy::Auto:
        ext[i] = naturals[i];
        break;
      case SizePolicy::Fixed:
        ext[i] = tracks[i].value;
        break;
      case SizePolicy::Percent:
        ext[i] = tracks[i].value * std::max(avail, 0) / 100;
        break;
      case SizePolicy::Stretch:
        totalWeight += tracks[i].value;
        // A stretch track is never smaller than its base size, so that is spoken
        // for before any leftover is shared out
        used += tracks[i].minSize;
        continue;  // sized in the second pass
    }
    if(tracks[i].maxSize > 0)
      ext[i] = std::min(ext[i], tracks[i].maxSize);
    used += ext[i];
  }

  // Second pass: stretch tracks take their base size, then share what is left
  // over by weight
  const int remaining = std::max(avail - used, 0);
  int distributed = 0, lastStretch = -1;
  for(int i = 0; i < n; ++i)
  {
    if(tracks[i].policy != SizePolicy::Stretch)
      continue;
    int e = tracks[i].minSize
          + (totalWeight > 0 ? remaining * tracks[i].value / totalWeight : 0);
    if(tracks[i].maxSize > 0)
      e = std::min(e, tracks[i].maxSize);
    ext[i] = e;
    distributed += e - tracks[i].minSize;
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

  IntArray colNat, rowNat, colExt, rowExt;
  trackNaturals(true, colNat);
  trackNaturals(false, rowNat);
  resolveTracks(myColumns, availW, colNat, colExt);
  resolveTracks(myRows, availH, rowNat, rowExt);

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
  IntArray colMin(cols, 0), rowMin(rows, 0), colNat, rowNat;

  trackNaturals(true, colNat);
  trackNaturals(false, rowNat);

  // A track can never be smaller than its own fixed size, nor one sized by its
  // content smaller than that content, nor a stretching one than its base
  for(int c = 0; c < cols; ++c)
    if(myColumns[c].policy == SizePolicy::Fixed)
      colMin[c] = myColumns[c].value;
    else if(myColumns[c].policy == SizePolicy::Auto)
      colMin[c] = colNat[c];
    else if(myColumns[c].policy == SizePolicy::Stretch)
      colMin[c] = myColumns[c].minSize;
  for(int r = 0; r < rows; ++r)
    if(myRows[r].policy == SizePolicy::Fixed)
      rowMin[r] = myRows[r].value;
    else if(myRows[r].policy == SizePolicy::Auto)
      rowMin[r] = rowNat[r];
    else if(myRows[r].policy == SizePolicy::Stretch)
      rowMin[r] = myRows[r].minSize;

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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size GridLayout::naturalSize() const
{
  const int cols = static_cast<int>(myColumns.size());
  const int rows = static_cast<int>(myRows.size());
  IntArray colNat, rowNat;

  trackNaturals(true, colNat);
  trackNaturals(false, rowNat);

  // A track sized in pixels wants exactly those, whatever is placed in it; a
  // stretching one wants at least the base it was given (its content — a field,
  // a list — has no width of its own to ask for)
  for(int c = 0; c < cols; ++c)
    if(myColumns[c].policy == SizePolicy::Fixed)
      colNat[c] = myColumns[c].value;
    else if(myColumns[c].policy == SizePolicy::Stretch)
      colNat[c] = std::max(colNat[c], myColumns[c].minSize);
  for(int r = 0; r < rows; ++r)
    if(myRows[r].policy == SizePolicy::Fixed)
      rowNat[r] = myRows[r].value;
    else if(myRows[r].policy == SizePolicy::Stretch)
      rowNat[r] = std::max(rowNat[r], myRows[r].minSize);

  int nw = 2 * myMarginH, nh = 2 * myMarginV;
  for(int c = 0; c < cols; ++c) nw += colNat[c];
  for(int r = 0; r < rows; ++r) nh += rowNat[r];
  if(cols > 0) nw += myHSpacing * (cols - 1);
  if(rows > 0) nh += myVSpacing * (rows - 1);

  return Common::Size(static_cast<uInt32>(nw), static_cast<uInt32>(nh));
}

}  // namespace GUI
