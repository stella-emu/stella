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
void TiaInfoWidget::reflow(int max_w)
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::labeledRow;
  using Dir = BoxLayout::Dir;

  const GUI::Font& lfont = _font;
  const int VGAP = lfont.getLineHeight() / 4;
  constexpr int VBORDER = 5 + 1;
  const int COLUMN_GAP = _fontWidth * 1.25;
  const bool longstr = lfont.getStringWidth("Frame Cycles12345") + _fontWidth * 0.5
    + COLUMN_GAP + lfont.getStringWidth("Scanline262262")
    + EditTextWidget::calcWidth(lfont) * 3 <= max_w;
  const int lineHeight = lfont.getLineHeight();
  int lwidth  = lfont.getStringWidth(longstr ? "Frame Cycles" : "F. Cycls");
  int lwidth8 = lwidth - lfont.getMaxCharWidth() * 3;
  int lwidthR = lfont.getStringWidth(longstr ? "Frame Cnt." : "Frame   ");
  const int fwidth5 = EditTextWidget::calcWidth(lfont, 5);
  const int fwidth3 = EditTextWidget::calcWidth(lfont, 3);
  const int twidth  = EditTextWidget::calcWidth(lfont, 8);
  const int LGAP = (max_w - lwidth - fwidth5 - lwidthR - fwidth5) / 4;

  lwidth  += LGAP;
  lwidth8 += LGAP;
  lwidthR += LGAP;

  // Set each label's text (short or long) and its natural width, and each value
  // field's font-derived width, so the anchored layout items below pick up the
  // right sizes (anchored items keep their own size and are only repositioned)
  const auto setText = [&](StaticTextWidget* w, string_view s) {
    w->setLabel(s);
    w->setWidth(lfont.getStringWidth(s));
  };
  setText(myFrameCyclesLabel, longstr ? "Frame Cycles" : "F. Cycls");
  setText(myWSyncCyclesLabel, longstr ? "WSync Cycles" : "WSync C.");
  setText(myTimerCyclesLabel, longstr ? "Timer Cycles" : "Timer C.");
  setText(myTotalLabel, "Total");
  setText(myDeltaLabel, "Delta");
  setText(myFrameCountLabel, longstr ? "Frame Cnt." : "Frame");
  setText(myScanlineLabel, longstr ? "Scanline" : "Scn Ln");
  setText(myScanCycleLabel, longstr ? "Scan Cycle" : "Scn Cycle");
  setText(myPixelPosLabel, "Pixel Pos");
  setText(myColorClockLabel, longstr ? "Color Clock" : "Color Clk");

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

  // Right column origin and its lower label-column width
  const int rightX = _x + max_w - lwidthR - fwidth5;
  const int lwidth2 = lfont.getStringWidth(longstr ? "Color Clock " : "Pixel Pos ") + LGAP;
  const int colH = VBORDER * 2 + 5 * lineHeight + 4 * VGAP;

  // Left column: label + value form rows (Total/Delta use a shorter label
  // column so their wider 8-digit value ends flush with the 5-digit ones)
  BoxLayout left(Dir::Vertical, VGAP, 0, VBORDER);
  left.addFixed(labeledRow(myFrameCyclesLabel, myFrameCycles, lwidth), lineHeight);
  left.addFixed(labeledRow(myWSyncCyclesLabel, myWSyncCylces, lwidth), lineHeight);
  left.addFixed(labeledRow(myTimerCyclesLabel, myTimerCylces, lwidth), lineHeight);
  left.addFixed(labeledRow(myTotalLabel, myTotalCycles, lwidth8), lineHeight);
  left.addFixed(labeledRow(myDeltaLabel, myDeltaCycles, lwidth8), lineHeight);
  left.doLayout(_x, _y, rightX - _x, colH);

  // Right column
  BoxLayout right(Dir::Vertical, VGAP, 0, VBORDER);
  right.addFixed(labeledRow(myFrameCountLabel, myFrameCount, lwidthR), lineHeight);
  // The scanline row shows the current and last-frame counts side by side
  auto scanRow = std::make_unique<BoxLayout>(Dir::Horizontal);
  scanRow->addFixed(anchoredItem(myScanlineLabel), lwidth2 - fwidth3 - 2);
  scanRow->addFixed(anchoredItem(myScanlineCount), fwidth3);
  scanRow->addSpace(2);
  scanRow->addFixed(anchoredItem(myScanlineCountLast), fwidth3);
  right.addFixed(std::move(scanRow), lineHeight);
  right.addFixed(labeledRow(myScanCycleLabel, myScanlineCycles, lwidth2), lineHeight);
  right.addFixed(labeledRow(myPixelPosLabel, myPixelPosition, lwidth2), lineHeight);
  right.addFixed(labeledRow(myColorClockLabel, myColorClocks, lwidth2), lineHeight);
  right.doLayout(rightX, _y, max_w - (rightX - _x), colH);

  // Final dimensions (the bottom-right field is the last right-column row)
  _w = myColorClocks->getRight() - _x;
  _h = myColorClocks->getBottom();
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
