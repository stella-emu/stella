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

#include "Base.hxx"
#include "Font.hxx"
#include "OSystem.hxx"
#include "Debugger.hxx"
#include "RiotDebug.hxx"
#include "TIADebug.hxx"
#include "Widget.hxx"
#include "EditTextWidget.hxx"
#include "GuiObject.hxx"
#include "Layout.hxx"

#include "TiaInfoWidget.hxx"

// Between the current and the last-frame scanline counts, which share a row
static constexpr int SCAN_GAP = 2;

// The label of each row, in the two forms picked between by the width available
struct RowLabel { string_view full; string_view abbr; };

static constexpr std::array<RowLabel, 5> LEFT_LABELS{{
  {"Frame Cycles", "F. Cycls"},
  {"WSync Cycles", "WSync C."},
  {"Timer Cycles", "Timer C."},
  {"Total",        "Total"},
  {"Delta",        "Delta"}
}};
static constexpr std::array<RowLabel, 5> RIGHT_LABELS{{
  {"Frame Cnt.",  "Frame"},
  {"Scanline",    "Scn Ln"},
  {"Scan Cycle",  "Scn Cycle"},
  {"Pixel Pos",   "Pixel Pos"},
  {"Color Clock", "Color Clk"}
}};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaInfoWidget::TiaInfoWidget(GuiObject* boss, const GUI::Font& lfont,
                             const GUI::Font& nfont,
                             int x, int y, int max_w)
  : Widget(boss, lfont, x, y, 16, 16),
    CommandSender(boss)
{
  const int lineHeight = lfont.getLineHeight();

  // Create every field at a placeholder position/size; reflow() picks the
  // short/long label text and positions and sizes everything for the width
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  myFrameCyclesLabel = new StaticTextWidget(boss, lfont, 0, 0, "Frame Cycles");
  myFrameCycles = new EditTextWidget(boss, nfont, 0, 0, 1, lineHeight);
  myFrameCycles->setToolTip("CPU cycles executed this frame.");
  myFrameCycles->setEditable(false, true);

  myWSyncCyclesLabel = new StaticTextWidget(boss, lfont, 0, 0, "WSync Cycles");
  myWSyncCylces = new EditTextWidget(boss, nfont, 0, 0, 1, lineHeight);
  myWSyncCylces->setToolTip("CPU cycles used for WSYNC this frame.");
  myWSyncCylces->setEditable(false, true);

  myTimerCyclesLabel = new StaticTextWidget(boss, lfont, 0, 0, "Timer Cycles");
  myTimerCylces = new EditTextWidget(boss, nfont, 0, 0, 1, lineHeight);
  myTimerCylces->setToolTip("CPU cycles roughly used for INTIM reads this frame.");
  myTimerCylces->setEditable(false, true);

  myTotalLabel = new StaticTextWidget(boss, lfont, 0, 0, "Total");
  myTotalCycles = new EditTextWidget(boss, nfont, 0, 0, 1, lineHeight);
  myTotalCycles->setEditable(false, true);

  myDeltaLabel = new StaticTextWidget(boss, lfont, 0, 0, "Delta");
  myDeltaCycles = new EditTextWidget(boss, nfont, 0, 0, 1, lineHeight);
  myDeltaCycles->setToolTip("CPU cycles executed since last debug break.");
  myDeltaCycles->setEditable(false, true);

  myFrameCountLabel = new StaticTextWidget(boss, lfont, 0, 0, "Frame Cnt.");
  myFrameCount = new EditTextWidget(boss, nfont, 0, 0, 1, lineHeight);
  myFrameCount->setToolTip("Total number of frames executed this session.");
  myFrameCount->setEditable(false, true);

  myScanlineLabel = new StaticTextWidget(boss, lfont, 0, 0, "Scanline");
  myScanlineCountLast = new EditTextWidget(boss, nfont, 0, 0, 1, lineHeight);
  myScanlineCountLast->setToolTip("Number of scanlines of last frame.");
  myScanlineCountLast->setEditable(false, true);
  myScanlineCount = new EditTextWidget(boss, nfont, 0, 0, 1, lineHeight);
  myScanlineCount->setToolTip("Current scanline of this frame.");
  myScanlineCount->setEditable(false, true);

  myScanCycleLabel = new StaticTextWidget(boss, lfont, 0, 0, "Scan Cycle");
  myScanlineCycles = new EditTextWidget(boss, nfont, 0, 0, 1, lineHeight);
  myScanlineCycles->setToolTip("CPU cycles in current scanline.");
  myScanlineCycles->setEditable(false, true);

  myPixelPosLabel = new StaticTextWidget(boss, lfont, 0, 0, "Pixel Pos");
  myPixelPosition = new EditTextWidget(boss, nfont, 0, 0, 1, lineHeight);
  myPixelPosition->setToolTip("Pixel position in current scanline.");
  myPixelPosition->setEditable(false, true);

  myColorClockLabel = new StaticTextWidget(boss, lfont, 0, 0, "Color Clock");
  myColorClocks = new EditTextWidget(boss, nfont, 0, 0, 1, lineHeight);
  myColorClocks->setToolTip("Color clocks in current scanline.");
  myColorClocks->setEditable(false, true);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  reflow(max_w);

  //setHelpAnchor("TIAInfo", true); // TODO: does not work due to missing focus
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaInfoWidget::setArea(int x, int y, int w, int h)
{
  setPos(x, y);
  reflow(w);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaInfoWidget::ColumnWidths TiaInfoWidget::columnWidths(bool longstr) const
{
  const GUI::Font& lfont = _font;

  // Every label keeps one character of clearance before its value field
  const int space = lfont.getMaxCharWidth();
  const int fwidth5 = EditTextWidget::calcWidth(lfont, 5),
            fwidth3 = EditTextWidget::calcWidth(lfont, 3),
            twidth  = EditTextWidget::calcWidth(lfont, 8);

  // What each row ends with; the scanline row shows two counts side by side
  const std::array<int, 5> leftFields{fwidth5, fwidth5, fwidth5, twidth, twidth};
  const std::array<int, 5> rightFields{fwidth5, fwidth3 * 2 + SCAN_GAP,
                                       fwidth3, fwidth3, fwidth3};

  const auto width = [&](const auto& labels, const auto& fields) {
    int w = 0;
    for(size_t i = 0; i < labels.size(); ++i)
      w = std::max(w, lfont.getStringWidth(longstr ? labels[i].full
                                                   : labels[i].abbr)
                      + space + fields[i]);
    return w;
  };

  return {width(LEFT_LABELS, leftFields), width(RIGHT_LABELS, rightFields)};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TiaInfoWidget::minWidthFor(bool longstr) const
{
  // The base sizes of the three cells reflow() lays the widget out with
  const ColumnWidths w = columnWidths(longstr);

  return w.left + columnGap() + w.right;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TiaInfoWidget::minWidth() const
{
  return minWidthFor(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaInfoWidget::reflow(int max_w)
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using Dir = BoxLayout::Dir;

  const GUI::Font& lfont = _font;
  const int VGAP = lfont.getLineHeight() / 4;
  constexpr int VBORDER = 5 + 1;
  // Spell the labels out only once they fit without eating their own clearance
  const bool longstr = minWidthFor(true) <= max_w;
  const int lineHeight = lfont.getLineHeight();
  const int fwidth5 = EditTextWidget::calcWidth(lfont, 5);
  const int fwidth3 = EditTextWidget::calcWidth(lfont, 3);
  const int twidth  = EditTextWidget::calcWidth(lfont, 8);

  // Set each label's text (short or long) and its natural width, and each value
  // field's font-derived width, so the anchored layout items below pick up the
  // right sizes (anchored items keep their own size and are only repositioned)
  const auto setText = [&](StaticTextWidget* w, const RowLabel& l) {
    const string_view s = longstr ? l.full : l.abbr;
    w->setLabel(s);
    w->setWidth(lfont.getStringWidth(s));
  };
  setText(myFrameCyclesLabel, LEFT_LABELS[0]);
  setText(myWSyncCyclesLabel, LEFT_LABELS[1]);
  setText(myTimerCyclesLabel, LEFT_LABELS[2]);
  setText(myTotalLabel,       LEFT_LABELS[3]);
  setText(myDeltaLabel,       LEFT_LABELS[4]);
  setText(myFrameCountLabel,  RIGHT_LABELS[0]);
  setText(myScanlineLabel,    RIGHT_LABELS[1]);
  setText(myScanCycleLabel,   RIGHT_LABELS[2]);
  setText(myPixelPosLabel,    RIGHT_LABELS[3]);
  setText(myColorClockLabel,  RIGHT_LABELS[4]);

  myFrameCycles->setWidth(fwidth5);
  myWSyncCylces->setWidth(fwidth5);
  myTimerCylces->setWidth(fwidth5);
  myTotalCycles->setWidth(twidth);
  myDeltaCycles->setWidth(twidth);
  myFrameCount->setWidth(fwidth5);
  myScanlineCount->setWidth(fwidth3);
  myScanlineCountLast->setWidth(fwidth3);
  myScanlineCycles->setWidth(fwidth3);
  myPixelPosition->setWidth(fwidth3);
  myColorClocks->setWidth(fwidth3);

  // A row stretches its label across whatever width the column has and anchors
  // its value field at the right, so that a column's fields all end flush, no
  // matter how wide its labels or how many digits each field holds
  const auto row = [&](StaticTextWidget* label, Widget* field, int fieldW) {
    auto r = std::make_unique<BoxLayout>(Dir::Horizontal);
    r->addStretch(anchoredItem(label));
    r->addFixed(anchoredItem(field), fieldW);
    return r;
  };

  auto left = std::make_unique<BoxLayout>(Dir::Vertical, VGAP, 0, VBORDER);
  left->addFixed(row(myFrameCyclesLabel, myFrameCycles, fwidth5), lineHeight);
  left->addFixed(row(myWSyncCyclesLabel, myWSyncCylces, fwidth5), lineHeight);
  left->addFixed(row(myTimerCyclesLabel, myTimerCylces, fwidth5), lineHeight);
  left->addFixed(row(myTotalLabel, myTotalCycles, twidth), lineHeight);
  left->addFixed(row(myDeltaLabel, myDeltaCycles, twidth), lineHeight);

  auto right = std::make_unique<BoxLayout>(Dir::Vertical, VGAP, 0, VBORDER);
  right->addFixed(row(myFrameCountLabel, myFrameCount, fwidth5), lineHeight);
  // The scanline row shows the current and last-frame counts side by side
  auto scanRow = std::make_unique<BoxLayout>(Dir::Horizontal);
  scanRow->addStretch(anchoredItem(myScanlineLabel));
  scanRow->addFixed(anchoredItem(myScanlineCount), fwidth3);
  scanRow->addSpace(SCAN_GAP);
  scanRow->addFixed(anchoredItem(myScanlineCountLast), fwidth3);
  right->addFixed(std::move(scanRow), lineHeight);
  right->addFixed(row(myScanCycleLabel, myScanlineCycles, fwidth3), lineHeight);
  right->addFixed(row(myPixelPosLabel, myPixelPosition, fwidth3), lineHeight);
  right->addFixed(row(myColorClockLabel, myColorClocks, fwidth3), lineHeight);

  // The two columns and the gap between them grow from their natural sizes in
  // the proportion 1 : 2 : 1, so the gap always outgrows the label clearances
  const ColumnWidths cw = columnWidths(longstr);

  BoxLayout root(Dir::Horizontal);
  root.addStretch(std::move(left), 1, cw.left);
  root.addStretchSpace(2, columnGap());
  root.addStretch(std::move(right), 1, cw.right);

  // The columns are as tall as their rows, margins and gaps make them
  root.doLayout(_x, _y, max_w, static_cast<int>(root.minSize().h));

  // Final dimensions (the bottom-right field is the last right-column row)
  _w = max_w;
  _h = myColorClocks->getBottom() - _y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaInfoWidget::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaInfoWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaInfoWidget::loadConfig()
{
  const Debugger& dbg = instance().debugger();
  TIADebug& tia = dbg.tiaDebug();
  const auto& oldTia = static_cast<const TiaState&>(tia.getOldState());
  RiotDebug& riot = dbg.riotDebug();
  const auto& oldRiot = static_cast<const RiotState&>(riot.getOldState());

  myFrameCount->setText(Common::Base::toString(tia.frameCount(), Common::Base::Fmt::_10_5),
                        tia.frameCount() != oldTia.info[0]);
  myFrameCycles->setText(Common::Base::toString(tia.frameCycles(), Common::Base::Fmt::_10_5),
                         tia.frameCycles() != oldTia.info[1]);

  const uInt64 total = tia.cyclesLo() + (static_cast<uInt64>(tia.cyclesHi()) << 32);
  const uInt64 totalOld = oldTia.info[2] + (static_cast<uInt64>(oldTia.info[3]) << 32);
  myTotalCycles->setText(Common::Base::toString(static_cast<uInt32>(total) / 1000000,
                         Common::Base::Fmt::_10_6) + "e6",
                         total / 1000000 != totalOld / 1000000);
  myTotalCycles->setToolTip("Total CPU cycles (E notation) executed for this session ("
                            + std::to_string(total) + ").");

  const uInt64 delta = total - totalOld;
  myDeltaCycles->setText(Common::Base::toString(static_cast<uInt32>(delta),
                         Common::Base::Fmt::_10_8)); // no coloring

  const int clk = tia.clocksThisLine();
  myScanlineCount->setText(Common::Base::toString(tia.scanlines(), Common::Base::Fmt::_10_3),
                           tia.scanlines() != oldTia.info[4]);
  myScanlineCountLast->setText(
    Common::Base::toString(tia.scanlinesLastFrame(), Common::Base::Fmt::_10_3),
    tia.scanlinesLastFrame() != oldTia.info[5]);
  myScanlineCycles->setText(Common::Base::toString(clk/3, Common::Base::Fmt::_10),
                            clk != oldTia.info[6]);
  myPixelPosition->setText(Common::Base::toString(clk-68, Common::Base::Fmt::_10),
                           clk != oldTia.info[6]);
  myColorClocks->setText(Common::Base::toString(clk, Common::Base::Fmt::_10),
                         clk != oldTia.info[6]);

  myWSyncCylces->setText(Common::Base::toString(tia.frameWsyncCycles(), Common::Base::Fmt::_10_5),
                         tia.frameWsyncCycles() != oldTia.info[7]);

  myTimerCylces->setText(Common::Base::toString(riot.timReadCycles(), Common::Base::Fmt::_10_5),
                         riot.timReadCycles() != oldRiot.timReadCycles);
}
