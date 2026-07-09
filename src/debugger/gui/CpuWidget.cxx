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
CpuWidget::CpuWidget(GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
                     int x, int y, int max_w)
  : Widget(boss, lfont, x, y, 16, 16),
    CommandSender(boss)
{
  const int fontHeight = lfont.getFontHeight();
  const std::array<string, 4> labels = { "SP", "A", "X", "Y" };

  // Create every widget at a placeholder position/size; reflow() positions and
  // sizes them for the available width
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  myPCText = new StaticTextWidget(boss, lfont, 0, 0, "PC ", TextAlign::Left);
  myPCGrid = new DataGridWidget(boss, nfont, 0, 0, 1, 1, 4, 16, Common::Base::Fmt::_16);
  myPCGrid->setHelpAnchor("CPURegisters", true);
  myPCGrid->setTarget(this);
  myPCGrid->setID(kPCRegID);
  addFocusWidget(myPCGrid);

  // Read-only textbox containing the current PC label
  myPCLabel = new EditTextWidget(boss, nfont, 0, 0, 1, fontHeight + 1, "");
  myPCLabel->setEditable(false, true);

  // 1x4 grid with labels for the other CPU registers
  myCpuGrid = new DataGridWidget(boss, nfont, 0, 0, 1, 4, 2, 8, Common::Base::Fmt::_16);
  myCpuGrid->setHelpAnchor("CPURegisters", true);
  myCpuGrid->setTarget(this);
  myCpuGrid->setID(kCpuRegID);
  addFocusWidget(myCpuGrid);

  // 1x4 grids with the decimal and binary values for those registers
  myCpuGridDecValue = new DataGridWidget(boss, nfont, 0, 0, 1, 4, 3, 8, Common::Base::Fmt::_10);
  myCpuGridDecValue->setHelpAnchor("CPURegisters", true);
  myCpuGridDecValue->setTarget(this);
  myCpuGridDecValue->setID(kCpuRegDecID);
  addFocusWidget(myCpuGridDecValue);

  myCpuGridBinValue = new DataGridWidget(boss, nfont, 0, 0, 1, 4, 8, 8, Common::Base::Fmt::_2);
  myCpuGridBinValue->setHelpAnchor("CPURegisters", true);
  myCpuGridBinValue->setTarget(this);
  myCpuGridBinValue->setID(kCpuRegBinID);
  addFocusWidget(myCpuGridBinValue);

  // Labels showing the source of data for the SP/A/X/Y registers
  for(int i = 0; i < 4; ++i)
  {
    myCpuDataSrc[i] = new EditTextWidget(boss, nfont, 0, 0, 1, fontHeight + 1);
    myCpuDataSrc[i]->setToolTip("Source label of last read for " + labels[i] + ".");
    myCpuDataSrc[i]->setEditable(false, true);
  }

  // Row labels for the other CPU registers and the '#'/'%' value prefixes
  for(int row = 0; row < 4; ++row)
  {
    myRegLabels[row] = new StaticTextWidget(boss, lfont, 0, 0, labels[row], TextAlign::Left);
    myDecPrefix[row] = new StaticTextWidget(boss, lfont, 0, 0, "#");
    myBinPrefix[row] = new StaticTextWidget(boss, lfont, 0, 0, "%");
  }

  // Bitfield widget for changing the processor status
  myPSText = new StaticTextWidget(boss, lfont, 0, 0, "PS ", TextAlign::Left);
  myPSRegister = new ToggleBitWidget(boss, nfont, 0, 0, 8, 1);
  myPSRegister->setHelpAnchor("CPURegisters", true);
  myPSRegister->setTarget(this);
  addFocusWidget(myPSRegister);

  // Last write destination address
  myDestText = new StaticTextWidget(boss, lfont, 0, 0, "Dest");
  myCpuDataDest = new EditTextWidget(boss, nfont, 0, 0, 1, fontHeight + 1);
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

  reflow(max_w);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuWidget::setArea(int x, int y, int w, int h)
{
  setPos(x, y);
  reflow(w);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuWidget::reflow(int max_w)
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::labelColumn;
  using GUI::vCentered;
  using Dir = BoxLayout::Dir;

  const int fontWidth  = _font.getMaxCharWidth(),
            lineHeight = _font.getLineHeight();
  constexpr int VGAP = 2;
  const int lwidth = 4 * fontWidth;
  const int x = _x, y = _y;

  // An edit field restores its own framed height on a font change, but these
  // are a pixel shorter, so that four of them stack onto four grid rows
  const int editHeight = _font.getFontHeight() + 3;

  myPCLabel->setHeight(editHeight);
  for(auto* w: myCpuDataSrc)
    w->setHeight(editHeight);
  myCpuDataDest->setHeight(editHeight);

  const int pcGridW  = myPCGrid->getWidth(),
            hexGridW = myCpuGrid->getWidth(),
            decGridW = myCpuGridDecValue->getWidth(),
            binGridW = myCpuGridBinValue->getWidth();

  // PC row: "PC" label, PC grid, then the current-PC label filling the rest
  BoxLayout pcRow(Dir::Horizontal);
  pcRow.addFixed(labelColumn(myPCText, myPCGrid), lwidth);
  pcRow.addFixed(anchoredItem(myPCGrid), pcGridW);
  pcRow.addSpace(10);
  pcRow.addStretch(vCentered(myPCLabel, myPCLabel->getHeight()));
  pcRow.addSpace(10);
  pcRow.doLayout(x, y, max_w, myPCGrid->getHeight());

  // Register block: label | hex | # dec | % bin | data source, the last of
  // which stretches to fill the remaining width.  Each per-row column is a
  // 4-row vertical box that lines up with the (4-row) data grids beside it.
  // A grid insets each row's text, so the labels beside it start that far down
  const auto column = [&](const std::array<StaticTextWidget*, 4>& cells) {
    auto col = std::make_unique<BoxLayout>(Dir::Vertical);
    col->addSpace(myCpuGrid->textOffsetY());
    for(auto* w: cells)
      col->addFixed(anchoredItem(w), lineHeight);
    return col;
  };
  auto srcCol = std::make_unique<BoxLayout>(Dir::Vertical);
  for(auto* w: myCpuDataSrc)
    srcCol->addFixed(vCentered(w, w->getHeight()), lineHeight);

  const int regY = myPCGrid->getBottom() + VGAP;
  BoxLayout regRow(Dir::Horizontal);
  regRow.addFixed(column(myRegLabels), lwidth);
  regRow.addFixed(anchoredItem(myCpuGrid), hexGridW);
  regRow.addSpace(pcGridW - hexGridW + 10 - fontWidth);
  regRow.addFixed(column(myDecPrefix), fontWidth);
  regRow.addFixed(anchoredItem(myCpuGridDecValue), decGridW);
  regRow.addSpace(fontWidth);
  regRow.addFixed(column(myBinPrefix), fontWidth);
  regRow.addFixed(anchoredItem(myCpuGridBinValue), binGridW);
  regRow.addSpace(20);
  regRow.addStretch(std::move(srcCol));
  regRow.addSpace(10);
  regRow.doLayout(x, regY, max_w, myCpuGrid->getHeight());

  // PS row: "PS" label and the processor-status toggle
  const int psY = myCpuGrid->getBottom() + VGAP;
  BoxLayout psRow(Dir::Horizontal);
  psRow.addFixed(labelColumn(myPSText, myPSRegister), lwidth);
  psRow.addFixed(anchoredItem(myPSRegister), myPSRegister->getWidth());
  psRow.doLayout(x, psY, max_w, myPSRegister->getHeight());

  // The "Dest" label and destination edit align under the data-source column
  // resolved above (a cross-reference the box layout does not express)
  const int srcX = myCpuDataSrc[0]->getLeft(),
            srcW = myCpuDataSrc[0]->getWidth();
  myDestText->setPos(srcX - fontWidth * 4.5, psY + myCpuDataDest->textOffsetY());
  myCpuDataDest->setPos(srcX, psY);
  myCpuDataDest->setWidth(srcW);

  _w = max_w;
  _h = myPSRegister->getBottom() - y;
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
