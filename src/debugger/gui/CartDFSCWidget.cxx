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

#include "CartDFSC.hxx"
#include "PopUpWidget.hxx"
#include "CartDFSCWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDFSCWidget::CartridgeDFSCWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeDFSC& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  uInt32 size = 32 * 4096;

  ostringstream info;
  info << "128K DFSC + RAM, 32 4K banks\n"
       << "128 bytes RAM @ $F000 - $F0FF\n"
       << "  $F080 - $F0FF (R), $F000 - $F07F (W)\n"
       << "Startup bank = " << cart.myStartBank << "\n";

  // Eventually, we should query this from the debugger/disassembler
  for(uInt32 i = 0, offset = 0xFFC, spot = 0xFC0; i < 32; ++i, offset += 0x1000)
  {
    uInt16 start = (cart.myImage[offset+1] << 8) | cart.myImage[offset];
    start -= start % 0x1000;
    info << "Bank " << dec << i << " @ $" << Common::Base::HEX4 << (start + 0x100)
         << " - " << "$" << (start + 0xFFF) << " (hotspot = $" << (spot+i) << ")\n";
  }

  int xpos = 10,
      ypos = addBaseInformation(size, "CPUWIZ", info.str()) + myLineHeight;

  VariantList items;
  VList::push_back(items, " 0 ($FC0)");
  VList::push_back(items, " 1 ($FC1)");
  VList::push_back(items, " 2 ($FC2)");
  VList::push_back(items, " 3 ($FC3)");
  VList::push_back(items, " 4 ($FC4)");
  VList::push_back(items, " 5 ($FC5)");
  VList::push_back(items, " 6 ($FC6)");
  VList::push_back(items, " 7 ($FC7)");
  VList::push_back(items, " 8 ($FC8)");
  VList::push_back(items, " 9 ($FC9)");
  VList::push_back(items, "10 ($FCA)");
  VList::push_back(items, "11 ($FCB)");
  VList::push_back(items, "12 ($FCC)");
  VList::push_back(items, "13 ($FCD)");
  VList::push_back(items, "14 ($FCE)");
  VList::push_back(items, "15 ($FCF)");
  VList::push_back(items, "16 ($FD0)");
  VList::push_back(items, "17 ($FD1)");
  VList::push_back(items, "18 ($FD2)");
  VList::push_back(items, "19 ($FD3)");
  VList::push_back(items, "20 ($FD4)");
  VList::push_back(items, "21 ($FD5)");
  VList::push_back(items, "22 ($FD6)");
  VList::push_back(items, "23 ($FD7)");
  VList::push_back(items, "24 ($FD8)");
  VList::push_back(items, "25 ($FD9)");
  VList::push_back(items, "26 ($FDA)");
  VList::push_back(items, "27 ($FDB)");
  VList::push_back(items, "28 ($FDC)");
  VList::push_back(items, "29 ($FDD)");
  VList::push_back(items, "30 ($FDE)");
  VList::push_back(items, "31 ($FDF)");

  myBank =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("31 ($FE0) "),
                    myLineHeight, items, "Set bank: ",
                    _font.getStringWidth("Set bank: "), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDFSCWidget::saveOldState()
{
  myOldState.internalram.clear();
  
  for(uInt32 i = 0; i < this->internalRamSize();i++)
  {
    myOldState.internalram.push_back(myCart.myRAM[i]);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDFSCWidget::loadConfig()
{
  myBank->setSelectedIndex(myCart.myCurrentBank);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDFSCWidget::handleCommand(CommandSender* sender,
                                      int cmd, int data, int id)
{
  if(cmd == kBankChanged)
  {
    myCart.unlockBank();
    myCart.bank(myBank->getSelected());
    myCart.lockBank();
    invalidate();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeDFSCWidget::bankState()
{
  ostringstream& buf = buffer();

  static const char* spot[] = {
    "$FC0", "$FC1", "$FC2", "$FC3", "$FC4", "$FC5", "$FC6", "$FC7",
    "$FC8", "$FC9", "$FCA", "$FCB", "$FCC", "$FCD", "$FCE", "$FCF",
    "$FD0", "$FD1", "$FD2", "$FD3", "$FD4", "$FD5", "$FD6", "$FE7",
    "$FD8", "$FD9", "$FDA", "$FDB", "$FDC", "$FDD", "$FDE", "$FDF"
  };
  buf << "Bank = " << dec << myCart.myCurrentBank
      << ", hotspot = " << spot[myCart.myCurrentBank];

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeDFSCWidget::internalRamSize() 
{
  return 128;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeDFSCWidget::internalRamRPort(int start)
{
  return 0xF080 + start;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeDFSCWidget::internalRamDescription() 
{
  ostringstream desc;
  desc << "$F000 - $F07F used for Write Access\n"
       << "$F080 - $F0FF used for Read Access";
  
  return desc.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeDFSCWidget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  for(int i = 0; i < count; i++)
    myRamOld.push_back(myOldState.internalram[start + i]);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeDFSCWidget::internalRamCurrent(int start, int count)
{
  myRamCurrent.clear();
  for(int i = 0; i < count; i++)
    myRamCurrent.push_back(myCart.myRAM[start + i]);
  return myRamCurrent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDFSCWidget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myRAM[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeDFSCWidget::internalRamGetValue(int addr)
{
  return myCart.myRAM[addr];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeDFSCWidget::internalRamLabel(int addr) 
{
  CartDebug& dbg = instance().debugger().cartDebug();
  return dbg.getLabel(addr + 0xF080, false);
}
