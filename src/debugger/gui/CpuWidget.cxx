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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <sstream>

#include "OSystem.hxx"
#include "GuiObject.hxx"
#include "Debugger.hxx"
#include "CartDebug.hxx"
#include "CpuDebug.hxx"
#include "Widget.hxx"
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
  int xpos, ypos, lwidth;

  // Create a 1x1 grid with label for the PC register
  xpos = x;  ypos = y;  lwidth = 4 * fontWidth;
  new StaticTextWidget(boss, lfont, xpos, ypos+1, lwidth-2, fontHeight,
                       "PC:", kTextAlignLeft);
  myPCGrid =
    new DataGridWidget(boss, nfont, xpos + lwidth, ypos, 1, 1, 4, 16, Common::Base::F_16);
  myPCGrid->setTarget(this);
  myPCGrid->setID(kPCRegID);
  addFocusWidget(myPCGrid);

  // Create a read-only textbox containing the current PC label
  xpos += lwidth + myPCGrid->getWidth() + 10;
  myPCLabel = new EditTextWidget(boss, nfont, xpos, ypos, (max_w - xpos + x) - 10,
                                 fontHeight+1, "");
  myPCLabel->setEditable(false);

  // Create a 1x4 grid with labels for the other CPU registers
  xpos = x + lwidth;  ypos += myPCGrid->getHeight() + 1;
  myCpuGrid =
    new DataGridWidget(boss, nfont, xpos, ypos, 1, 4, 2, 8, Common::Base::F_16);
  myCpuGrid->setTarget(this);
  myCpuGrid->setID(kCpuRegID);
  addFocusWidget(myCpuGrid);

  // Create a 1x4 grid with decimal and binary values for the other CPU registers
  xpos = x + lwidth + myPCGrid->getWidth() + 10;
  myCpuGridDecValue = 
    new DataGridWidget(boss, nfont, xpos, ypos, 1, 4, 3, 8, Common::Base::F_10);
  myCpuGridDecValue->setEditable(false);
  xpos += myCpuGridDecValue->getWidth() + 5;
  myCpuGridBinValue = 
    new DataGridWidget(boss, nfont, xpos, ypos, 1, 4, 8, 8, Common::Base::F_2);
  myCpuGridBinValue->setEditable(false);

  // Calculate real dimensions (_y will be calculated at the end)
  _w = lwidth + myPCGrid->getWidth() + myPCLabel->getWidth() + 20;

  // Create labels showing the source of data for SP/A/X/Y registers
  xpos += myCpuGridBinValue->getWidth() + 20;
  int src_y = ypos, src_w = (max_w - xpos + x) - 10;
  for(int i = 0; i < 4; ++i)
  {
    myCpuDataSrc[i] = new EditTextWidget(boss, nfont, xpos, src_y, src_w,
                                 fontHeight+1, "");
    myCpuDataSrc[i]->setEditable(false);
    src_y += fontHeight+2;
  }
  int swidth = lfont.getStringWidth("Source Address");
  new StaticTextWidget(boss, lfont, xpos, src_y + 4, src_w,
                       fontHeight, swidth <= src_w ? "Source Address" : "Source Addr",
                       kTextAlignCenter);

  // Add labels for other CPU registers
  xpos = x;
  string labels[4] = { "SP:", "A:", "X:", "Y:" };
  for(int row = 0; row < 4; ++row)
  {
    new StaticTextWidget(boss, lfont, xpos, ypos + row*lineHeight + 1,
                         lwidth-2, fontHeight,
                         labels[row], kTextAlignLeft);
  }

  // Create a bitfield widget for changing the processor status
  xpos = x;  ypos += 4*lineHeight + 2;
  new StaticTextWidget(boss, lfont, xpos, ypos+1, lwidth-2, fontHeight,
                       "PS:", kTextAlignLeft);
  myPSRegister = new ToggleBitWidget(boss, nfont, xpos+lwidth, ypos, 8, 1);
  myPSRegister->setTarget(this);
  addFocusWidget(myPSRegister);

  // Set the strings to be used in the PSRegister
  // We only do this once because it's the state that changes, not the strings
  const char* offstr[] = { "n", "v", "-", "b", "d", "i", "z", "c" };
  const char* onstr[]  = { "N", "V", "-", "B", "D", "I", "Z", "C" };
  StringList off, on;
  for(int i = 0; i < 8; ++i)
  {
    off.push_back(offstr[i]);
    on.push_back(onstr[i]);
  }
  myPSRegister->setList(off, on);

  _h = ypos + myPSRegister->getHeight() - y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CpuWidget::~CpuWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuWidget::setOpsWidget(DataGridOpsWidget* w)
{
  myPCGrid->setOpsWidget(w);
  myCpuGrid->setOpsWidget(w);
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
      }

      switch(addr)
      {
        case kPCRegAddr:
        {
          // Use the parser to set PC, since we want to propagate the
          // event the rest of the debugger widgets
          ostringstream command;
          command << "pc #" << value;
          instance().debugger().run(command.str());
          break;
        }

        case kSPRegAddr:
          dbg.setSP(value);
          myCpuGridDecValue->setValue(0, value);
          myCpuGridBinValue->setValue(0, value);
          break;

        case kARegAddr:
          dbg.setA(value);
          myCpuGridDecValue->setValue(1, value);
          myCpuGridBinValue->setValue(1, value);
          break;

        case kXRegAddr:
          dbg.setX(value);
          myCpuGridDecValue->setValue(2, value);
          myCpuGridBinValue->setValue(2, value);
          break;

        case kYRegAddr:
          dbg.setY(value);
          myCpuGridDecValue->setValue(3, value);
          myCpuGridBinValue->setValue(3, value);
          break;
      }
      break;

    case ToggleWidget::kItemDataChangedCmd:
    {
      bool state = myPSRegister->getSelectedState();

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
      }
    }
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
  Debugger& dbg = instance().debugger();
  CartDebug& cart = dbg.cartDebug();
  CpuDebug& cpu = dbg.cpuDebug();
  const CpuState& state    = (CpuState&) cpu.getState();
  const CpuState& oldstate = (CpuState&) cpu.getOldState();

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
  const string& srcS = state.srcS < 0 ? "IMM" : cart.getLabel(state.srcS, true);
  myCpuDataSrc[0]->setText((srcS != EmptyString ? srcS : Common::Base::toString(state.srcS)),
                           state.srcS != oldstate.srcS);
  const string& srcA = state.srcA < 0 ? "IMM" : cart.getLabel(state.srcA, true);
  myCpuDataSrc[1]->setText((srcA != EmptyString ? srcA : Common::Base::toString(state.srcA)),
                           state.srcA != oldstate.srcA);
  const string& srcX = state.srcX < 0 ? "IMM" : cart.getLabel(state.srcX, true);
  myCpuDataSrc[2]->setText((srcX != EmptyString ? srcX : Common::Base::toString(state.srcX)),
                           state.srcX != oldstate.srcX);
  const string& srcY = state.srcY < 0 ? "IMM" : cart.getLabel(state.srcY, true);
  myCpuDataSrc[3]->setText((srcY != EmptyString ? srcY : Common::Base::toString(state.srcY)),
                           state.srcY != oldstate.srcY);

  // Update the PS register booleans
  changed.clear();
  for(unsigned int i = 0; i < state.PSbits.size(); ++i)
    changed.push_back(state.PSbits[i] != oldstate.PSbits[i]);

  myPSRegister->setState(state.PSbits, changed);
  myPCLabel->setText(dbg.cartDebug().getLabel(state.PC, true));
}
