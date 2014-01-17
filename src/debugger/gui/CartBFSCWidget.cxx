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
  items.push_back(" 0 ($F80)");
  items.push_back(" 1 ($F81)");
  items.push_back(" 2 ($F82)");
  items.push_back(" 3 ($F83)");
  items.push_back(" 4 ($F84)");
  items.push_back(" 5 ($F85)");
  items.push_back(" 6 ($F86)");
  items.push_back(" 7 ($F87)");
  items.push_back(" 8 ($F88)");
  items.push_back(" 9 ($F89)");
  items.push_back("10 ($F8A)");
  items.push_back("11 ($F8B)");
  items.push_back("12 ($F8C)");
  items.push_back("13 ($F8D)");
  items.push_back("14 ($F8E)");
  items.push_back("15 ($F8F)");
  items.push_back("16 ($F90)");
  items.push_back("17 ($F91)");
  items.push_back("18 ($F92)");
  items.push_back("19 ($F93)");
  items.push_back("20 ($F94)");
  items.push_back("21 ($F95)");
  items.push_back("22 ($F96)");
  items.push_back("23 ($F97)");
  items.push_back("24 ($F98)");
  items.push_back("25 ($F99)");
  items.push_back("26 ($F9A)");
  items.push_back("27 ($F9B)");
  items.push_back("28 ($F9C)");
  items.push_back("29 ($F9D)");
  items.push_back("30 ($F9E)");
  items.push_back("31 ($F9F)");
  items.push_back("32 ($FA0)");
  items.push_back("33 ($FA1)");
  items.push_back("34 ($FA2)");
  items.push_back("35 ($FA3)");
  items.push_back("36 ($FA4)");
  items.push_back("37 ($FA5)");
  items.push_back("38 ($FA6)");
  items.push_back("39 ($FA7)");
  items.push_back("40 ($FA8)");
  items.push_back("41 ($FA9)");
  items.push_back("42 ($FAA)");
  items.push_back("43 ($FAB)");
  items.push_back("44 ($FAC)");
  items.push_back("45 ($FAD)");
  items.push_back("46 ($FAE)");
  items.push_back("47 ($FAF)");
  items.push_back("48 ($FB0)");
  items.push_back("49 ($FB1)");
  items.push_back("50 ($FB2)");
  items.push_back("51 ($FB3)");
  items.push_back("52 ($FB4)");
  items.push_back("53 ($FB5)");
  items.push_back("54 ($FB6)");
  items.push_back("55 ($FB7)");
  items.push_back("56 ($FB8)");
  items.push_back("57 ($FB9)");
  items.push_back("58 ($FBA)");
  items.push_back("59 ($FBB)");
  items.push_back("60 ($FBC)");
  items.push_back("61 ($FBD)");
  items.push_back("62 ($FBE)");
  items.push_back("63 ($FBF)");

  myBank =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("63 ($FBF) "),
                    myLineHeight, items, "Set bank: ",
                    _font.getStringWidth("Set bank: "), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
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
