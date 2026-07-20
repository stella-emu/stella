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

#include "OSystem.hxx"
#include "GuiObject.hxx"
#include "Debugger.hxx"
#include "CartDebug.hxx"
#include "CpuDebug.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "DataGridWidget.hxx"
#include "EditTextWidget.hxx"
#include "ToggleBitWidget.hxx"
#include "Layout.hxx"

#include "CpuWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CpuWidget::CpuWidget(GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont)
  : Widget(boss, lfont, 0, 0, 0, 0),
    CommandSender(boss)
{
  const int fontHeight = lfont.getFontHeight();
  const std::array<string, 4> labels = { "SP", "A", "X", "Y" };

  // Create every widget; reflow() positions and sizes them for the area the
  // parent layout gives us
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  myPCText = new StaticTextWidget(boss, lfont, "PC", TextAlign::Left);
  myPCGrid = new DataGridWidget(boss, nfont, 1, 1, 4, 16, Common::Base::Fmt::_16);
  myPCGrid->setHelpAnchor("CPURegisters", true);
  myPCGrid->setTarget(this);
  myPCGrid->setID(kPCRegID);
  addFocusWidget(myPCGrid);

  // Read-only textbox containing the current PC label
  myPCLabel = new EditTextWidget(boss, nfont, 1, fontHeight + 1, "");
  myPCLabel->setEditable(false, true);

  // 1x4 grid with labels for the other CPU registers
  myCpuGrid = new DataGridWidget(boss, nfont, 1, 4, 2, 8, Common::Base::Fmt::_16);
  myCpuGrid->setHelpAnchor("CPURegisters", true);
  myCpuGrid->setTarget(this);
  myCpuGrid->setID(kCpuRegID);
  addFocusWidget(myCpuGrid);

  // 1x4 grids with the decimal and binary values for those registers
  myCpuGridDecValue = new DataGridWidget(boss, nfont, 1, 4, 3, 8, Common::Base::Fmt::_10);
  myCpuGridDecValue->setHelpAnchor("CPURegisters", true);
  myCpuGridDecValue->setTarget(this);
  myCpuGridDecValue->setID(kCpuRegDecID);
  addFocusWidget(myCpuGridDecValue);

  myCpuGridBinValue = new DataGridWidget(boss, nfont, 1, 4, 8, 8, Common::Base::Fmt::_2);
  myCpuGridBinValue->setHelpAnchor("CPURegisters", true);
  myCpuGridBinValue->setTarget(this);
  myCpuGridBinValue->setID(kCpuRegBinID);
  addFocusWidget(myCpuGridBinValue);

  // Labels showing the source of data for the SP/A/X/Y registers
  for(int i = 0; i < 4; ++i)
  {
    myCpuDataSrc[i] = new EditTextWidget(boss, nfont, 1, fontHeight + 1);
    myCpuDataSrc[i]->setToolTip("Source label of last read for " + labels[i] + ".");
    myCpuDataSrc[i]->setEditable(false, true);
  }

  // Row labels for the other CPU registers and the '#'/'%' value prefixes
  for(int row = 0; row < 4; ++row)
  {
    myRegLabels[row] = new StaticTextWidget(boss, lfont, labels[row], TextAlign::Left);
    myDecPrefix[row] = new StaticTextWidget(boss, lfont, "#");
    myBinPrefix[row] = new StaticTextWidget(boss, lfont, "%");
  }

  // Bitfield widget for changing the processor status
  myPSText = new StaticTextWidget(boss, lfont, "PS", TextAlign::Left);
  myPSRegister = new ToggleBitWidget(boss, nfont, 8, 1);
  myPSRegister->setHelpAnchor("CPURegisters", true);
  myPSRegister->setTarget(this);
  addFocusWidget(myPSRegister);

  // Last write destination address
  myDestText = new StaticTextWidget(boss, lfont, "Dest");
  myCpuDataDest = new EditTextWidget(boss, nfont, 1, fontHeight + 1);
  myCpuDataDest->setToolTip("Destination label of last write.");
  myCpuDataDest->setEditable(false, true);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  // Set the strings to be used in the PSRegister
  // We only do this once because it's the state that changes, not the strings
  static constexpr std::array<string_view, 8> offstr = {
    "n", "v", "-", "b", "d", "i", "z", "c"
  };
  static constexpr std::array<string_view, 8> onstr  = {
    "N", "V", "-", "B", "D", "I", "Z", "C"
  };
  const StringList off(offstr.begin(), offstr.end());
  const StringList on(onstr.begin(), onstr.end());
  myPSRegister->setList(off, on);

  setHelpAnchor("DataOpButtons", true);

  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuWidget::setArea(int x, int y, int w, int h)
{
  Widget::setArea(x, y, w, h);
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<GUI::Layout> CpuWidget::buildLayout() const
{
  using GUI::BoxLayout;
  using GUI::GridLayout;
  using GUI::anchoredItem;
  using GUI::alignedItem;
  using GUI::stretchedItem;
  using GUI::HAlign;
  using GUI::VAlign;
  using Dir = BoxLayout::Dir;

  // A label beside a data grid sits on the grid's first row, not on the middle
  // of the whole grid, so the two share the row's baseline
  const auto onBaseline = [](Widget* wid) {
    return alignedItem(wid, HAlign::Left, VAlign::Baseline);
  };

  const int lineHeight = _font.getLineHeight(),
            HGAP       = _fontWidth;
  constexpr int VGAP = 2;
  // A deliberate four-character column, wider than the register names in it
  const int lwidth = 4 * _fontWidth;

  // A per-row column of labels beside a (4-row) data grid.  The grid insets each
  // row's text where a label centers its own, so the column starts that much
  // lower and label i lands on grid row i's line.  A stack of labels is one item
  // to the row beside it, so it cannot use VAlign::Baseline
  const auto column = [&](const std::array<StaticTextWidget*, 4>& cells) {
    auto col = std::make_unique<BoxLayout>(Dir::Vertical);
    col->addSpace(myCpuGrid->firstTextY() - cells[0]->firstTextY());
    for(auto* wid: cells)
      col->addFixed(anchoredItem(wid), lineHeight);
    return col;
  };

  // The four data-source fields, one per grid row.  Each is a pixel taller than
  // its row, so that its bottom frame falls on the next one's top frame and the
  // two draw as a single line -- the height the constructor gave them, restored
  // here because a font change resets a field to the standard framed height
  for(auto* wid: myCpuDataSrc)
    wid->setHeight(lineHeight + 1);

  auto srcCol = std::make_unique<BoxLayout>(Dir::Vertical);
  for(auto* wid: myCpuDataSrc)
    srcCol->addFixed(alignedItem(wid, HAlign::Fill, VAlign::Top), lineHeight);

  // Three rows over one set of columns.  Sharing the columns is what lines the
  // hex grid up under the PC grid and -- the point of doing it this way -- puts
  // the destination field in the very column the source fields are in, with
  // nobody reading back where those ended up
  enum Col: uInt8 {
    LABEL, GRID, DECPFX, DEC, GAP1, BINPFX, BIN, GAP2, DATA, NUM_COLS
  };
  auto grid = std::make_unique<GridLayout>(NUM_COLS, 3, 0, VGAP);

  // Each prefix abuts the grid it belongs to, so those columns are sized by
  // what is in them and carry no gap; the gaps separate the value groups
  grid->columnFixed(LABEL, lwidth);
  grid->columnAuto(GRID).columnAuto(DECPFX).columnAuto(DEC);
  grid->columnAuto(BINPFX).columnAuto(BIN);
  grid->columnFixed(GAP1, HGAP).columnFixed(GAP2, HGAP * 2);
  grid->columnStretch(DATA);
  grid->rowAuto(0).rowAuto(1).rowAuto(2);

  // PC row: "PC", the PC grid, then the current-PC label across the rest
  auto pcLabel = std::make_unique<BoxLayout>(Dir::Horizontal);
  pcLabel->addSpace(HGAP);
  pcLabel->addStretch(alignedItem(myPCLabel, HAlign::Fill, VAlign::Fill));

  grid->place(LABEL, 0, onBaseline(myPCText));
  grid->place(GRID,  0, onBaseline(myPCGrid));
  grid->place(DECPFX, 0, std::move(pcLabel), NUM_COLS - DECPFX);

  // Register rows: names | hex | # dec | % bin | the source of each value
  grid->place(LABEL,  1, column(myRegLabels));
  grid->place(GRID,   1, anchoredItem(myCpuGrid));
  grid->place(DECPFX, 1, column(myDecPrefix));
  grid->place(DEC,    1, anchoredItem(myCpuGridDecValue));
  grid->place(BINPFX, 1, column(myBinPrefix));
  grid->place(BIN,    1, anchoredItem(myCpuGridBinValue));
  grid->place(DATA,   1, std::move(srcCol));

  // PS row: the status toggle and the write destination share one cell across
  // the value columns.  They must, rather than take a column each: the toggle
  // is wider than any one of them, and a spanning cell that its tracks cannot
  // hold BETWEEN them would widen those tracks and unpick the rows above
  auto psRow = std::make_unique<BoxLayout>(Dir::Horizontal);
  psRow->addAuto(onBaseline(myPSRegister));
  psRow->addStretchSpace();
  psRow->addAuto(onBaseline(myDestText));
  psRow->addSpace(HGAP);

  grid->place(LABEL, 2, onBaseline(myPSText));
  grid->place(GRID,  2, std::move(psRow), DATA - GRID);
  grid->place(DATA,  2, alignedItem(myCpuDataDest, HAlign::Fill, VAlign::Fill));

  // The rows sit in from the right, as the fields above them do
  auto root = std::make_unique<BoxLayout>(Dir::Horizontal, 0, 0, 0);
  root->addStretch(std::move(grid));
  root->addSpace(HGAP);

  return root;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size CpuWidget::naturalSize() const
{
  return buildLayout()->naturalSize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuWidget::reflow()
{
  buildLayout()->doLayout(_x, _y, _w, _h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuWidget::setOpsWidget(DataGridOpsWidget* w)
{
  myPCGrid->setOpsWidget(w);
  myCpuGrid->setOpsWidget(w);
  myCpuGridDecValue->setOpsWidget(w);
  myCpuGridBinValue->setOpsWidget(w);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  int addr = -1, value = -1;
  CpuDebug& dbg = instance().debugger().cpuDebug();

  switch(cmd)
  {
    case DataGridWidget::kItemDataChangedCmd:
      switch(id)
      {
        case kPCRegID:
          addr  = myPCGrid->getSelectedAddr();
          value = myPCGrid->getSelectedValue();
          break;

        case kCpuRegID:
          addr  = myCpuGrid->getSelectedAddr();
          value = myCpuGrid->getSelectedValue();
          break;

        case kCpuRegDecID:
          addr  = myCpuGridDecValue->getSelectedAddr();
          value = myCpuGridDecValue->getSelectedValue();
          break;

        case kCpuRegBinID:
          addr  = myCpuGridBinValue->getSelectedAddr();
          value = myCpuGridBinValue->getSelectedValue();
          break;

        default:
          break;
      }

      switch(addr)
      {
        case kPCRegAddr:
          // Use the parser to set PC, since we want to propagate the
          // event the rest of the debugger widgets
          instance().debugger().run(std::format("pc #{}", value));
          break;

        case kSPRegAddr:
          dbg.setSP(value);
          myCpuGrid->setValueInternal(0, value);
          myCpuGridDecValue->setValueInternal(0, value);
          myCpuGridBinValue->setValueInternal(0, value);
          break;

        case kARegAddr:
          dbg.setA(value);
          myCpuGrid->setValueInternal(1, value);
          myCpuGridDecValue->setValueInternal(1, value);
          myCpuGridBinValue->setValueInternal(1, value);
          break;

        case kXRegAddr:
          dbg.setX(value);
          myCpuGrid->setValueInternal(2, value);
          myCpuGridDecValue->setValueInternal(2, value);
          myCpuGridBinValue->setValueInternal(2, value);
          break;

        case kYRegAddr:
          dbg.setY(value);
          myCpuGrid->setValueInternal(3, value);
          myCpuGridDecValue->setValueInternal(3, value);
          myCpuGridBinValue->setValueInternal(3, value);
          break;

        default:
          break;
      }
      break;

    case ToggleWidget::kItemDataChangedCmd:
    {
      const bool state = myPSRegister->getSelectedState();

      switch(data)
      {
        case kPSRegN:
          dbg.setN(state);
          break;

        case kPSRegV:
          dbg.setV(state);
          break;

        case kPSRegB:
          dbg.setB(state);
          break;

        case kPSRegD:
          dbg.setD(state);
          break;

        case kPSRegI:
          dbg.setI(state);
          break;

        case kPSRegZ:
          dbg.setZ(state);
          break;

        case kPSRegC:
          dbg.setC(state);
          break;

        default:
          break;
      }
    }
    break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuWidget::loadConfig()
{
  IntArray alist;
  IntArray vlist;
  BoolArray changed;

  // We push the enumerated items as addresses, and deal with the real
  // address in the callback (handleCommand)
  const Debugger& dbg = instance().debugger();
  const CartDebug& cart = dbg.cartDebug();
  CpuDebug& cpu = dbg.cpuDebug();
  const auto& state    = static_cast<const CpuState&>(cpu.getState());
  const auto& oldstate = static_cast<const CpuState&>(cpu.getOldState());

  // Add PC to its own DataGridWidget
  alist.push_back(kPCRegAddr);
  vlist.push_back(state.PC);
  changed.push_back(state.PC != oldstate.PC);

  myPCGrid->setList(alist, vlist, changed);

  // Add the other registers
  alist.clear(); vlist.clear(); changed.clear();
  alist.push_back(kSPRegAddr);
  alist.push_back(kARegAddr);
  alist.push_back(kXRegAddr);
  alist.push_back(kYRegAddr);

  // And now fill the values
  vlist.push_back(state.SP);
  vlist.push_back(state.A);
  vlist.push_back(state.X);
  vlist.push_back(state.Y);

  // Figure out which items have changed
  changed.push_back(state.SP != oldstate.SP);
  changed.push_back(state.A  != oldstate.A);
  changed.push_back(state.X  != oldstate.X);
  changed.push_back(state.Y  != oldstate.Y);

  // Finally, update the register list
  myCpuGrid->setList(alist, vlist, changed);
  myCpuGridDecValue->setList(alist, vlist, changed);
  myCpuGridBinValue->setList(alist, vlist, changed);

  // Update the data sources for the SP/A/X/Y registers
  const auto setSrc = [&](int idx, int cur, int old, bool isTrue = true) {
    const string& label = cur < 0 ? "IMM" : cart.getLabel(cur, isTrue);
    myCpuDataSrc[idx]->setText(
      !label.empty() ? label : Common::Base::toString(cur),
      cur != old);
  };
  setSrc(0, state.srcS, oldstate.srcS);
  setSrc(1, state.srcA, oldstate.srcA);
  setSrc(2, state.srcX, oldstate.srcX);
  setSrc(3, state.srcY, oldstate.srcY);

  const string& dest = state.dest < 0 ? "" : cart.getLabel(state.dest, false);
  myCpuDataDest->setText((!dest.empty() ? dest : Common::Base::toString(state.dest)),
                         state.dest != oldstate.dest);

  // Update the PS register booleans
  changed.clear();
  for(uInt32 i = 0; i < state.PSbits.size(); ++i)
    changed.push_back(state.PSbits[i] != oldstate.PSbits[i]);

  myPSRegister->setState(state.PSbits, changed);
  myPCLabel->setText(dbg.cartDebug().getLabel(state.PC, true));
}
