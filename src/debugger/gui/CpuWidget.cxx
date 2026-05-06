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

#include "CpuWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CpuWidget::CpuWidget(GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
                     int x, int y, int max_w)
  : Widget(boss, lfont, x, y, 16, 16),
    CommandSender(boss)
{
  const int fontWidth  = lfont.getMaxCharWidth(),
            fontHeight = lfont.getFontHeight(),
            lineHeight = lfont.getLineHeight();
  constexpr int VGAP = 2;
  const int lwidth = 4 * fontWidth;

  // Create a 1x1 grid with label for the PC register
  int xpos = x, ypos = y;
  new StaticTextWidget(boss, lfont, xpos, ypos + 2, lwidth-2, fontHeight,
                       "PC ", TextAlign::Left);
  myPCGrid =
    new DataGridWidget(boss, nfont, xpos + lwidth, ypos, 1, 1, 4, 16, Common::Base::Fmt::_16);
  myPCGrid->setHelpAnchor("CPURegisters", true);
  myPCGrid->setTarget(this);
  myPCGrid->setID(kPCRegID);
  addFocusWidget(myPCGrid);

  // Create a read-only textbox containing the current PC label
  xpos += lwidth + myPCGrid->getWidth() + 10;
  myPCLabel = new EditTextWidget(boss, nfont, xpos, ypos, (max_w - xpos + x) - 10,
                                 fontHeight+1, "");
  myPCLabel->setEditable(false, true);

  // Create a 1x4 grid with labels for the other CPU registers
  xpos = x + lwidth;  ypos = myPCGrid->getBottom() + VGAP;
  myCpuGrid =
    new DataGridWidget(boss, nfont, xpos, ypos, 1, 4, 2, 8, Common::Base::Fmt::_16);
  myCpuGrid->setHelpAnchor("CPURegisters", true);
  myCpuGrid->setTarget(this);
  myCpuGrid->setID(kCpuRegID);
  addFocusWidget(myCpuGrid);

  // Create a 1x4 grid with decimal and binary values for the other CPU registers
  xpos = myPCGrid->getRight() + 10;
  myCpuGridDecValue =
    new DataGridWidget(boss, nfont, xpos, ypos, 1, 4, 3, 8, Common::Base::Fmt::_10);
  myCpuGridDecValue->setHelpAnchor("CPURegisters", true);
  myCpuGridDecValue->setTarget(this);
  myCpuGridDecValue->setID(kCpuRegDecID);
  addFocusWidget(myCpuGridDecValue);

  xpos = myCpuGridDecValue->getRight() + fontWidth * 2;
  myCpuGridBinValue =
    new DataGridWidget(boss, nfont, xpos, ypos, 1, 4, 8, 8, Common::Base::Fmt::_2);
  myCpuGridBinValue->setHelpAnchor("CPURegisters", true);
  myCpuGridBinValue->setTarget(this);
  myCpuGridBinValue->setID(kCpuRegBinID);
  addFocusWidget(myCpuGridBinValue);

  // Calculate real dimensions (_y will be calculated at the end)
  _w = lwidth + myPCGrid->getWidth() + myPCLabel->getWidth() + 20;

  // Create labels showing the source of data for SP/A/X/Y registers
  const std::array<string, 4> labels = { "SP", "A", "X", "Y" };
  xpos += myCpuGridBinValue->getWidth() + 20;
  const int src_w = (max_w - xpos + x) - 10;
  for(int i = 0, src_y = ypos; i < 4; ++i)
  {
    myCpuDataSrc[i] = new EditTextWidget(boss, nfont, xpos, src_y, src_w, fontHeight + 1);
    myCpuDataSrc[i]->setToolTip("Source label of last read for " + labels[i] + ".");
    myCpuDataSrc[i]->setEditable(false, true);
    src_y += fontHeight + 2;
  }

  // Add labels for other CPU registers
  xpos = x;
  for(int row = 0; row < 4; ++row)
  {
    new StaticTextWidget(boss, lfont, xpos, ypos + row*lineHeight + 2,
                         lwidth-2, fontHeight,
                         labels[row], TextAlign::Left);
  }

  // Add prefixes for decimal and binary values
  for(int row = 0; row < 4; ++row)
  {
    new StaticTextWidget(boss, lfont, myCpuGridDecValue->getLeft() - fontWidth,
                         ypos + row * lineHeight + 2,
                         fontWidth, fontHeight, "#");
    new StaticTextWidget(boss, lfont, myCpuGridBinValue->getLeft() - fontWidth,
                         ypos + row * lineHeight + 2,
                         fontWidth, fontHeight, "%");
  }

  // Create a bitfield widget for changing the processor status
  xpos = x;  ypos = myCpuGrid->getBottom() + VGAP;
  new StaticTextWidget(boss, lfont, xpos, ypos + 2, lwidth-2, fontHeight,
                       "PS ", TextAlign::Left);
  myPSRegister = new ToggleBitWidget(boss, nfont, xpos+lwidth, ypos, 8, 1);
  myPSRegister->setHelpAnchor("CPURegisters", true);
  myPSRegister->setTarget(this);
  addFocusWidget(myPSRegister);

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

  // Last write destination address
  xpos = myCpuDataSrc[0]->getLeft();
  new StaticTextWidget(boss, lfont, xpos - fontWidth * 4.5, ypos + 2, "Dest");
  myCpuDataDest = new EditTextWidget(boss, nfont, xpos, ypos, src_w, fontHeight + 1);
  myCpuDataDest->setToolTip("Destination label of last write.");
  myCpuDataDest->setEditable(false, true);

  setHelpAnchor("DataOpButtons", true);

  _h = ypos + myPSRegister->getHeight() - y;
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
