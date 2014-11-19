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

#include "CartBFSC.hxx"
#include "PopUpWidget.hxx"
#include "CartBFSCWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeBFSCWidget::CartridgeBFSCWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeBFSC& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  uInt32 size = 64 * 4096;

  ostringstream info;
  info << "256K BFSC + RAM, 64 4K banks\n"
       << "128 bytes RAM @ $F000 - $F0FF\n"
       << "  $F080 - $F0FF (R), $F000 - $F07F (W)\n"
       << "Startup bank = " << cart.myStartBank << "\n";

  // Eventually, we should query this from the debugger/disassembler
  for(uInt32 i = 0, offset = 0xFFC, spot = 0xF80; i < 64; ++i, offset += 0x1000)
  {
    uInt16 start = (cart.myImage[offset+1] << 8) | cart.myImage[offset];
    start -= start % 0x1000;
    info << "Bank " << dec << i << " @ $" << Common::Base::HEX4 << (start + 0x100)
         << " - " << "$" << (start + 0xFFF) << " (hotspot = $" << (spot+i) << ")\n";
  }

  int xpos = 10,
      ypos = addBaseInformation(size, "CPUWIZ", info.str()) + myLineHeight;

  VariantList items;
  VarList::push_back(items, " 0 ($F80)");
  VarList::push_back(items, " 1 ($F81)");
  VarList::push_back(items, " 2 ($F82)");
  VarList::push_back(items, " 3 ($F83)");
  VarList::push_back(items, " 4 ($F84)");
  VarList::push_back(items, " 5 ($F85)");
  VarList::push_back(items, " 6 ($F86)");
  VarList::push_back(items, " 7 ($F87)");
  VarList::push_back(items, " 8 ($F88)");
  VarList::push_back(items, " 9 ($F89)");
  VarList::push_back(items, "10 ($F8A)");
  VarList::push_back(items, "11 ($F8B)");
  VarList::push_back(items, "12 ($F8C)");
  VarList::push_back(items, "13 ($F8D)");
  VarList::push_back(items, "14 ($F8E)");
  VarList::push_back(items, "15 ($F8F)");
  VarList::push_back(items, "16 ($F90)");
  VarList::push_back(items, "17 ($F91)");
  VarList::push_back(items, "18 ($F92)");
  VarList::push_back(items, "19 ($F93)");
  VarList::push_back(items, "20 ($F94)");
  VarList::push_back(items, "21 ($F95)");
  VarList::push_back(items, "22 ($F96)");
  VarList::push_back(items, "23 ($F97)");
  VarList::push_back(items, "24 ($F98)");
  VarList::push_back(items, "25 ($F99)");
  VarList::push_back(items, "26 ($F9A)");
  VarList::push_back(items, "27 ($F9B)");
  VarList::push_back(items, "28 ($F9C)");
  VarList::push_back(items, "29 ($F9D)");
  VarList::push_back(items, "30 ($F9E)");
  VarList::push_back(items, "31 ($F9F)");
  VarList::push_back(items, "32 ($FA0)");
  VarList::push_back(items, "33 ($FA1)");
  VarList::push_back(items, "34 ($FA2)");
  VarList::push_back(items, "35 ($FA3)");
  VarList::push_back(items, "36 ($FA4)");
  VarList::push_back(items, "37 ($FA5)");
  VarList::push_back(items, "38 ($FA6)");
  VarList::push_back(items, "39 ($FA7)");
  VarList::push_back(items, "40 ($FA8)");
  VarList::push_back(items, "41 ($FA9)");
  VarList::push_back(items, "42 ($FAA)");
  VarList::push_back(items, "43 ($FAB)");
  VarList::push_back(items, "44 ($FAC)");
  VarList::push_back(items, "45 ($FAD)");
  VarList::push_back(items, "46 ($FAE)");
  VarList::push_back(items, "47 ($FAF)");
  VarList::push_back(items, "48 ($FB0)");
  VarList::push_back(items, "49 ($FB1)");
  VarList::push_back(items, "50 ($FB2)");
  VarList::push_back(items, "51 ($FB3)");
  VarList::push_back(items, "52 ($FB4)");
  VarList::push_back(items, "53 ($FB5)");
  VarList::push_back(items, "54 ($FB6)");
  VarList::push_back(items, "55 ($FB7)");
  VarList::push_back(items, "56 ($FB8)");
  VarList::push_back(items, "57 ($FB9)");
  VarList::push_back(items, "58 ($FBA)");
  VarList::push_back(items, "59 ($FBB)");
  VarList::push_back(items, "60 ($FBC)");
  VarList::push_back(items, "61 ($FBD)");
  VarList::push_back(items, "62 ($FBE)");
  VarList::push_back(items, "63 ($FBF)");

  myBank =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("63 ($FBF) "),
                    myLineHeight, items, "Set bank: ",
                    _font.getStringWidth("Set bank: "), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBFSCWidget::saveOldState()
{
  myOldState.internalram.clear();
  
  for(uInt32 i = 0; i < this->internalRamSize();i++)
  {
    myOldState.internalram.push_back(myCart.myRAM[i]);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBFSCWidget::loadConfig()
{
  myBank->setSelectedIndex(myCart.myCurrentBank);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBFSCWidget::handleCommand(CommandSender* sender,
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
string CartridgeBFSCWidget::bankState()
{
  ostringstream& buf = buffer();

  static const char* spot[] = {
    "$F80", "$F81", "$F82", "$F83", "$F84", "$F85", "$F86", "$F87",
    "$F88", "$F89", "$F8A", "$F8B", "$F8C", "$F8D", "$F8E", "$F8F",
    "$F90", "$F91", "$F92", "$F93", "$F94", "$F95", "$F96", "$F97",
    "$F98", "$F99", "$F9A", "$F9B", "$F9C", "$F9D", "$F9E", "$F9F",
    "$FA0", "$FA1", "$FA2", "$FA3", "$FA4", "$FA5", "$FA6", "$FA7",
    "$FA8", "$FA9", "$FAA", "$FAB", "$FAC", "$FAD", "$FAE", "$FAF",
    "$FB0", "$FB1", "$FB2", "$FB3", "$FB4", "$FB5", "$FB6", "$FB7",
    "$FB8", "$FB9", "$FBA", "$FBB", "$FBC", "$FBD", "$FBE", "$FBF"
  };
  buf << "Bank = " << dec << myCart.myCurrentBank
      << ", hotspot = " << spot[myCart.myCurrentBank];

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeBFSCWidget::internalRamSize() 
{
  return 128;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeBFSCWidget::internalRamRPort(int start)
{
  return 0xF080 + start;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeBFSCWidget::internalRamDescription() 
{
  ostringstream desc;
  desc << "$F000 - $F07F used for Write Access\n"
       << "$F080 - $F0FF used for Read Access";
  
  return desc.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeBFSCWidget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  for(int i = 0; i < count; i++)
    myRamOld.push_back(myOldState.internalram[start + i]);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeBFSCWidget::internalRamCurrent(int start, int count)
{
  myRamCurrent.clear();
  for(int i = 0; i < count; i++)
    myRamCurrent.push_back(myCart.myRAM[start + i]);
  return myRamCurrent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBFSCWidget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myRAM[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeBFSCWidget::internalRamGetValue(int addr)
{
  return myCart.myRAM[addr];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeBFSCWidget::internalRamLabel(int addr) 
{
  CartDebug& dbg = instance().debugger().cartDebug();
  return dbg.getLabel(addr + 0xF080, false);
}
