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

// How many characters wide each kind of value field is
static constexpr int CYCLE_CHARS = 5,  // a cycle count for this frame
                     TOTAL_CHARS = 8,  // the session total, in E notation
                     COUNT_CHARS = 3;  // a scanline, pixel or clock count

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
                             const GUI::Font& nfont)
  : Widget(boss, lfont, 0, 0, 0, 0),
    CommandSender(boss)
{
  // Create every field; reflow() picks the short/long label text and positions
  // and sizes everything for the width the parent layout gives us
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  myFrameCyclesLabel = new StaticTextWidget(boss, lfont, "Frame Cycles");
  myFrameCycles = new EditTextWidget(boss, nfont, 1);
  myFrameCycles->setToolTip("CPU cycles executed this frame.");
  myFrameCycles->setEditable(false, true);

  myWSyncCyclesLabel = new StaticTextWidget(boss, lfont, "WSync Cycles");
  myWSyncCylces = new EditTextWidget(boss, nfont, 1);
  myWSyncCylces->setToolTip("CPU cycles used for WSYNC this frame.");
  myWSyncCylces->setEditable(false, true);

  myTimerCyclesLabel = new StaticTextWidget(boss, lfont, "Timer Cycles");
  myTimerCylces = new EditTextWidget(boss, nfont, 1);
  myTimerCylces->setToolTip("CPU cycles roughly used for INTIM reads this frame.");
  myTimerCylces->setEditable(false, true);

  myTotalLabel = new StaticTextWidget(boss, lfont, "Total");
  myTotalCycles = new EditTextWidget(boss, nfont, 1);
  myTotalCycles->setEditable(false, true);

  myDeltaLabel = new StaticTextWidget(boss, lfont, "Delta");
  myDeltaCycles = new EditTextWidget(boss, nfont, 1);
  myDeltaCycles->setToolTip("CPU cycles executed since last debug break.");
  myDeltaCycles->setEditable(false, true);

  myFrameCountLabel = new StaticTextWidget(boss, lfont, "Frame Cnt.");
  myFrameCount = new EditTextWidget(boss, nfont, 1);
  myFrameCount->setToolTip("Total number of frames executed this session.");
  myFrameCount->setEditable(false, true);

  myScanlineLabel = new StaticTextWidget(boss, lfont, "Scanline");
  myScanlineCountLast = new EditTextWidget(boss, nfont, 1);
  myScanlineCountLast->setToolTip("Number of scanlines of last frame.");
  myScanlineCountLast->setEditable(false, true);
  myScanlineCount = new EditTextWidget(boss, nfont, 1);
  myScanlineCount->setToolTip("Current scanline of this frame.");
  myScanlineCount->setEditable(false, true);

  myScanCycleLabel = new StaticTextWidget(boss, lfont, "Scan Cycle");
  myScanlineCycles = new EditTextWidget(boss, nfont, 1);
  myScanlineCycles->setToolTip("CPU cycles in current scanline.");
  myScanlineCycles->setEditable(false, true);

  myPixelPosLabel = new StaticTextWidget(boss, lfont, "Pixel Pos");
  myPixelPosition = new EditTextWidget(boss, nfont, 1);
  myPixelPosition->setToolTip("Pixel position in current scanline.");
  myPixelPosition->setEditable(false, true);

  myColorClockLabel = new StaticTextWidget(boss, lfont, "Color Clock");
  myColorClocks = new EditTextWidget(boss, nfont, 1);
  myColorClocks->setToolTip("Color clocks in current scanline.");
  myColorClocks->setEditable(false, true);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  // Each label switches between a long and a short form as the width allows, so
  // it must take its width from whichever one it is currently showing
  for(auto* l: {myFrameCyclesLabel, myWSyncCyclesLabel, myTimerCyclesLabel,
                myTotalLabel, myDeltaLabel, myFrameCountLabel, myScanlineLabel,
                myScanCycleLabel, myPixelPosLabel, myColorClockLabel})
    l->setAutoResize(true);

  reflow();

  //setHelpAnchor("TIAInfo", true); // TODO: does not work due to missing focus
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaInfoWidget::setArea(int x, int y, int w, int h)
{
  Widget::setArea(x, y, w, h);
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaInfoWidget::setLabels(bool longstr)
{
  const auto set = [&](StaticTextWidget* w, const RowLabel& l) {
    w->setLabel(longstr ? l.full : l.abbr);
  };
  set(myFrameCyclesLabel, LEFT_LABELS[0]);
  set(myWSyncCyclesLabel, LEFT_LABELS[1]);
  set(myTimerCyclesLabel, LEFT_LABELS[2]);
  set(myTotalLabel,       LEFT_LABELS[3]);
  set(myDeltaLabel,       LEFT_LABELS[4]);
  set(myFrameCountLabel,  RIGHT_LABELS[0]);
  set(myScanlineLabel,    RIGHT_LABELS[1]);
  set(myScanCycleLabel,   RIGHT_LABELS[2]);
  set(myPixelPosLabel,    RIGHT_LABELS[3]);
  set(myColorClockLabel,  RIGHT_LABELS[4]);

  myLongLabels = longstr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<GUI::BoxLayout> TiaInfoWidget::buildLayout() const
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::stretchedItem;
  using Dir = BoxLayout::Dir;

  const int VGAP = _font.getLineHeight() / 4;
  const int VBORDER = _font.getFontHeight() / 2;
  // Every label keeps one character of clearance before its value field, which
  // is simply the row's spacing -- so a row is that much wider than its parts
  const int space = _font.getMaxCharWidth();

  // A field is as wide as the characters it has to show; it fills the cell that
  // says so, which is how its width follows the font without anyone setting it
  const auto field = [&](Widget* w, int chars) {
    return std::pair{stretchedItem(w), EditTextWidget::calcWidth(_font, chars)};
  };

  // A row stretches its label across whatever width the column has and anchors
  // its value field at the right, so that a column's fields all end flush, no
  // matter how wide its labels or how many characters each field holds
  const auto row = [&](StaticTextWidget* label, Widget* value, int chars) {
    auto [item, w] = field(value, chars);
    auto r = std::make_unique<BoxLayout>(Dir::Horizontal, space);
    r->addStretch(anchoredItem(label));
    r->addFixed(std::move(item), w);
    return r;
  };

  auto left = std::make_unique<BoxLayout>(Dir::Vertical, VGAP, 0, VBORDER);
  left->addAuto(row(myFrameCyclesLabel, myFrameCycles, CYCLE_CHARS));
  left->addAuto(row(myWSyncCyclesLabel, myWSyncCylces, CYCLE_CHARS));
  left->addAuto(row(myTimerCyclesLabel, myTimerCylces, CYCLE_CHARS));
  left->addAuto(row(myTotalLabel, myTotalCycles, TOTAL_CHARS));
  left->addAuto(row(myDeltaLabel, myDeltaCycles, TOTAL_CHARS));

  // The scanline row shows the current and last-frame counts side by side
  auto counts = std::make_unique<BoxLayout>(Dir::Horizontal, SCAN_GAP);
  for(auto* c: {myScanlineCount, myScanlineCountLast})
  {
    auto [item, w] = field(c, COUNT_CHARS);
    counts->addFixed(std::move(item), w);
  }

  auto scanRow = std::make_unique<BoxLayout>(Dir::Horizontal, space);
  scanRow->addStretch(anchoredItem(myScanlineLabel));
  scanRow->addAuto(std::move(counts));

  auto right = std::make_unique<BoxLayout>(Dir::Vertical, VGAP, 0, VBORDER);
  right->addAuto(row(myFrameCountLabel, myFrameCount, CYCLE_CHARS));
  right->addAuto(std::move(scanRow));
  right->addAuto(row(myScanCycleLabel, myScanlineCycles, COUNT_CHARS));
  right->addAuto(row(myPixelPosLabel, myPixelPosition, COUNT_CHARS));
  right->addAuto(row(myColorClockLabel, myColorClocks, COUNT_CHARS));

  // The two columns and the gap between them start at the size their contents
  // need and share any surplus in the proportion 1 : 2 : 1, so the gap always
  // outgrows the label clearances
  const int leftW  = static_cast<int>(left->naturalSize().w),
            rightW = static_cast<int>(right->naturalSize().w);

  auto root = std::make_unique<BoxLayout>(Dir::Horizontal);
  root->addStretch(std::move(left), 1, leftW);
  root->addStretchSpace(2, columnGap());
  root->addStretch(std::move(right), 1, rightW);

  return root;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size TiaInfoWidget::naturalSize() const
{
  return buildLayout()->naturalSize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TiaInfoWidget::naturalWidthFor(bool longstr)
{
  setLabels(longstr);

  return static_cast<int>(naturalSize().w);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TiaInfoWidget::minWidth()
{
  // Measuring means showing the short labels, so put back what was on screen
  const bool wasLong = myLongLabels;
  const int w = naturalWidthFor(false);

  setLabels(wasLong);

  return w;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaInfoWidget::reflow()
{
  // Spell the labels out only once they fit without eating their own clearance
  if(naturalWidthFor(true) > _w)
    setLabels(false);

  buildLayout()->doLayout(_x, _y, _w, _h);
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
