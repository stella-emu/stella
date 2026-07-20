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

#include "ColorWidget.hxx"
#include "DataGridWidget.hxx"
#include "EditTextWidget.hxx"
#include "FrameBuffer.hxx"
#include "Font.hxx"
#include "GuiObject.hxx"
#include "Layout.hxx"
#include "OSystem.hxx"
#include "Debugger.hxx"
#include "TIA.hxx"
#include "TIADebug.hxx"
#include "TogglePixelWidget.hxx"
#include "Widget.hxx"
#include "DelayQueueWidget.hxx"
#include "TiaWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaWidget::TiaWidget(GuiObject* boss, const GUI::Font& lfont,
                     const GUI::Font& nfont)
  : Widget(boss, lfont, 0, 0),
    CommandSender(boss)
{
  const int lineHeight = lfont.getLineHeight();

  // Create every widget; reflow() positions and sizes them for the area the
  // parent layout gives us
  ////////////////////////////
  // VSync/VBlank
  ////////////////////////////
  myVSync = new CheckboxWidget(boss, lfont, "VSync", kVSyncCmd);
  myVSync->setTarget(this);
  addFocusWidget(myVSync);

  myVBlank = new CheckboxWidget(boss, lfont, "VBlank", kVBlankCmd);
  myVBlank->setTarget(this);
  addFocusWidget(myVBlank);

  // Color registers
  static constexpr std::array<string_view, 4> regNames = {
    "COLUP0", "COLUP1", "COLUPF", "COLUBK"
  };
  for(int row = 0; row < 4; ++row)
    myColorRegLabels[row] = new StaticTextWidget(boss, lfont, regNames[row]);

  myColorRegs = new DataGridWidget(boss, nfont,
                                   1, 4, 2, 8, Common::Base::Fmt::_16);
  myColorRegs->setTarget(this);
  myColorRegs->setID(kColorRegsID);
  addFocusWidget(myColorRegs);

  // A colour swatch is half again as wide as it is tall; reflow() re-applies
  // both, so they follow the font
  const auto swatch = [&]() {
    auto* c = new ColorWidget(boss, nfont,
                              static_cast<uInt32>(1.5 * lineHeight), lineHeight - 4);
    c->setTarget(this);
    return c;
  };
  myCOLUP0Color = swatch();
  myCOLUP1Color = swatch();
  myCOLUPFColor = swatch();
  myCOLUBKColor = swatch();

  // Fixed debug colors
  myFixedEnabled = new CheckboxWidget(boss, lfont, "Debug Colors", kDbgClCmd);
  myFixedEnabled->setToolTip("Enable fixed debug colors", Event::ToggleFixedColors);
  myFixedEnabled->setTarget(this);
  addFocusWidget(myFixedEnabled);

  static constexpr std::array<string_view, 8> dbgLabels = {
    "P0", "P1", "PF", "BK", "M0", "M1", "BL", "HM"
  };
  for(uInt32 row = 0; row < 8; ++row)
  {
    myDbgColorLabels[row] = new StaticTextWidget(boss, lfont, dbgLabels[row]);
    myFixedColors[row] = swatch();
  }

  ////////////////////////////
  // Collision register bits
  ////////////////////////////
  // All 15 collision bits, with a label down the left and along the top
  static constexpr std::array<string_view, 5> rowLabel = { "P0", "P1", "M0", "M1", "BL" };
  static constexpr std::array<string_view, 5> colLabel = { "PF", "BL", "M1", "M0", "P1" };
  int idx = 0;
  for(uInt32 row = 0; row < 5; ++row)
  {
    myCollRowLabels[row] = new StaticTextWidget(boss, lfont, rowLabel[row]);
    myCollColLabels[row] = new StaticTextWidget(boss, lfont, colLabel[row]);

    for(uInt32 col = 0; col < 5 - row; ++col, ++idx)
    {
      myCollision[idx] = new CheckboxWidget(boss, lfont, "",
                                            CheckboxWidget::kCheckActionCmd);
      myCollision[idx]->setTarget(this);
      myCollision[idx]->setID(idx);
    }
  }

  // Clear all collision bits
  myCxclrButton = new ButtonWidget(boss, lfont, "CXCLR", kCxclrCmd);
  myCxclrButton->setCompact();
  myCxclrButton->setTarget(this);
  addFocusWidget(myCxclrButton);

  ////////////////////////////
  // P0 register info
  ////////////////////////////
  myBlockLabels[0].name = new StaticTextWidget(boss, lfont, "P0");
  myBlockLabels[0].pos  = new StaticTextWidget(boss, lfont, "Pos#");
  myBlockLabels[0].hm   = new StaticTextWidget(boss, lfont, "HM");
  myBlockLabels[0].size = new StaticTextWidget(boss, lfont, "NuSiz");

  // grP0 (new)
  myGRP0 = new TogglePixelWidget(boss, nfont, 8, 1);
  myGRP0->setTarget(this);
  myGRP0->setID(kGRP0ID);
  myGRP0->clearBackgroundColor();
  addFocusWidget(myGRP0);

  // posP0
  myPosP0 = new DataGridWidget(boss, nfont,
                               1, 1, 3, 8, Common::Base::Fmt::_10);
  myPosP0->setTarget(this);
  myPosP0->setID(kPosP0ID);
  myPosP0->setRange(0, 160);
  addFocusWidget(myPosP0);

  // hmP0
  myHMP0 = new DataGridWidget(boss, nfont,
                              1, 1, 1, 4, Common::Base::Fmt::_16_1);
  myHMP0->setTarget(this);
  myHMP0->setID(kHMP0ID);
  addFocusWidget(myHMP0);

  // P0 reflect
  myRefP0 = new CheckboxWidget(boss, lfont,
                               "Reflect", CheckboxWidget::kCheckActionCmd);
  myRefP0->setTarget(this);
  myRefP0->setID(kRefP0ID);
  addFocusWidget(myRefP0);

  // P0 reset
  myResButtons[0] = new ButtonWidget(boss, lfont, "RESP0", kResP0Cmd);
  myResButtons[0]->setCompact();
  myResButtons[0]->setTarget(this);
  addFocusWidget(myResButtons[0]);

  // grP0 (old)
  myGRP0Old = new TogglePixelWidget(boss, nfont, 8, 1);
  myGRP0Old->setTarget(this);
  myGRP0Old->setID(kGRP0OldID);
  myGRP0Old->clearBackgroundColor();
  addFocusWidget(myGRP0Old);

  // P0 delay
  myDelP0 = new CheckboxWidget(boss, lfont,
                               "VDel", CheckboxWidget::kCheckActionCmd);
  myDelP0->setTarget(this);
  myDelP0->setID(kDelP0ID);
  addFocusWidget(myDelP0);

  // NUSIZ0 (player portion)
  myNusizP0 = new DataGridWidget(boss, nfont,
                                 1, 1, 1, 3, Common::Base::Fmt::_16_1);
  myNusizP0->setTarget(this);
  myNusizP0->setID(kNusizP0ID);
  addFocusWidget(myNusizP0);

  myNusizP0Text = new EditTextWidget(boss, nfont, 1, lineHeight);
  myNusizP0Text->setEditable(false, true);

  ////////////////////////////
  // P1 register info
  ////////////////////////////
  myBlockLabels[1].name = new StaticTextWidget(boss, lfont, "P1");
  myBlockLabels[1].pos  = new StaticTextWidget(boss, lfont, "Pos#");
  myBlockLabels[1].hm   = new StaticTextWidget(boss, lfont, "HM");
  myBlockLabels[1].size = new StaticTextWidget(boss, lfont, "NuSiz");

  // grP1 (new)
  myGRP1 = new TogglePixelWidget(boss, nfont, 8, 1);
  myGRP1->setTarget(this);
  myGRP1->setID(kGRP1ID);
  myGRP1->clearBackgroundColor();
  addFocusWidget(myGRP1);

  // posP1
  myPosP1 = new DataGridWidget(boss, nfont,
                               1, 1, 3, 8, Common::Base::Fmt::_10);
  myPosP1->setTarget(this);
  myPosP1->setID(kPosP1ID);
  myPosP1->setRange(0, 160);
  addFocusWidget(myPosP1);

  // hmP1
  myHMP1 = new DataGridWidget(boss, nfont,
                              1, 1, 1, 4, Common::Base::Fmt::_16_1);
  myHMP1->setTarget(this);
  myHMP1->setID(kHMP1ID);
  addFocusWidget(myHMP1);

  // P1 reflect
  myRefP1 = new CheckboxWidget(boss, lfont,
                               "Reflect", CheckboxWidget::kCheckActionCmd);
  myRefP1->setTarget(this);
  myRefP1->setID(kRefP1ID);
  addFocusWidget(myRefP1);

  // P1 reset
  myResButtons[1] = new ButtonWidget(boss, lfont, "RESP1", kResP1Cmd);
  myResButtons[1]->setCompact();
  myResButtons[1]->setTarget(this);
  addFocusWidget(myResButtons[1]);

  // grP1 (old)
  myGRP1Old = new TogglePixelWidget(boss, nfont, 8, 1);
  myGRP1Old->setTarget(this);
  myGRP1Old->setID(kGRP1OldID);
  myGRP1Old->clearBackgroundColor();
  addFocusWidget(myGRP1Old);

  // P1 delay
  myDelP1 = new CheckboxWidget(boss, lfont,
                               "VDel", CheckboxWidget::kCheckActionCmd);
  myDelP1->setTarget(this);
  myDelP1->setID(kDelP1ID);
  addFocusWidget(myDelP1);

  // NUSIZ1 (player portion)
  myNusizP1 = new DataGridWidget(boss, nfont,
                                 1, 1, 1, 3, Common::Base::Fmt::_16_1);
  myNusizP1->setTarget(this);
  myNusizP1->setID(kNusizP1ID);
  addFocusWidget(myNusizP1);

  myNusizP1Text = new EditTextWidget(boss, nfont, 1, lineHeight);
  myNusizP1Text->setEditable(false, true);

  ////////////////////////////
  // M0 register info
  ////////////////////////////
  myBlockLabels[2].name = new StaticTextWidget(boss, lfont, "M0");
  myBlockLabels[2].pos  = new StaticTextWidget(boss, lfont, "Pos#");
  myBlockLabels[2].hm   = new StaticTextWidget(boss, lfont, "HM");
  myBlockLabels[2].size = new StaticTextWidget(boss, lfont, "Size");

  // enaM0
  myEnaM0 = new TogglePixelWidget(boss, nfont, 1, 1);
  myEnaM0->setTarget(this);
  myEnaM0->setID(kEnaM0ID);
  myEnaM0->clearBackgroundColor();
  addFocusWidget(myEnaM0);

  // posM0
  myPosM0 = new DataGridWidget(boss, nfont,
                               1, 1, 3, 8, Common::Base::Fmt::_10);
  myPosM0->setTarget(this);
  myPosM0->setID(kPosM0ID);
  myPosM0->setRange(0, 160);
  addFocusWidget(myPosM0);

  // hmM0
  myHMM0 = new DataGridWidget(boss, nfont,
                              1, 1, 1, 4, Common::Base::Fmt::_16_1);
  myHMM0->setTarget(this);
  myHMM0->setID(kHMM0ID);
  addFocusWidget(myHMM0);

  // NUSIZ0 (missile portion)
  myNusizM0 = new DataGridWidget(boss, nfont,
                                 1, 1, 1, 2, Common::Base::Fmt::_16_1);
  myNusizM0->setTarget(this);
  myNusizM0->setID(kNusizM0ID);
  addFocusWidget(myNusizM0);

  // M0 reset to player 0
  myResMP0 = new CheckboxWidget(boss, lfont,
                                "Reset to P0", CheckboxWidget::kCheckActionCmd);
  myResMP0->setTarget(this);
  myResMP0->setID(kResMP0ID);
  addFocusWidget(myResMP0);

  // M0 reset
  myResButtons[2] = new ButtonWidget(boss, lfont, "RESM0", kResM0Cmd);
  myResButtons[2]->setCompact();
  myResButtons[2]->setTarget(this);
  addFocusWidget(myResButtons[2]);

  ////////////////////////////
  // M1 register info
  ////////////////////////////
  myBlockLabels[3].name = new StaticTextWidget(boss, lfont, "M1");
  myBlockLabels[3].pos  = new StaticTextWidget(boss, lfont, "Pos#");
  myBlockLabels[3].hm   = new StaticTextWidget(boss, lfont, "HM");
  myBlockLabels[3].size = new StaticTextWidget(boss, lfont, "Size");

  // enaM1
  myEnaM1 = new TogglePixelWidget(boss, nfont, 1, 1);
  myEnaM1->setTarget(this);
  myEnaM1->setID(kEnaM1ID);
  myEnaM1->clearBackgroundColor();
  addFocusWidget(myEnaM1);

  // posM1
  myPosM1 = new DataGridWidget(boss, nfont,
                               1, 1, 3, 8, Common::Base::Fmt::_10);
  myPosM1->setTarget(this);
  myPosM1->setID(kPosM1ID);
  myPosM1->setRange(0, 160);
  addFocusWidget(myPosM1);

  // hmM1
  myHMM1 = new DataGridWidget(boss, nfont,
                              1, 1, 1, 4, Common::Base::Fmt::_16_1);
  myHMM1->setTarget(this);
  myHMM1->setID(kHMM1ID);
  addFocusWidget(myHMM1);

  // NUSIZ1 (missile portion)
  myNusizM1 = new DataGridWidget(boss, nfont,
                                 1, 1, 1, 2, Common::Base::Fmt::_16_1);
  myNusizM1->setTarget(this);
  myNusizM1->setID(kNusizM1ID);
  addFocusWidget(myNusizM1);

  // M1 reset to player 1
  myResMP1 = new CheckboxWidget(boss, lfont,
                                "Reset to P1", CheckboxWidget::kCheckActionCmd);
  myResMP1->setTarget(this);
  myResMP1->setID(kResMP1ID);
  addFocusWidget(myResMP1);

  // M1 reset
  myResButtons[3] = new ButtonWidget(boss, lfont, "RESM1", kResM1Cmd);
  myResButtons[3]->setCompact();
  myResButtons[3]->setTarget(this);
  addFocusWidget(myResButtons[3]);

  ////////////////////////////
  // BL register info
  ////////////////////////////
  myBlockLabels[4].name = new StaticTextWidget(boss, lfont, "BL");
  myBlockLabels[4].pos  = new StaticTextWidget(boss, lfont, "Pos#");
  myBlockLabels[4].hm   = new StaticTextWidget(boss, lfont, "HM");
  myBlockLabels[4].size = new StaticTextWidget(boss, lfont, "Size");

  // enaBL
  myEnaBL = new TogglePixelWidget(boss, nfont, 1, 1);
  myEnaBL->setTarget(this);
  myEnaBL->setID(kEnaBLID);
  myEnaBL->clearBackgroundColor();
  addFocusWidget(myEnaBL);

  // posBL
  myPosBL = new DataGridWidget(boss, nfont,
                               1, 1, 3, 8, Common::Base::Fmt::_10);
  myPosBL->setTarget(this);
  myPosBL->setID(kPosBLID);
  myPosBL->setRange(0, 160);
  addFocusWidget(myPosBL);

  // hmBL
  myHMBL = new DataGridWidget(boss, nfont,
                              1, 1, 1, 4, Common::Base::Fmt::_16_1);
  myHMBL->setTarget(this);
  myHMBL->setID(kHMBLID);
  addFocusWidget(myHMBL);

  // CTRLPF (size portion)
  mySizeBL = new DataGridWidget(boss, nfont,
                                1, 1, 1, 2, Common::Base::Fmt::_16_1);
  mySizeBL->setTarget(this);
  mySizeBL->setID(kSizeBLID);
  addFocusWidget(mySizeBL);

  // Reset ball
  myResButtons[4] = new ButtonWidget(boss, lfont, "RESBL", kResBLCmd);
  myResButtons[4]->setCompact();
  myResButtons[4]->setTarget(this);
  addFocusWidget(myResButtons[4]);

  // Ball (old)
  myEnaBLOld = new TogglePixelWidget(boss, nfont, 1, 1);
  myEnaBLOld->setTarget(this);
  myEnaBLOld->setID(kEnaBLOldID);
  myEnaBLOld->clearBackgroundColor();
  addFocusWidget(myEnaBLOld);

  // Ball delay
  myDelBL = new CheckboxWidget(boss, lfont,
                               "VDel", CheckboxWidget::kCheckActionCmd);
  myDelBL->setTarget(this);
  myDelBL->setID(kDelBLID);
  addFocusWidget(myDelBL);

  ////////////////////////////
  // PF 0/1/2 registers
  ////////////////////////////
  const GUI::Font& sf = instance().frameBuffer().smallFont();
  static constexpr std::array<string_view, 8> bitNames = {
    "0", "1", "2", "3", "4", "5", "6", "7"
  };

  myPFLabel = new StaticTextWidget(boss, lfont, "PF");

  // PF0 is four bits wide at double width; PF1 and PF2 are eight
  myPF[0] = new TogglePixelWidget(boss, nfont, 4, 1, 4);
  myPF[0]->setTarget(this);
  myPF[0]->setID(kPF0ID);
  addFocusWidget(myPF[0]);

  // PF1
  myPF[1] = new TogglePixelWidget(boss, nfont, 8, 1);
  myPF[1]->setTarget(this);
  myPF[1]->setID(kPF1ID);
  addFocusWidget(myPF[1]);

  // PF2
  myPF[2] = new TogglePixelWidget(boss, nfont, 8, 1);
  myPF[2]->setTarget(this);
  myPF[2]->setID(kPF2ID);
  addFocusWidget(myPF[2]);

  // PFx bit labels, in the small font: PF0 shows bits 4-7, PF1 counts down from
  // 7, PF2 counts up from 0 -- one label per bit box, in that order
  {
    int label = 0;
    for(int i = 4; i <= 7; ++i)
      myPFBitLabels[label++] = new StaticTextWidget(boss, sf, bitNames[i]);
    for(int i = 7; i >= 0; --i)
      myPFBitLabels[label++] = new StaticTextWidget(boss, sf, bitNames[i]);
    for(int i = 0; i <= 7; ++i)
      myPFBitLabels[label++] = new StaticTextWidget(boss, sf, bitNames[i]);
  }

  // PF reflect, score, priority
  myRefPF = new CheckboxWidget(boss, lfont,
                               "Reflect", CheckboxWidget::kCheckActionCmd);
  myRefPF->setTarget(this);
  myRefPF->setID(kRefPFID);
  addFocusWidget(myRefPF);

  myScorePF = new CheckboxWidget(boss, lfont,
                                 "Score", CheckboxWidget::kCheckActionCmd);
  myScorePF->setTarget(this);
  myScorePF->setID(kScorePFID);
  addFocusWidget(myScorePF);

  myPriorityPF = new CheckboxWidget(boss, lfont,
                                    "Priority", CheckboxWidget::kCheckActionCmd);
  myPriorityPF->setTarget(this);
  myPriorityPF->setID(kPriorityPFID);
  addFocusWidget(myPriorityPF);

  myQueuedWritesLabel = new StaticTextWidget(boss, lfont, "Queued Writes");
  myDelayQueueWidget = new DelayQueueWidget(boss, lfont);

  ////////////////////////////
  // Strobe buttons
  ////////////////////////////
  static constexpr std::array<string_view, 4> strobeNames = {
    "WSYNC", "RSYNC", "HMOVE", "HMCLR"
  };
  static constexpr std::array<int, 4> strobeCmds = {
    kWsyncCmd, kRsyncCmd, kHmoveCmd, kHmclrCmd
  };
  for(int i = 0; i < 4; ++i)
  {
    myStrobeButtons[i] = new ButtonWidget(boss, lfont,
                                          strobeNames[i], strobeCmds[i]);
    myStrobeButtons[i]->setCompact();
    myStrobeButtons[i]->setTarget(this);
    addFocusWidget(myStrobeButtons[i]);
  }

  setHelpAnchor("TIATab", true);

  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaWidget::setArea(int x, int y, int w, int h)
{
  Widget::setArea(x, y, w, h);
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size TiaWidget::naturalSize() const
{
  return buildLayout()->naturalSize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaWidget::reflow()
{
  buildLayout()->doLayout(_x, _y, _w, _h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<GUI::Layout> TiaWidget::buildLayout() const
{
  using GUI::BoxLayout;
  using GUI::GridLayout;
  using GUI::anchoredItem;
  using GUI::alignedItem;
  using GUI::centeredItem;
  using GUI::HAlign;
  using GUI::VAlign;
  using Dir = BoxLayout::Dir;

  const int lineHeight = _font.getLineHeight(),
            HGAP    = _fontWidth,
            // What separates a label from the grid it belongs to: enough to
            // clear the frame, but far less than what separates the groups
            LBLGAP  = HGAP / 2,
            VGAP    = _fontHeight / 2,
            HBORDER = static_cast<int>(_fontWidth * 1.25),
            VBORDER = _fontHeight / 2;

  // Every button in the tab takes the widest label's width
  GUI::alignButtons({myCxclrButton, myResButtons[0], myResButtons[1],
                     myResButtons[2], myResButtons[3], myResButtons[4],
                     myStrobeButtons[0], myStrobeButtons[1],
                     myStrobeButtons[2], myStrobeButtons[3]});

  // A control that frames its text sits beside a label on the label's own line
  const auto onBaseline = [](Widget* wid) {
    return alignedItem(wid, HAlign::Left, VAlign::Baseline);
  };
  // A register row is a line of single-line parts of differing heights -- a
  // grid, a checkbox, a button.  They agree on the row's MIDDLE: a baseline
  // would line their text up and leave the frames around it stepped, since a
  // grid reports its text near its top and everything else reports its centre
  const auto onRow = [](Widget* wid) {
    return alignedItem(wid, HAlign::Left, VAlign::Center);
  };
  // A colour swatch is half again as wide as it is tall
  const auto swatchW = static_cast<int>(1.5 * lineHeight);

  ////////////////////////////////////////////////////////////////////
  // Top band: colour registers | fixed debug colours | collisions
  ////////////////////////////////////////////////////////////////////
  // The four COLUPx rows: labels, the shared 4-row grid, then the swatches.
  // Each side column is built at the grid's row pitch so row i lines up with
  // grid row i (the grid insets its text where a label centres its own)
  const auto gridColumn = [&](auto&& cells, int cellHeight) {
    auto col = std::make_unique<BoxLayout>(Dir::Vertical);
    col->addSpace(myColorRegs->firstTextY() - cells[0]->firstTextY());
    for(auto* wid: cells)
      col->addFixed(alignedItem(wid, HAlign::Left, VAlign::Center), cellHeight);
    return col;
  };

  const std::array<ColorWidget*, 4> colorSwatches{
    myCOLUP0Color, myCOLUP1Color, myCOLUPFColor, myCOLUBKColor
  };
  auto swatchCol = std::make_unique<BoxLayout>(Dir::Vertical);
  swatchCol->addSpace(myColorRegs->firstTextY() - 2);
  for(auto* c: colorSwatches)
    swatchCol->addFixed(alignedItem(c, HAlign::Fill, VAlign::Center), lineHeight);

  // The row this sits in takes up the collisions' slack, so it is taller than
  // its content: everything in it hangs from the TOP, or the grid would centre
  // itself in that height and drop below the label and swatch columns beside it
  auto colorRow = std::make_unique<BoxLayout>(Dir::Horizontal, HGAP);
  colorRow->addAuto(gridColumn(myColorRegLabels, lineHeight));
  colorRow->addAuto(alignedItem(myColorRegs, HAlign::Left, VAlign::Top));
  colorRow->addFixed(std::move(swatchCol), swatchW);

  auto vsyncRow = std::make_unique<BoxLayout>(Dir::Horizontal, HGAP * 2);
  vsyncRow->addAuto(anchoredItem(myVSync));
  vsyncRow->addAuto(anchoredItem(myVBlank));

  // The eight fixed debug colours, in two columns of four
  const auto dbgColumn = [&](int first) {
    auto col = std::make_unique<BoxLayout>(Dir::Vertical);
    for(int i = first; i < first + 4; ++i)
    {
      auto row = std::make_unique<BoxLayout>(Dir::Horizontal, HGAP);
      row->addAuto(alignedItem(myDbgColorLabels[i], HAlign::Left, VAlign::Center));
      row->addFixed(alignedItem(myFixedColors[i], HAlign::Fill, VAlign::Center),
                    swatchW);
      col->addFixed(std::move(row), lineHeight);
    }
    return col;
  };

  auto dbgRow = std::make_unique<BoxLayout>(Dir::Horizontal, HGAP * 2);
  dbgRow->addAuto(dbgColumn(0));
  dbgRow->addAuto(dbgColumn(4));

  // The collision triangle: row r holds 5 - r bits, labelled down the left and
  // along the top, with CXCLR parked in the empty corner it leaves.  The row
  // pitch is a constant, not a font height: the checkboxes are one fixed size
  auto collGrid = std::make_unique<GridLayout>(6, 6, HGAP, 0);
  collGrid->columnAuto(0);
  for(int col = 1; col <= 5; ++col)
    collGrid->columnAuto(col);
  collGrid->rowAuto(0);
  for(int row = 1; row <= 5; ++row)
    collGrid->rowFixed(row, lineHeight + 3);

  int idx = 0;
  for(int row = 0; row < 5; ++row)
  {
    collGrid->place(0, row + 1,
                    alignedItem(myCollRowLabels[row], HAlign::Left, VAlign::Center));
    for(int col = 0; col < 5 - row; ++col, ++idx)
    {
      if(row == 0)
        collGrid->place(col + 1, 0, centeredItem(myCollColLabels[col]));
      collGrid->place(col + 1, row + 1, centeredItem(myCollision[idx]));
    }
  }
  // Parked in the corner the triangle leaves empty: it lies across the columns
  // the last row does not use, and sits in them the way a bit would -- ending
  // with the last column, and centred on the row rather than hanging below it
  collGrid->place(2, 5, alignedItem(myCxclrButton, HAlign::Right, VAlign::Center), 4);

  // The band is three columns staggered against ONE set of rows: VSync/VBlank
  // own the first line, the Debug Colors switch the second, and the two colour
  // blocks start together on the third, with the collisions running down the
  // whole height beside them.
  // VSync/VBlank LIE ACROSS the colour and debug columns rather than sitting in
  // the first: that row is wider than the COLUPx rows below it, and a column
  // sized by it would push the debug block (and the collisions after it) right.
  // The collisions are taller than the three rows come to, so the row holding
  // the colour blocks takes up that slack -- it is the one with room below its
  // content, so growing it moves nothing
  enum TCol: uInt8 { COLOURS, TGAP0, DEBUG, TGAP1, COLLS, TSLACK, T_COLS };
  enum TRow: uInt8 { VSYNC_LN, DBGSW_LN, BLOCKS_LN, T_ROWS };

  auto topBand = std::make_unique<GridLayout>(T_COLS, T_ROWS, 0, 0);
  topBand->columnAuto(COLOURS).columnAuto(DEBUG).columnAuto(COLLS);
  topBand->columnFixed(TGAP0, HGAP * 4).columnFixed(TGAP1, HGAP * 4);
  topBand->columnStretch(TSLACK);
  topBand->rowAuto(VSYNC_LN).rowAuto(DBGSW_LN).rowStretch(BLOCKS_LN);

  topBand->place(COLOURS, VSYNC_LN, std::move(vsyncRow), DEBUG - COLOURS + 1);
  topBand->place(DEBUG, DBGSW_LN, anchoredItem(myFixedEnabled));
  topBand->place(COLOURS, BLOCKS_LN, std::move(colorRow));
  topBand->place(DEBUG, BLOCKS_LN, std::move(dbgRow));
  topBand->place(COLLS, VSYNC_LN, std::move(collGrid), 1, T_ROWS);

  ////////////////////////////////////////////////////////////////////
  // The register table: every block over one set of columns, which is what
  // lines the five RESxx buttons up without anyone tracking an x position
  ////////////////////////////////////////////////////////////////////
  // A missile or ball row is far more compact than a player's, so it takes one
  // cell that LIES ACROSS the player columns instead of sitting in them -- put
  // every field in a shared column and the compact rows stretch to the players'
  // widths.  OVERRUN holds nothing of its own: it is the flexible track that
  // takes the NuSiz readout's overhang, so a readout wider than the block
  // cannot push the button column along with it
  enum RCol: uInt8 {
    NAME, GAP1, TOGGLE, GAP2, POSLBL, LGAP1, POS, GAP3, HMLBL, LGAP2, HM,
    GAP4, EXTRA, GAP5, BTN, OVERRUN, R_COLS
  };
  // A player's two rows belong together, so the gap that separates one register
  // block from the next is a row of its own -- the same way the gaps between
  // the register groups across a row are columns of their own.  The missile and
  // ball rows read as one block, so nothing separates them
  enum RRow: uInt8 {
    P0_A, P0_B, RGAP0, P1_A, P1_B, RGAP1, M0_R, M1_R, BL_A, BL_B, R_ROWS
  };

  auto regs = std::make_unique<GridLayout>(R_COLS, R_ROWS, 0, VGAP / 2);

  // The content columns are sized by what is in them and carry no gap of their
  // own; every gap is a column that says which one it is.  A label sits a short
  // LBLGAP from its own grid, so it reads as belonging to it, while the wider
  // gaps keep the register groups apart
  regs->columnAuto(NAME).columnAuto(TOGGLE);
  regs->columnAuto(POSLBL).columnAuto(POS);
  regs->columnAuto(HMLBL).columnAuto(HM);
  regs->columnAuto(EXTRA).columnAuto(BTN);
  regs->columnFixed(LGAP1, LBLGAP).columnFixed(LGAP2, LBLGAP);
  regs->columnFixed(GAP1, HGAP).columnFixed(GAP2, HGAP).columnFixed(GAP3, HGAP);
  regs->columnFixed(GAP4, HGAP * 2).columnFixed(GAP5, HGAP * 2);
  regs->columnStretch(OVERRUN);
  for(int row = 0; row < R_ROWS; ++row)
    regs->rowAuto(row);
  regs->rowFixed(RGAP0, VGAP / 2).rowFixed(RGAP1, VGAP / 2);

  // A player is two rows over ONE set of columns: that is what puts the delayed
  // graphic under the live one, and NuSiz under the motion register
  const auto playerBlock = [&](int block, int row, TogglePixelWidget* grp,
                               TogglePixelWidget* grpOld, DataGridWidget* pos,
                               DataGridWidget* hm, CheckboxWidget* ref,
                               CheckboxWidget* del, DataGridWidget* nusiz,
                               EditTextWidget* nusizText) {
    const BlockLabels& lbl = myBlockLabels[block];

    regs->place(NAME,   row, onRow(lbl.name));
    regs->place(TOGGLE, row, onRow(grp));
    regs->place(POSLBL, row, onRow(lbl.pos));
    regs->place(POS,    row, onRow(pos));
    regs->place(HMLBL,  row, alignedItem(lbl.hm, HAlign::Right, VAlign::Center));
    regs->place(HM,     row, onRow(hm));
    regs->place(EXTRA,  row, onRow(ref));
    regs->place(BTN,    row, onRow(myResButtons[block]));

    // The readout spells a NUSIZ setting out, so it is as wide as the longest
    // one.  It belongs to the grid beside it, so it starts from the gap column
    // rather than the checkbox one -- that keeps it near its grid without
    // touching the column the checkboxes above and below it are sized by.
    // Spanning it out to OVERRUN lets it run past the button as it always has
    auto textCell = std::make_unique<BoxLayout>(Dir::Horizontal);
    textCell->addSpace(LBLGAP);
    textCell->addFixed(alignedItem(nusizText, HAlign::Fill, VAlign::Center),
                       EditTextWidget::calcWidth(_font, 21));

    regs->place(TOGGLE, row + 1, onRow(grpOld));
    regs->place(POSLBL, row + 1, onRow(del), POS - POSLBL + 1);
    regs->place(HMLBL,  row + 1, alignedItem(lbl.size, HAlign::Right, VAlign::Center));
    regs->place(HM,     row + 1, onRow(nusiz));
    regs->place(GAP4,   row + 1, std::move(textCell), OVERRUN - GAP4 + 1);
  };
  playerBlock(0, P0_A, myGRP0, myGRP0Old, myPosP0, myHMP0, myRefP0,
              myDelP0, myNusizP0, myNusizP0Text);
  playerBlock(1, P1_A, myGRP1, myGRP1Old, myPosP1, myHMP1, myRefP1,
              myDelP1, myNusizP1, myNusizP1Text);

  // A missile or the ball: enable bit, position, motion, size.  The three are
  // built alike from equal-sized parts, so they line up with each other by
  // construction; only their name and their button share the players' columns
  const auto objectBlock = [&](int block, int row, TogglePixelWidget* ena,
                               DataGridWidget* pos, DataGridWidget* hm,
                               DataGridWidget* size, CheckboxWidget* extra,
                               TogglePixelWidget* enaOld, CheckboxWidget* del) {
    const BlockLabels& lbl = myBlockLabels[block];

    // Same rule as the shared columns: a label abuts its own grid, and the gaps
    // fall between the register groups
    auto rest = std::make_unique<BoxLayout>(Dir::Horizontal);
    rest->addAuto(onRow(ena));
    rest->addSpace(HGAP);
    rest->addAuto(onRow(lbl.pos));
    rest->addSpace(LBLGAP);
    rest->addAuto(onRow(pos));
    rest->addSpace(HGAP);
    rest->addAuto(onRow(lbl.hm));
    rest->addSpace(LBLGAP);
    rest->addAuto(onRow(hm));
    rest->addSpace(HGAP);
    rest->addAuto(onRow(lbl.size));
    rest->addSpace(LBLGAP);
    rest->addAuto(onRow(size));
    if(extra != nullptr)
    {
      rest->addSpace(HGAP * 2);
      rest->addAuto(onRow(extra));
    }

    regs->place(NAME,   row, onRow(lbl.name));
    regs->place(TOGGLE, row, std::move(rest), EXTRA - TOGGLE + 1);
    regs->place(BTN,    row, onRow(myResButtons[block]));

    // The ball's delayed bit starts where the live one does, so the second row
    // spans from the same column
    if(enaOld != nullptr)
    {
      auto delayed = std::make_unique<BoxLayout>(Dir::Horizontal, HGAP);
      delayed->addAuto(onRow(enaOld));
      delayed->addAuto(onRow(del));
      regs->place(TOGGLE, row + 1, std::move(delayed), EXTRA - TOGGLE + 1);
    }
  };
  objectBlock(2, M0_R, myEnaM0, myPosM0, myHMM0, myNusizM0,
              myResMP0, nullptr, nullptr);
  objectBlock(3, M1_R, myEnaM1, myPosM1, myHMM1, myNusizM1,
              myResMP1, nullptr, nullptr);
  objectBlock(4, BL_A, myEnaBL, myPosBL, myHMBL, mySizeBL,
              nullptr, myEnaBLOld, myDelBL);

  ////////////////////////////////////////////////////////////////////
  // Playfield
  ////////////////////////////////////////////////////////////////////
  // One column per playfield BIT: the bit number goes in it, and each toggle
  // SPANS the columns of its own group.  Sharing the columns is what keeps a
  // number over the box it names -- the alternative, sizing the number's cell
  // to the toggle's bit pitch, only lines up while the two agree, and drifts a
  // box at a time when they do not.  PF0 has four bits, PF1 and PF2 eight
  static constexpr int PF_NAME = 0, PF_GAP0 = 1,
                       PF_G0 = 2,           PF_GAP1 = PF_G0 + 4,
                       PF_G1 = PF_GAP1 + 1, PF_GAP2 = PF_G1 + 8,
                       PF_G2 = PF_GAP2 + 1, PF_COLS = PF_G2 + 8;
  static constexpr std::array<int, 3> pfFirst = { PF_G0, PF_G1, PF_G2 };
  static constexpr std::array<int, 3> pfCount = { 4, 8, 8 };

  // The numbers hang directly on the boxes, so the block carries no row spacing
  auto pfBlock = std::make_unique<GridLayout>(PF_COLS, 3, 0, 0);
  for(int col = 0; col < PF_COLS; ++col)
    pfBlock->columnAuto(col);
  pfBlock->columnFixed(PF_GAP0, HGAP);
  pfBlock->columnFixed(PF_GAP1, HGAP / 2).columnFixed(PF_GAP2, HGAP / 2);
  pfBlock->rowAuto(0).rowAuto(1).rowAuto(2);

  {
    int label = 0;
    for(int reg = 0; reg < 3; ++reg)
    {
      for(int bit = 0; bit < pfCount[reg]; ++bit)
        pfBlock->place(pfFirst[reg] + bit, 0,
                       centeredItem(myPFBitLabels[label++]));
      pfBlock->place(pfFirst[reg], 1, anchoredItem(myPF[reg]), pfCount[reg]);
    }
  }
  pfBlock->place(PF_NAME, 1, onBaseline(myPFLabel));

  auto pfFlags = std::make_unique<BoxLayout>(Dir::Horizontal, HGAP * 2);
  pfFlags->addAuto(anchoredItem(myRefPF));
  pfFlags->addAuto(anchoredItem(myScorePF));
  pfFlags->addAuto(anchoredItem(myPriorityPF));

  // The numbers hang directly over the boxes, so the block itself carries no
  // row spacing; the flags are a group of their own and want the usual gap
  auto pfFlagsRow = std::make_unique<BoxLayout>(Dir::Vertical);
  pfFlagsRow->addSpace(VGAP);
  pfFlagsRow->addAuto(std::move(pfFlags));
  pfBlock->place(PF_G0, 2, std::move(pfFlagsRow), PF_COLS - PF_G0);

  ////////////////////////////////////////////////////////////////////
  // Queued writes, with the strobe buttons beside them
  ////////////////////////////////////////////////////////////////////
  auto strobes = std::make_unique<GridLayout>(2, 2, HGAP * 2, VGAP);
  strobes->columnAuto(0).columnAuto(1).rowAuto(0).rowAuto(1);
  strobes->place(0, 0, anchoredItem(myStrobeButtons[0]));
  strobes->place(0, 1, anchoredItem(myStrobeButtons[1]));
  strobes->place(1, 0, anchoredItem(myStrobeButtons[2]));
  strobes->place(1, 1, anchoredItem(myStrobeButtons[3]));

  auto queueRow = std::make_unique<BoxLayout>(Dir::Horizontal, HGAP);
  queueRow->addAuto(onBaseline(myQueuedWritesLabel));
  queueRow->addAuto(anchoredItem(myDelayQueueWidget));
  queueRow->addSpace(HGAP * 2);
  queueRow->addAuto(std::move(strobes));

  ////////////////////////////////////////////////////////////////////
  auto root = std::make_unique<BoxLayout>(Dir::Vertical, VGAP, HBORDER, VBORDER);
  root->addAuto(std::move(topBand));
  root->addAuto(std::move(regs));
  // The playfield follows the registers directly: its bit numbers are a line of
  // their own, which already sets it apart from the row above
  root->addAuto(std::move(pfBlock));
  root->addAuto(std::move(queueRow));
  root->addStretchSpace();

  return root;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  TIADebug& tia = instance().debugger().tiaDebug();

  switch(cmd)
  {
    case kWsyncCmd:
      tia.strobeWsync();
      break;

    case kRsyncCmd:
      tia.strobeRsync();
      break;

    case kResP0Cmd:
      tia.strobeResP0();
      break;

    case kResP1Cmd:
      tia.strobeResP1();
      break;

    case kResM0Cmd:
      tia.strobeResM0();
      break;

    case kResM1Cmd:
      tia.strobeResM1();
      break;

    case kResBLCmd:
      tia.strobeResBL();
      break;

    case kHmoveCmd:
      tia.strobeHmove();
      break;

    case kHmclrCmd:
      tia.strobeHmclr();
      break;

    case kCxclrCmd:
      tia.strobeCxclr();
      break;

    case kDbgClCmd:
      myFixedEnabled->setState(tia.tia().toggleFixedColors());
      break;

    case kVSyncCmd:
      tia.vsync((tia.vsyncAsInt() & ~0x02) | (myVSync->getState() ? 0x02 : 0x00));
      break;

    case kVBlankCmd:
      tia.vblank((tia.vblankAsInt() & ~0x02) | (myVBlank->getState() ? 0x02 : 0x00));
      break;

    case DataGridWidget::kItemDataChangedCmd:
      switch(id)
      {
        case kColorRegsID:
          changeColorRegs();
          break;

        case kPosP0ID:
          tia.posP0(myPosP0->getSelectedValue());
          break;

        case kPosP1ID:
          tia.posP1(myPosP1->getSelectedValue());
          break;

        case kPosM0ID:
          tia.posM0(myPosM0->getSelectedValue());
          break;

        case kPosM1ID:
          tia.posM1(myPosM1->getSelectedValue());
          break;

        case kPosBLID:
          tia.posBL(myPosBL->getSelectedValue());
          break;

        case kHMP0ID:
          tia.hmP0(myHMP0->getSelectedValue());
          break;

        case kHMP1ID:
          tia.hmP1(myHMP1->getSelectedValue());
          break;

        case kHMM0ID:
          tia.hmM0(myHMM0->getSelectedValue());
          break;

        case kHMM1ID:
          tia.hmM1(myHMM1->getSelectedValue());
          break;

        case kHMBLID:
          tia.hmBL(myHMBL->getSelectedValue());
          break;

        case kNusizP0ID:
          tia.nusizP0(myNusizP0->getSelectedValue());
          myNusizP0Text->setText(tia.nusizP0String());
          break;

        case kNusizP1ID:
          tia.nusizP1(myNusizP1->getSelectedValue());
          myNusizP1Text->setText(tia.nusizP1String());
          break;

        case kNusizM0ID:
          tia.nusizM0(myNusizM0->getSelectedValue());
          break;

        case kNusizM1ID:
          tia.nusizM1(myNusizM1->getSelectedValue());
          break;

        case kSizeBLID:
          tia.sizeBL(mySizeBL->getSelectedValue());
          break;

        default:
          cerr << "TiaWidget DG changed\n";
          break;
      }
      break;

    case ToggleWidget::kItemDataChangedCmd:
      switch(id)
      {
        case kGRP0ID:
          tia.grP0(myGRP0->getIntState());
          break;

        case kGRP0OldID:
          tia.setGRP0Old(myGRP0Old->getIntState());
          break;

        case kGRP1ID:
          tia.grP1(myGRP1->getIntState());
          break;

        case kGRP1OldID:
          tia.setGRP1Old(myGRP1Old->getIntState());
          break;

        case kEnaM0ID:
          tia.enaM0(myEnaM0->getIntState());
          break;

        case kEnaM1ID:
          tia.enaM1(myEnaM1->getIntState());
          break;

        case kEnaBLID:
          tia.enaBL(myEnaBL->getIntState());
          break;

        case kEnaBLOldID:
          tia.setENABLOld(myEnaBLOld->getIntState() != 0);
          break;

        case kPF0ID:
          tia.pf0(myPF[0]->getIntState());
          break;

        case kPF1ID:
          tia.pf1(myPF[1]->getIntState());
          break;

        case kPF2ID:
          tia.pf2(myPF[2]->getIntState());
          break;

        default:
          break;
      }
      break;

    case CheckboxWidget::kCheckActionCmd:
      switch(id)
      {
        case kP0_PFID:
          tia.collision(CollisionBit::P0PF, true);
          break;

        case kP0_BLID:
          tia.collision(CollisionBit::P0BL, true);
          break;

        case kP0_M1ID:
          tia.collision(CollisionBit::M1P0, true);
          break;

        case kP0_M0ID:
          tia.collision(CollisionBit::M0P0, true);
          break;

        case kP0_P1ID:
          tia.collision(CollisionBit::P0P1, true);
          break;

        case kP1_PFID:
          tia.collision(CollisionBit::P1PF, true);
          break;
        case kP1_BLID:
          tia.collision(CollisionBit::P1BL, true);
          break;

        case kP1_M1ID:
          tia.collision(CollisionBit::M1P1, true);
          break;
        case kP1_M0ID:
          tia.collision(CollisionBit::M0P1, true);
          break;

        case kM0_PFID:
          tia.collision(CollisionBit::M0PF, true);
          break;

        case kM0_BLID:
          tia.collision(CollisionBit::M0BL, true);
          break;

        case kM0_M1ID:
          tia.collision(CollisionBit::M0M1, true);
          break;

        case kM1_PFID:
          tia.collision(CollisionBit::M1PF, true);
          break;

        case kM1_BLID:
          tia.collision(CollisionBit::M1BL, true);
          break;

        case kBL_PFID:
          tia.collision(CollisionBit::BLPF, true);
          break;

        case kRefP0ID:
          tia.refP0(myRefP0->getState() ? 1 : 0);
          break;

        case kRefP1ID:
          tia.refP1(myRefP1->getState() ? 1 : 0);
          break;

        case kDelP0ID:
          tia.vdelP0(myDelP0->getState() ? 1 : 0);
          break;

        case kDelP1ID:
          tia.vdelP1(myDelP1->getState() ? 1 : 0);
          break;

        case kDelBLID:
          tia.vdelBL(myDelBL->getState() ? 1 : 0);
          break;

        case kResMP0ID:
          tia.resMP0(myResMP0->getState() ? 1 : 0);
          break;

        case kResMP1ID:
          tia.resMP1(myResMP1->getState() ? 1 : 0);
          break;

        case kRefPFID:
          tia.refPF(myRefPF->getState() ? 1 : 0);
          break;

        case kScorePFID:
          tia.scorePF(myScorePF->getState() ? 1 : 0);
          break;

        case kPriorityPFID:
          tia.priorityPF(myPriorityPF->getState() ? 1 : 0);
          break;

        default:
          break;
      }
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaWidget::loadConfig()
{
  IntArray alist;
  IntArray vlist;
  BoolArray changed;

  TIADebug& tia = instance().debugger().tiaDebug();
  const auto& state    = static_cast<const TiaState&>(tia.getState());
  const auto& oldstate = static_cast<const TiaState&>(tia.getOldState());

  // Color registers
  alist.clear();  vlist.clear();  changed.clear();
  for(uInt32 i = 0; i < 4; ++i)
  {
    alist.push_back(i);
    vlist.push_back(state.coluRegs[i]);
    changed.push_back(state.coluRegs[i] != oldstate.coluRegs[i]);
  }
  myColorRegs->setList(alist, vlist, changed);

  const bool fixed = tia.tia().usingFixedColors();

  myCOLUP0Color->setColor(state.coluRegs[0]);
  myCOLUP1Color->setColor(state.coluRegs[1]);
  myCOLUPFColor->setColor(state.coluRegs[2]);
  myCOLUBKColor->setColor(state.coluRegs[3]);
  myCOLUP0Color->setCrossed(fixed);
  myCOLUP1Color->setCrossed(fixed);
  myCOLUPFColor->setCrossed(fixed);
  myCOLUBKColor->setCrossed(fixed);

  // Fixed debug colors
  myFixedEnabled->setState(fixed);
  for(uInt32 c = 0; c < 8; ++c)
  {
    myFixedColors[c]->setColor(state.fixedCols[c]);
    myFixedColors[c]->setCrossed(!fixed);
  }

  ////////////////////////////
  // Collision register bits
  ////////////////////////////
  myCollision[kP0_PFID]->setState(tia.collP0_PF(), state.cx[0] != oldstate.cx[0]);
  myCollision[kP0_BLID]->setState(tia.collP0_BL(), state.cx[1] != oldstate.cx[1]);
  myCollision[kP0_M1ID]->setState(tia.collM1_P0(), state.cx[2] != oldstate.cx[2]);
  myCollision[kP0_M0ID]->setState(tia.collM0_P0(), state.cx[3] != oldstate.cx[3]);
  myCollision[kP0_P1ID]->setState(tia.collP0_P1(), state.cx[4] != oldstate.cx[4]);
  myCollision[kP1_PFID]->setState(tia.collP1_PF(), state.cx[5] != oldstate.cx[5]);
  myCollision[kP1_BLID]->setState(tia.collP1_BL(), state.cx[6] != oldstate.cx[6]);
  myCollision[kP1_M1ID]->setState(tia.collM1_P1(), state.cx[7] != oldstate.cx[7]);
  myCollision[kP1_M0ID]->setState(tia.collM0_P1(), state.cx[8] != oldstate.cx[8]);
  myCollision[kM0_PFID]->setState(tia.collM0_PF(), state.cx[9] != oldstate.cx[9]);
  myCollision[kM0_BLID]->setState(tia.collM0_BL(), state.cx[10] != oldstate.cx[10]);
  myCollision[kM0_M1ID]->setState(tia.collM0_M1(), state.cx[11] != oldstate.cx[11]);
  myCollision[kM1_PFID]->setState(tia.collM1_PF(), state.cx[12] != oldstate.cx[12]);
  myCollision[kM1_BLID]->setState(tia.collM1_BL(), state.cx[13] != oldstate.cx[13]);
  myCollision[kBL_PFID]->setState(tia.collBL_PF(), state.cx[14] != oldstate.cx[14]);

  ////////////////////////////
  // P0 register info
  ////////////////////////////
  // grP0 (new and old)
  if(tia.vdelP0())
  {
    myGRP0->setColor(kBGColorLo);
    myGRP0Old->setColor(state.coluRegs[0]);
    myGRP0Old->setCrossed(false);
  }
  else
  {
    myGRP0->setColor(state.coluRegs[0]);
    myGRP0Old->setColor(kBGColorLo);
    myGRP0Old->setCrossed(true);
  }
  myGRP0->setIntState(state.gr[TiaState::P0], state.ref[TiaState::P0]);
  myGRP0Old->setIntState(state.gr[TiaState::P0 + 2], state.ref[TiaState::P0]);

  // posP0
  myPosP0->setList(0, state.pos[TiaState::P0],
      state.pos[TiaState::P0] != oldstate.pos[TiaState::P0]);

  // hmP0
  myHMP0->setList(0, state.hm[TiaState::P0],
      state.hm[TiaState::P0] != oldstate.hm[TiaState::P0]);

  // refP0 & vdelP0
  myRefP0->setState(tia.refP0(), state.ref[TiaState::P0] != oldstate.ref[TiaState::P0]);
  myDelP0->setState(tia.vdelP0(), state.vdel[TiaState::P0] != oldstate.vdel[TiaState::P0]);

  // NUSIZ0 (player portion)
  const bool nusiz0changed = state.size[TiaState::P0] != oldstate.size[TiaState::P0];
  myNusizP0->setList(0, state.size[TiaState::P0], nusiz0changed);
  myNusizP0Text->setText(tia.nusizP0String(), nusiz0changed);

  ////////////////////////////
  // P1 register info
  ////////////////////////////
  // grP1 (new and old)
  if(tia.vdelP1())
  {
    myGRP1->setColor(kBGColorLo);
    myGRP1Old->setColor(state.coluRegs[1]);
    myGRP1Old->setCrossed(false);
  }
  else
  {
    myGRP1->setColor(state.coluRegs[1]);
    myGRP1Old->setColor(kBGColorLo);
    myGRP1Old->setCrossed(true);
  }
  myGRP1->setIntState(state.gr[TiaState::P1], state.ref[TiaState::P1]);
  myGRP1Old->setIntState(state.gr[TiaState::P1 + 2], state.ref[TiaState::P1]);

  // posP1
  myPosP1->setList(0, state.pos[TiaState::P1],
      state.pos[TiaState::P1] != oldstate.pos[TiaState::P1]);

  // hmP1
  myHMP1->setList(0, state.hm[TiaState::P1],
      state.hm[TiaState::P1] != oldstate.hm[TiaState::P1]);

  // refP1 & vdelP1
  myRefP1->setState(tia.refP1(), state.ref[TiaState::P1] != oldstate.ref[TiaState::P1]);
  myDelP1->setState(tia.vdelP1(), state.vdel[TiaState::P1] != oldstate.vdel[TiaState::P1]);

  // NUSIZ1 (player portion)
  const bool nusiz1changed = state.size[TiaState::P1] != oldstate.size[TiaState::P1];
  myNusizP1->setList(0, state.size[TiaState::P1], nusiz1changed);
  myNusizP1Text->setText(tia.nusizP1String(), nusiz1changed);

  ////////////////////////////
  // M0 register info
  ////////////////////////////
  // enaM0
  myEnaM0->setColor(tia.resMP0() ? kBGColorLo : state.coluRegs[0]);
  myEnaM0->setCrossed(tia.resMP0());
  myEnaM0->setIntState(tia.enaM0() ? 1 : 0, false);

  // posM0
  myPosM0->setList(0, state.pos[TiaState::M0],
      state.pos[TiaState::M0] != oldstate.pos[TiaState::M0]);

  // hmM0
  myHMM0->setList(0, state.hm[TiaState::M0],
      state.hm[TiaState::M0] != oldstate.hm[TiaState::M0]);

  // NUSIZ0 (missile portion)
  myNusizM0->setList(0, state.size[TiaState::M0],
      state.size[TiaState::M0] != oldstate.size[TiaState::M0]);

  // resMP0
  myResMP0->setState(tia.resMP0(), state.resm[TiaState::P0] != oldstate.resm[TiaState::P0]);

  ////////////////////////////
  // M1 register info
  ////////////////////////////
  // enaM1
  myEnaM1->setColor(tia.resMP1() ? kBGColorLo : state.coluRegs[1]);
  myEnaM1->setCrossed(tia.resMP1());
  myEnaM1->setIntState(tia.enaM1() ? 1 : 0, false);

  // posM1
  myPosM1->setList(0, state.pos[TiaState::M1],
      state.pos[TiaState::M1] != oldstate.pos[TiaState::M1]);

  // hmM1
  myHMM1->setList(0, state.hm[TiaState::M1],
      state.hm[TiaState::M1] != oldstate.hm[TiaState::M1]);

  // NUSIZ1 (missile portion)
  myNusizM1->setList(0, state.size[TiaState::M1],
      state.size[TiaState::M1] != oldstate.size[TiaState::M1]);

  // resMP1
  myResMP1->setState(tia.resMP1(),state.resm[TiaState::P1] != oldstate.resm[TiaState::P1]);

  ////////////////////////////
  // BL register info
  ////////////////////////////
  // enaBL (new and old)
  if(tia.vdelBL())
  {
    myEnaBL->setColor(kBGColorLo);
    myEnaBLOld->setColor(state.coluRegs[2]);
    myEnaBLOld->setCrossed(false);
  }
  else
  {
    myEnaBL->setColor(state.coluRegs[2]);
    myEnaBLOld->setColor(kBGColorLo);
    myEnaBLOld->setCrossed(true);
  }
  myEnaBL->setIntState(state.gr[4], false);
  myEnaBLOld->setIntState(state.gr[5], false);

  // posBL
  myPosBL->setList(0, state.pos[TiaState::BL],
      state.pos[TiaState::BL] != oldstate.pos[TiaState::BL]);

  // hmBL
  myHMBL->setList(0, state.hm[TiaState::BL],
      state.hm[TiaState::BL] != oldstate.hm[TiaState::BL]);

  // CTRLPF (size portion)
  mySizeBL->setList(0, state.size[TiaState::BL],
      state.size[TiaState::BL] != oldstate.size[TiaState::BL]);

  // vdelBL
  myDelBL->setState(tia.vdelBL(), state.vdel[2] != oldstate.vdel[2]);

  ////////////////////////////
  // PF register info
  ////////////////////////////
  // PF0
  myPF[0]->setColor(state.coluRegs[2]);
  myPF[0]->setIntState(state.pf[0], true);  // reverse bit order

  // PF1
  myPF[1]->setColor(state.coluRegs[2]);
  myPF[1]->setIntState(state.pf[1], false);

  // PF2
  myPF[2]->setColor(state.coluRegs[2]);
  myPF[2]->setIntState(state.pf[2], true);  // reverse bit order

  // Reflect
  myRefPF->setState(tia.refPF(), state.pf[3] != oldstate.pf[3]);

  // Score
  myScorePF->setState(tia.scorePF(), state.pf[4] != oldstate.pf[4]);

  // Priority
  myPriorityPF->setState(tia.priorityPF(), state.pf[5] != oldstate.pf[5]);

  myDelayQueueWidget->loadConfig();

  myVSync->setState(tia.vsync(), tia.vsync() != oldstate.vsb[0]);
  myVBlank->setState(tia.vblank(), tia.vblank() != oldstate.vsb[1]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaWidget::changeColorRegs()
{
  const int addr  = myColorRegs->getSelectedAddr();
  const int value = myColorRegs->getSelectedValue();

  switch(addr)
  {
    case kCOLUP0Addr:
      instance().debugger().tiaDebug().coluP0(value);
      myCOLUP0Color->setColor(value);
      break;

    case kCOLUP1Addr:
      instance().debugger().tiaDebug().coluP1(value);
      myCOLUP1Color->setColor(value);
      break;

    case kCOLUPFAddr:
      instance().debugger().tiaDebug().coluPF(value);
      myCOLUPFColor->setColor(value);
      break;

    case kCOLUBKAddr:
      instance().debugger().tiaDebug().coluBK(value);
      myCOLUBKColor->setColor(value);
      break;

    default:
      break;
  }
}
