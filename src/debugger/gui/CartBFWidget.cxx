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

#include "CartBF.hxx"
#include "PopUpWidget.hxx"
#include "CartBFWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeBFWidget::CartridgeBFWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeBF& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  uInt32 size = 64 * 4096;

  ostringstream info;
  info << "BF cartridge, 64 4K banks\n"
       << "Startup bank = " << cart.myStartBank << "\n";

  // Eventually, we should query this from the debugger/disassembler
  for(uInt32 i = 0, offset = 0xFFC, spot = 0xF80; i < 64; ++i, offset += 0x1000)
  {
    uInt16 start = (cart.myImage[offset+1] << 8) | cart.myImage[offset];
    start -= start % 0x1000;
    info << "Bank " << dec << i << " @ $" << Common::Base::HEX4 << start << " - "
         << "$" << (start + 0xFFF) << " (hotspot = $" << (spot+i) << ")\n";
  }

  int xpos = 10,
      ypos = addBaseInformation(size, "CPUWIZ", info.str()) + myLineHeight;

  VariantList items;
  VList::push_back(items, " 0 ($F80)");
  VList::push_back(items, " 1 ($F81)");
  VList::push_back(items, " 2 ($F82)");
  VList::push_back(items, " 3 ($F83)");
  VList::push_back(items, " 4 ($F84)");
  VList::push_back(items, " 5 ($F85)");
  VList::push_back(items, " 6 ($F86)");
  VList::push_back(items, " 7 ($F87)");
  VList::push_back(items, " 8 ($F88)");
  VList::push_back(items, " 9 ($F89)");
  VList::push_back(items, "10 ($F8A)");
  VList::push_back(items, "11 ($F8B)");
  VList::push_back(items, "12 ($F8C)");
  VList::push_back(items, "13 ($F8D)");
  VList::push_back(items, "14 ($F8E)");
  VList::push_back(items, "15 ($F8F)");
  VList::push_back(items, "16 ($F90)");
  VList::push_back(items, "17 ($F91)");
  VList::push_back(items, "18 ($F92)");
  VList::push_back(items, "19 ($F93)");
  VList::push_back(items, "20 ($F94)");
  VList::push_back(items, "21 ($F95)");
  VList::push_back(items, "22 ($F96)");
  VList::push_back(items, "23 ($F97)");
  VList::push_back(items, "24 ($F98)");
  VList::push_back(items, "25 ($F99)");
  VList::push_back(items, "26 ($F9A)");
  VList::push_back(items, "27 ($F9B)");
  VList::push_back(items, "28 ($F9C)");
  VList::push_back(items, "29 ($F9D)");
  VList::push_back(items, "30 ($F9E)");
  VList::push_back(items, "31 ($F9F)");
  VList::push_back(items, "32 ($FA0)");
  VList::push_back(items, "33 ($FA1)");
  VList::push_back(items, "34 ($FA2)");
  VList::push_back(items, "35 ($FA3)");
  VList::push_back(items, "36 ($FA4)");
  VList::push_back(items, "37 ($FA5)");
  VList::push_back(items, "38 ($FA6)");
  VList::push_back(items, "39 ($FA7)");
  VList::push_back(items, "40 ($FA8)");
  VList::push_back(items, "41 ($FA9)");
  VList::push_back(items, "42 ($FAA)");
  VList::push_back(items, "43 ($FAB)");
  VList::push_back(items, "44 ($FAC)");
  VList::push_back(items, "45 ($FAD)");
  VList::push_back(items, "46 ($FAE)");
  VList::push_back(items, "47 ($FAF)");
  VList::push_back(items, "48 ($FB0)");
  VList::push_back(items, "49 ($FB1)");
  VList::push_back(items, "50 ($FB2)");
  VList::push_back(items, "51 ($FB3)");
  VList::push_back(items, "52 ($FB4)");
  VList::push_back(items, "53 ($FB5)");
  VList::push_back(items, "54 ($FB6)");
  VList::push_back(items, "55 ($FB7)");
  VList::push_back(items, "56 ($FB8)");
  VList::push_back(items, "57 ($FB9)");
  VList::push_back(items, "58 ($FBA)");
  VList::push_back(items, "59 ($FBB)");
  VList::push_back(items, "60 ($FBC)");
  VList::push_back(items, "61 ($FBD)");
  VList::push_back(items, "62 ($FBE)");
  VList::push_back(items, "63 ($FBF)");
 
  myBank =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("64 ($FBF) "),
                    myLineHeight, items, "Set bank: ",
                    _font.getStringWidth("Set bank: "), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBFWidget::loadConfig()
{
  myBank->setSelectedIndex(myCart.myCurrentBank);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBFWidget::handleCommand(CommandSender* sender,
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
string CartridgeBFWidget::bankState()
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
