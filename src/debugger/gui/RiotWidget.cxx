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

#include "Settings.hxx"
#include "DataGridWidget.hxx"
#include "FrameBuffer.hxx"
#include "GuiObject.hxx"
#include "OSystem.hxx"
#include "EventHandler.hxx"
#include "Debugger.hxx"
#include "RiotDebug.hxx"
#include "PopUpWidget.hxx"
#include "ToggleBitWidget.hxx"
#include "Widget.hxx"
#include "Layout.hxx"

#include "NullControlWidget.hxx"
#include "JoystickWidget.hxx"
#include "PaddleWidget.hxx"
#include "BoosterWidget.hxx"
#include "DrivingWidget.hxx"
#include "GenesisWidget.hxx"
#include "KeyboardWidget.hxx"
#include "AtariVoxWidget.hxx"
#include "SaveKeyWidget.hxx"
#include "AmigaMouseWidget.hxx"
#include "AtariMouseWidget.hxx"
#include "TrakBallWidget.hxx"
#include "QuadTariWidget.hxx"
#include "Joy2BPlusWidget.hxx"

#include "RiotWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RiotWidget::RiotWidget(GuiObject* boss, const GUI::Font& lfont,
                       const GUI::Font& nfont,
                       int x, int y, int w, int h)
  : Widget(boss, lfont, x, y, w, h),
    CommandSender(boss)
{
  VariantList items;

  // Bit strings for the toggle registers (the state changes, not the strings)
  StringList off, on;
  for(int i = 0; i < 8; ++i)
  {
    off.emplace_back("0");
    on.emplace_back("1");
  }
  StringList labels;

  // Create every widget at a placeholder position; reflow() lays them out
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  int reg = 0;
  const auto ioReg = [&](string_view desc, uInt8 bitsID) {
    myRegLabel[reg++] = new StaticTextWidget(boss, lfont, 0, 0, desc);
    auto* wid = new ToggleBitWidget(boss, nfont, 0, 0, 8, 1, 1, labels);
    wid->setTarget(this);
    wid->setID(bitsID);
    addFocusWidget(wid);
    wid->setList(off, on);
    return wid;
  };

  // SWCHA(W) / SWACNT / SWCHA(R) -- the read register carries per-bit labels
  labels.clear();
  mySWCHAWriteBits = ioReg("SWCHA(W)", kSWCHABitsID);
  mySWACNTBits = ioReg("SWACNT", kSWACNTBitsID);
  labels.clear();
  labels.emplace_back("Left right");   labels.emplace_back("Left left");
  labels.emplace_back("Left down");    labels.emplace_back("Left up");
  labels.emplace_back("Right right");  labels.emplace_back("Right left");
  labels.emplace_back("Right down");   labels.emplace_back("Right up");
  mySWCHAReadBits = ioReg("SWCHA(R)", kSWCHARBitsID);

  // SWCHB(W) / SWBCNT / SWCHB(R)
  labels.clear();
  mySWCHBWriteBits = ioReg("SWCHB(W)", kSWCHBBitsID);
  mySWBCNTBits = ioReg("SWBCNT", kSWBCNTBitsID);
  labels.clear();
  labels.emplace_back("Right difficulty");  labels.emplace_back("Left difficulty");
  labels.emplace_back("");                  labels.emplace_back("");
  labels.emplace_back("Color/B+W");         labels.emplace_back("");
  labels.emplace_back("Select");            labels.emplace_back("Reset");
  mySWCHBReadBits = ioReg("SWCHB(R)", kSWCHBRBitsID);

  // Timer registers (R/W): four labels beside a 4-row grid, plus the cycle count
  static constexpr std::array<string_view, 4> writeNames = {
    "TIM1T", "TIM8T", "TIM64T", "T1024T"
  };
  for(int row = 0; row < 4; ++row)
    myTimWriteLabel[row] = new StaticTextWidget(boss, lfont, 0, 0, writeNames[row]);
  myTimWrite = new DataGridWidget(boss, nfont, 0, 0, 1, 4, 2, 8, Common::Base::Fmt::_16);
  myTimWrite->setTarget(this);
  myTimWrite->setID(kTimWriteID);
  addFocusWidget(myTimWrite);
  myTimHash[0] = new StaticTextWidget(boss, lfont, 0, 0, "#");
  myTimAvail = new DataGridWidget(boss, nfont, 0, 0, 1, 1, 6, 30, Common::Base::Fmt::_10_6);
  myTimAvail->setToolTip("Number of CPU cycles available for current timer interval.\n");
  myTimAvail->setTarget(this);
  myTimAvail->setEditable(false);

  // Timer registers (RO)
  static constexpr std::array<string_view, 4> readNames = {
    "INTIM", " Clocks", "TIMINT", "Divider #"
  };
  for(int row = 0; row < 4; ++row)
    myTimReadLabel[row] = new StaticTextWidget(boss, lfont, 0, 0, readNames[row]);
  myTimRead = new DataGridWidget(boss, nfont, 0, 0, 1, 3, 4, 30, Common::Base::Fmt::_16_2);
  myTimRead->setToolTip(0, 1, "Remaining timer interval clocks.\n");
  myTimRead->setToolTip(0, 2, "Timer interrupt flag in bit 7.\n");
  myTimRead->setTarget(this);
  myTimRead->setEditable(false);
  myTimHash[1] = new StaticTextWidget(boss, lfont, 0, 0, "#");
  myTimHash[2] = new StaticTextWidget(boss, lfont, 0, 0, "#");
  myTimTotal = new DataGridWidget(boss, nfont, 0, 0, 1, 2, 6, 30, Common::Base::Fmt::_10_6);
  myTimTotal->setToolTip(0, 0, "Number of CPU cycles since last TIMxxT write.\n");
  myTimTotal->setToolTip(0, 1, "Number of CPU cycles remaining.\n");
  myTimTotal->setTarget(this);
  myTimTotal->setEditable(false);
  myTimDivider = new DataGridWidget(boss, nfont, 0, 0, 1, 1, 4, 12, Common::Base::Fmt::_10_4);
  myTimDivider->setTarget(this);
  myTimDivider->setEditable(false);

  // Controller ports: two full controller widgets (the row lays them out via
  // their own setArea(), so they re-flow themselves)
  myLeftControl = addControlWidget(boss, lfont, 0, 0,
      instance().console().leftController());
  addToFocusList(myLeftControl->getFocusList());
  myRightControl = addControlWidget(boss, lfont, 0, 0,
      instance().console().rightController());
  addToFocusList(myRightControl->getFocusList());

  // TIA INPTx registers (R), left and right ports
  static constexpr std::array<string_view, 3> leftINPTNames  = {"INPT0", "INPT1", "INPT4"};
  static constexpr std::array<string_view, 3> rightINPTNames = {"INPT2", "INPT3", "INPT5"};
  for(int row = 0; row < 3; ++row)
  {
    myLeftINPTLabel[row]  = new StaticTextWidget(boss, lfont, 0, 0, leftINPTNames[row]);
    myRightINPTLabel[row] = new StaticTextWidget(boss, lfont, 0, 0, rightINPTNames[row]);
  }
  myLeftINPT = new DataGridWidget(boss, nfont, 0, 0, 1, 3, 2, 8, Common::Base::Fmt::_16);
  myLeftINPT->setTarget(this);
  myLeftINPT->setEditable(false);
  myRightINPT = new DataGridWidget(boss, nfont, 0, 0, 1, 3, 2, 8, Common::Base::Fmt::_16);
  myRightINPT->setTarget(this);
  myRightINPT->setEditable(false);

  // TIA INPTx VBLANK bits (D6-latch, D7-dump) (R)
  myINPTLatch = new CheckboxWidget(boss, lfont, 0, 0, "INPT latch (VBlank D6)");
  myINPTLatch->setTarget(this);
  myINPTLatch->setEditable(false);
  myINPTDump = new CheckboxWidget(boss, lfont, 0, 0, "INPT dump to gnd (VBlank D7)");
  myINPTDump->setTarget(this);
  myINPTDump->setEditable(false);

  // P0 & P1 difficulty switches (self-sizing, fixed lists)
  items.clear();
  VarList::push_back(items, "B/easy", "b");
  VarList::push_back(items, "A/hard", "a");
  myP0Diff = new PopUpWidget(boss, lfont, 0, 0, items, "Left Diff", 0, kP0DiffChanged);
  myP0Diff->setTarget(this);
  addFocusWidget(myP0Diff);
  myP1Diff = new PopUpWidget(boss, lfont, 0, 0, items, "Right Diff", 0, kP1DiffChanged);
  myP1Diff->setTarget(this);
  addFocusWidget(myP1Diff);

  // TV Type
  items.clear();
  VarList::push_back(items, "B&W", "bw");
  VarList::push_back(items, "Color", "color");
  myTVType = new PopUpWidget(boss, lfont, 0, 0, items, "TV Type", 0, kTVTypeChanged);
  myTVType->setToolTip("Atari 2600 Color/B&W switch.");
  myTVType->setTarget(this);
  addFocusWidget(myTVType);

  // 2600/7800 mode (bottom of the left column)
  items.clear();
  VarList::push_back(items, "Atari 2600", "2600");
  VarList::push_back(items, "Atari 7800", "7800");
  myConsoleLabel = new StaticTextWidget(boss, lfont, 0, 0, "Console");
  myConsole = new PopUpWidget(boss, lfont, 0, 0, items, "", 0, kConsoleID);
  myConsole->setTarget(this);
  myConsole->setToolTip("Emulated console.");
  addFocusWidget(myConsole);

  // Select, Reset and Pause (beside the difficulty/TV pop-ups)
  const auto check = [&](string_view label, uInt8 id) {
    auto* cb = new CheckboxWidget(boss, lfont, 0, 0, label,
                                  CheckboxWidget::kCheckActionCmd);
    cb->setID(id);
    cb->setTarget(this);
    addFocusWidget(cb);
    return cb;
  };
  mySelect = check("Select", kSelectID);
  myReset  = check("Reset", kResetID);
  myPause  = check("Pause", kPauseID);
  myPause->setToolTip("Atari 7800 pause switch.");
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  setHelpAnchor("IOTab", true);

  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RiotWidget::setArea(int x, int y, int w, int h)
{
  Widget::setArea(x, y, w, h);
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<GUI::Layout> RiotWidget::buildLayout() const
{
  using GUI::BoxLayout;
  using GUI::GridLayout;
  using GUI::anchoredItem;
  using GUI::indentedItem;
  using GUI::alignedItem;
  using GUI::HAlign;
  using GUI::VAlign;
  using Dir = BoxLayout::Dir;

  // A label beside a single-row register/grid sits on its line (shared baseline)
  const auto onBaseline = [](Widget* wid) {
    return alignedItem(wid, HAlign::Left, VAlign::Baseline);
  };

  const int fontWidth  = _fontWidth,
            lineHeight = _lineHeight,
            fontHeight = _font.getFontHeight(),
            VGAP    = fontHeight / 4,
            HBORDER = static_cast<int>(fontWidth * 1.25),
            VBORDER = fontHeight / 2,
            HGAP    = fontWidth,
            lwidth  = 9 * fontWidth;   // shared left-column label width

  // A vertical stack of labels lined up with the rows of the grid beside it: the
  // grid insets each row's text where a label centers its own (CpuWidget idiom)
  const auto gridLabels = [&](auto&& lbls, DataGridWidget* grid) {
    auto colv = std::make_unique<BoxLayout>(Dir::Vertical);
    colv->addSpace(grid->firstTextY() - lbls[0]->firstTextY());
    for(auto* l: lbls)
      colv->addFixed(anchoredItem(l), lineHeight);
    return colv;
  };

  // ---- LEFT column: the SW registers, the timers, the console selector ----
  auto left = std::make_unique<BoxLayout>(Dir::Vertical, VGAP);

  // Six I/O bit registers: label + 8-bit toggle, a wider gap between the groups
  const auto regRow = [&](StaticTextWidget* label, ToggleBitWidget* bits) {
    auto row = std::make_unique<BoxLayout>(Dir::Horizontal);
    row->addFixed(onBaseline(label), lwidth);
    row->addFixed(onBaseline(bits), bits->getWidth());
    return row;
  };
  left->addAuto(regRow(myRegLabel[0], mySWCHAWriteBits));
  left->addAuto(regRow(myRegLabel[1], mySWACNTBits));
  left->addAuto(regRow(myRegLabel[2], mySWCHAReadBits));
  left->addSpace(VGAP * 2);
  left->addAuto(regRow(myRegLabel[3], mySWCHBWriteBits));
  left->addAuto(regRow(myRegLabel[4], mySWBCNTBits));
  left->addAuto(regRow(myRegLabel[5], mySWCHBReadBits));
  left->addSpace(VGAP * 4);

  // The write grid is 2 chars wide, the read/divider grids 4; giving both the
  // same column keeps the "#" and cycle readouts in one line down both blocks
  const int gridColW = std::max(myTimWrite->getWidth(), myTimRead->getWidth());

  // Timer write: 4 labels | 4-row grid | "#" | available-cycles grid.  The "#"
  // abuts its readout (gaps added explicitly so the two stay centered but touch)
  {
    auto row = std::make_unique<BoxLayout>(Dir::Horizontal);
    row->addFixed(gridLabels(myTimWriteLabel, myTimWrite), lwidth);
    row->addFixed(anchoredItem(myTimWrite), gridColW);
    row->addSpace(HGAP);
    row->addAuto(anchoredItem(myTimHash[0]));
    row->addAuto(anchoredItem(myTimAvail));
    left->addAuto(std::move(row));
  }
  left->addSpace(VGAP);

  // Timer read: 3 labels | 3-row grid | "#"x2 | 2-row total.  The total nestles
  // half a row down, centered against the 3 read rows; its two "#" ride along
  // (both top-aligned within the block, so gridLabels pairs each "#" to its row)
  {
    const std::array<StaticTextWidget*, 3> readLbls =
      {myTimReadLabel[0], myTimReadLabel[1], myTimReadLabel[2]};
    const std::array<StaticTextWidget*, 2> hashLbls =
      {myTimHash[1], myTimHash[2]};

    auto total = std::make_unique<BoxLayout>(Dir::Horizontal);
    total->addAuto(gridLabels(hashLbls, myTimTotal));
    total->addAuto(alignedItem(myTimTotal, HAlign::Left, VAlign::Top));
    auto totalCol = std::make_unique<BoxLayout>(Dir::Vertical);
    totalCol->addSpace(lineHeight / 2);
    totalCol->addAuto(std::move(total));

    auto row = std::make_unique<BoxLayout>(Dir::Horizontal);
    row->addFixed(gridLabels(readLbls, myTimRead), lwidth);
    row->addFixed(anchoredItem(myTimRead), gridColW);
    row->addSpace(HGAP);
    row->addAuto(std::move(totalCol));
    left->addAuto(std::move(row));
  }

  // Timer divider: its own labeled row, value aligned under the read grid
  {
    auto row = std::make_unique<BoxLayout>(Dir::Horizontal);
    row->addFixed(onBaseline(myTimReadLabel[3]), lwidth);
    row->addFixed(onBaseline(myTimDivider), gridColW);
    left->addAuto(std::move(row));
  }
  left->addSpace(VGAP * 2);

  // Console 2600/7800 selector
  {
    auto row = std::make_unique<BoxLayout>(Dir::Horizontal);
    row->addFixed(onBaseline(myConsoleLabel), lwidth);
    row->addAuto(anchoredItem(myConsole));
    left->addAuto(std::move(row));
  }

  // ---- RIGHT column: controllers, INPT registers, switches ----
  auto right = std::make_unique<BoxLayout>(Dir::Vertical, VGAP);

  // The two controller widgets side by side; each re-flows via its own setArea
  {
    auto row = std::make_unique<BoxLayout>(Dir::Horizontal, HGAP * 4);
    row->addAuto(anchoredItem(myLeftControl));
    row->addAuto(anchoredItem(myRightControl));
    right->addAuto(std::move(row));
  }
  right->addSpace(VGAP * 2);

  // INPT registers: left labels+grid and right labels+grid, side by side
  const auto inptCluster = [&](auto&& lbls, DataGridWidget* grid) {
    auto row = std::make_unique<BoxLayout>(Dir::Horizontal, HGAP);
    row->addAuto(gridLabels(lbls, grid));
    row->addAuto(anchoredItem(grid));
    return row;
  };
  {
    auto row = std::make_unique<BoxLayout>(Dir::Horizontal, HGAP * 3);
    row->addAuto(inptCluster(myLeftINPTLabel, myLeftINPT));
    row->addAuto(inptCluster(myRightINPTLabel, myRightINPT));
    right->addAuto(std::move(row));
  }
  right->addSpace(VGAP);

  // INPT latch / dump, indented under the INPT registers
  right->addAuto(indentedItem(myINPTLatch, HGAP * 2));
  right->addAuto(indentedItem(myINPTDump, HGAP * 2));
  right->addSpace(VGAP * 4);

  // Switches: the diff/TV pop-ups line up as one column, the Select/Reset/Pause
  // checkboxes beside them, row for row
  GUI::alignLabels({{myP0Diff}, {myP1Diff}, {myTVType}});
  GUI::alignPopUps({myP0Diff, myP1Diff, myTVType});
  {
    auto grid = std::make_unique<GridLayout>(2, 3, HGAP * 2, VGAP);
    grid->columnAuto(0).columnAuto(1);
    for(int r = 0; r < 3; ++r)
      grid->rowAuto(r);
    grid->place(0, 0, anchoredItem(myP0Diff));
    grid->place(1, 0, anchoredItem(mySelect));
    grid->place(0, 1, anchoredItem(myP1Diff));
    grid->place(1, 1, anchoredItem(myReset));
    grid->place(0, 2, anchoredItem(myTVType));
    grid->place(1, 2, anchoredItem(myPause));
    right->addAuto(std::move(grid));
  }

  // ---- assemble the two columns, inset by the standard borders ----
  auto root = std::make_unique<BoxLayout>(Dir::Horizontal, fontWidth * 2,
                                          HBORDER, VBORDER);
  root->addAuto(std::move(left));
  root->addAuto(std::move(right));

  return root;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RiotWidget::reflow()
{
  auto root = buildLayout();

  // This tab does not fill its area, it sizes itself to its content: take the
  // size the tree comes to, then lay out within it
  const Common::Size natural = root->naturalSize();
  _w = static_cast<int>(natural.w);
  _h = static_cast<int>(natural.h);
  root->doLayout(_x, _y, _w, _h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size RiotWidget::naturalSize() const
{
  return buildLayout()->naturalSize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RiotWidget::loadConfig()
{
  IntArray alist;
  IntArray vlist;
  BoolArray changed;

  auto IO_REGS_UPDATE = [&](ToggleBitWidget* bits,
                            const BoolArray& s_bits, const BoolArray& old_bits)
  {
    changed.clear();
    for(uInt32 i = 0; i < s_bits.size(); ++i)
      changed.push_back(s_bits[i] != old_bits[i]);
    bits->setState(s_bits, changed);
  };

  // We push the enumerated items as addresses, and deal with the real
  // address in the callback (handleCommand)
  RiotDebug& riot = instance().debugger().riotDebug();
  const auto& state    = static_cast<const RiotState&>(riot.getState());
  const auto& oldstate = static_cast<const RiotState&>(riot.getOldState());

  // Update the SWCHA register booleans (poke mode)
  IO_REGS_UPDATE(mySWCHAWriteBits, state.swchaWriteBits, oldstate.swchaWriteBits);

  // Update the SWACNT register booleans
  IO_REGS_UPDATE(mySWACNTBits, state.swacntBits, oldstate.swacntBits);

  // Update the SWCHA register booleans (peek mode)
  IO_REGS_UPDATE(mySWCHAReadBits, state.swchaReadBits, oldstate.swchaReadBits);

  // Update the SWCHB register booleans (poke mode)
  IO_REGS_UPDATE(mySWCHBWriteBits, state.swchbWriteBits, oldstate.swchbWriteBits);

  // Update the SWBCNT register booleans
  IO_REGS_UPDATE(mySWBCNTBits, state.swbcntBits, oldstate.swbcntBits);

  // Update the SWCHB register booleans (peek mode)
  IO_REGS_UPDATE(mySWCHBReadBits, state.swchbReadBits, oldstate.swchbReadBits);

  // Update TIA INPTx registers
  alist.clear();  vlist.clear();  changed.clear();
  alist.push_back(0);  vlist.push_back(state.INPT0);
    changed.push_back(state.INPT0 != oldstate.INPT0);
  alist.push_back(1);  vlist.push_back(state.INPT1);
    changed.push_back(state.INPT1 != oldstate.INPT1);
  alist.push_back(4);  vlist.push_back(state.INPT4);
    changed.push_back(state.INPT4 != oldstate.INPT4);
  myLeftINPT->setList(alist, vlist, changed);
  alist.clear();  vlist.clear();  changed.clear();
  alist.push_back(2);  vlist.push_back(state.INPT2);
    changed.push_back(state.INPT2 != oldstate.INPT2);
  alist.push_back(3);  vlist.push_back(state.INPT3);
    changed.push_back(state.INPT3 != oldstate.INPT3);
  alist.push_back(5);  vlist.push_back(state.INPT5);
    changed.push_back(state.INPT5 != oldstate.INPT5);
  myRightINPT->setList(alist, vlist, changed);

  // Update TIA VBLANK bits
  myINPTLatch->setState(riot.vblank(6), state.INPTLatch != oldstate.INPTLatch);
  myINPTDump->setState(riot.vblank(7), state.INPTDump != oldstate.INPTDump);

  // Update timer write registers
  alist.clear();  vlist.clear();  changed.clear();
  alist.push_back(kTim1TID);  vlist.push_back(state.TIM1T);
    changed.push_back(state.TIM1T != oldstate.TIM1T);
  alist.push_back(kTim8TID);  vlist.push_back(state.TIM8T);
    changed.push_back(state.TIM8T != oldstate.TIM8T);
  alist.push_back(kTim64TID);  vlist.push_back(state.TIM64T);
    changed.push_back(state.TIM64T != oldstate.TIM64T);
  alist.push_back(kTim1024TID);  vlist.push_back(state.T1024T);
    changed.push_back(state.T1024T != oldstate.T1024T);
  myTimWrite->setList(alist, vlist, changed);
  myTimWriteLabel[0]->setEnabled(state.TIM1T);
  myTimWriteLabel[1]->setEnabled(state.TIM8T);
  myTimWriteLabel[2]->setEnabled(state.TIM64T);
  myTimWriteLabel[3]->setEnabled(state.T1024T);

  alist.clear();  vlist.clear();  changed.clear();
  alist.push_back(0);
  Int32 avail = 0;
  if(state.TIMDIV == 1)
    avail = (state.TIM1T  - 1) * 1;
  else if(state.TIMDIV == 8)
    avail = (state.TIM8T  - 1) * 8;
  else if(state.TIMDIV == 64)
    avail = (state.TIM64T - 1) * 64;
  else if(state.TIMDIV == 1024)
    avail = (state.T1024T - 1) * 1024;
  vlist.push_back(avail);
  changed.push_back(state.TIM1T != oldstate.TIM1T ||
                    state.TIM8T != oldstate.TIM8T ||
                    state.TIM64T != oldstate.TIM64T ||
                    state.T1024T != oldstate.T1024T);
  myTimAvail->setList(alist, vlist, changed);

  // Update timer read registers
  alist.clear();  vlist.clear();  changed.clear();
  alist.push_back(0);  vlist.push_back(state.INTIM);
    changed.push_back(state.INTIM != oldstate.INTIM);
  alist.push_back(0);  vlist.push_back(state.INTIMCLKS);
    changed.push_back(state.INTIMCLKS != oldstate.INTIMCLKS);
  alist.push_back(0);  vlist.push_back(state.TIMINT);
    changed.push_back(state.TIMINT != oldstate.TIMINT);
  myTimRead->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  alist.push_back(0);  vlist.push_back(state.TIMCLKS);
    changed.push_back(state.TIMCLKS != oldstate.TIMCLKS);
  alist.push_back(0);  vlist.push_back(avail - state.TIMCLKS);
    changed.push_back(state.TIMCLKS != oldstate.TIMCLKS);
  myTimTotal->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  alist.push_back(0);  vlist.push_back(state.TIMDIV);
  changed.push_back(state.TIMDIV != oldstate.TIMDIV);
  myTimDivider->setList(alist, vlist, changed);

  // Console switches (inverted, since 'selected' in the UI
  // means 'grounded' in the system)
  myP0Diff->setSelectedIndex(riot.diffP0(), state.swchbReadBits[1] != oldstate.swchbReadBits[1]);
  myP1Diff->setSelectedIndex(riot.diffP1(), state.swchbReadBits[0] != oldstate.swchbReadBits[0]);

  const bool devSettings = instance().settings().getBool("dev.settings");
  myConsole->setSelected(instance().settings().getString(devSettings ? "dev.console" : "plr.console"));

  myTVType->setSelectedIndex(riot.tvType(), state.swchbReadBits[4] != oldstate.swchbReadBits[4]);
  myPause->setState(!riot.tvType(), state.swchbReadBits[4] != oldstate.swchbReadBits[4]);
  mySelect->setState(!riot.select(), state.swchbReadBits[6] != oldstate.swchbReadBits[6]);
  myReset->setState(!riot.reset(), state.swchbReadBits[7] != oldstate.swchbReadBits[7]);

  myLeftControl->loadConfig();
  myRightControl->loadConfig();

  handleConsole();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RiotWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  int value = -1;
  RiotDebug& riot = instance().debugger().riotDebug();

  switch(cmd)
  {
    case DataGridWidget::kItemDataChangedCmd:
      if(id == kTimWriteID)
      {
        switch(myTimWrite->getSelectedAddr())
        {
          case kTim1TID:
            riot.tim1T(myTimWrite->getSelectedValue());
            break;
          case kTim8TID:
            riot.tim8T(myTimWrite->getSelectedValue());
            break;
          case kTim64TID:
            riot.tim64T(myTimWrite->getSelectedValue());
            break;
          case kTim1024TID:
            riot.tim1024T(myTimWrite->getSelectedValue());
            break;
          default:
            break;
        }
      }
      break;

    case ToggleWidget::kItemDataChangedCmd:
      switch(id)
      {
        case kSWCHABitsID:
          value = Debugger::get_bits(mySWCHAWriteBits->getState());
          riot.swcha(value & 0xff);
          break;
        case kSWACNTBitsID:
          value = Debugger::get_bits(mySWACNTBits->getState());
          riot.swacnt(value & 0xff);
          break;
        case kSWCHBBitsID:
          value = Debugger::get_bits(mySWCHBWriteBits->getState());
          riot.swchb(value & 0xff);
          break;
        case kSWBCNTBitsID:
          value = Debugger::get_bits(mySWBCNTBits->getState());
          riot.swbcnt(value & 0xff);
          break;
        case kSWCHARBitsID:
        {
          value = Debugger::get_bits(mySWCHAReadBits->getState());
          ControllerLowLevel lport(instance().console().leftController());
          ControllerLowLevel rport(instance().console().rightController());
          lport.setPin(Controller::DigitalPin::One,   value & 0b00010000);
          lport.setPin(Controller::DigitalPin::Two,   value & 0b00100000);
          lport.setPin(Controller::DigitalPin::Three, value & 0b01000000);
          lport.setPin(Controller::DigitalPin::Four,  value & 0b10000000);
          rport.setPin(Controller::DigitalPin::One,   value & 0b00000001);
          rport.setPin(Controller::DigitalPin::Two,   value & 0b00000010);
          rport.setPin(Controller::DigitalPin::Three, value & 0b00000100);
          rport.setPin(Controller::DigitalPin::Four,  value & 0b00001000);
          break;
        }
        case kSWCHBRBitsID:
        {
          value = Debugger::get_bits(mySWCHBReadBits->getState());

          riot.reset( value & 0b00000001);
          riot.select(value & 0b00000010);
          riot.tvType(value & 0b00001000);
          riot.diffP0(value & 0b01000000);
          riot.diffP1(value & 0b10000000);
          break;
        }
        default:
          break;
      }
      break;

    case kConsoleID:
    {
      Settings& settings = instance().settings();
      const string& prefix = settings.getBool("dev.settings") ? "dev." : "plr.";

      settings.setValue(prefix + "console", myConsole->getSelectedTag());
      instance().eventHandler().set7800Mode();
      break;
    }

    case CheckboxWidget::kCheckActionCmd:
      switch(id)
      {
        case kSelectID:
          riot.select(!mySelect->getState());
          break;
        case kResetID:
          riot.reset(!myReset->getState());
          break;
        case kPauseID:
          handleConsole();
          break;
        default:
          break;
      }
      break;

    case kP0DiffChanged:
      riot.diffP0(myP0Diff->getSelectedTag().toString() != "b");
      break;

    case kP1DiffChanged:
      riot.diffP1(myP1Diff->getSelectedTag().toString() != "b");
      break;

    case kTVTypeChanged:
      handleConsole();
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ControllerWidget*
RiotWidget::addControlWidget(GuiObject* boss, const GUI::Font& font,
                             int x, int y, Controller& controller)
{
  switch(controller.type())
  {
    using enum Controller::Type;
    case AmigaMouse:  return new AmigaMouseWidget(boss, font, x, y, controller);
    case AtariMouse:  return new AtariMouseWidget(boss, font, x, y, controller);
    case AtariVox:    return new AtariVoxWidget(boss, font, x, y, controller);
    case BoosterGrip: return new BoosterWidget(boss, font, x, y, controller);
    case Driving:     return new DrivingWidget(boss, font, x, y, controller);
    case Genesis:     return new GenesisWidget(boss, font, x, y, controller);
    case Joy2BPlus:   return new Joy2BPlusWidget(boss, font, x, y, controller);
    case Joystick:    return new JoystickWidget(boss, font, x, y, controller);
    case Keyboard:    return new KeyboardWidget(boss, font, x, y, controller);
//    case KidVid:      // TODO - implement this
//    case MindLink:    // TODO - implement this
//    case Lightgun:    // TODO - implement this
    case QuadTari:    return new QuadTariWidget(boss, font, x, y, controller);
    case Paddles:     return new PaddleWidget(boss, font, x, y, controller);
    case SaveKey:     return new SaveKeyWidget(boss, font, x, y, controller);
    case TrakBall:    return new TrakBallWidget(boss, font, x, y, controller);
    default:          return new NullControlWidget(boss, font, x, y, controller);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RiotWidget::handleConsole()
{
  RiotDebug& riot = instance().debugger().riotDebug();
  const bool devSettings = instance().settings().getBool("dev.settings");
  const bool is7800 = instance().settings().getString(
    devSettings ? "dev.console" : "plr.console") == "7800";

  myTVType->setEnabled(!is7800);
  myPause->setEnabled(is7800);
  if(is7800)
    myTVType->setSelectedIndex(myPause->getState() ? 0 : 1);
  else
    myPause->setState(myTVType->getSelected() == 0);
  riot.tvType(myTVType->getSelected());
}
