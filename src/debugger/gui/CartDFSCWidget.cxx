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
  items.push_back(" 0 ($FC0)");
  items.push_back(" 1 ($FC1)");
  items.push_back(" 2 ($FC2)");
  items.push_back(" 3 ($FC3)");
  items.push_back(" 4 ($FC4)");
  items.push_back(" 5 ($FC5)");
  items.push_back(" 6 ($FC6)");
  items.push_back(" 7 ($FC7)");
  items.push_back(" 8 ($FC8)");
  items.push_back(" 9 ($FC9)");
  items.push_back("10 ($FCA)");
  items.push_back("11 ($FCB)");
  items.push_back("12 ($FCC)");
  items.push_back("13 ($FCD)");
  items.push_back("14 ($FCE)");
  items.push_back("15 ($FCF)");
  items.push_back("16 ($FD0)");
  items.push_back("17 ($FD1)");
  items.push_back("18 ($FD2)");
  items.push_back("19 ($FD3)");
  items.push_back("20 ($FD4)");
  items.push_back("21 ($FD5)");
  items.push_back("22 ($FD6)");
  items.push_back("23 ($FD7)");
  items.push_back("24 ($FD8)");
  items.push_back("25 ($FD9)");
  items.push_back("26 ($FDA)");
  items.push_back("27 ($FDB)");
  items.push_back("28 ($FDC)");
  items.push_back("29 ($FDD)");
  items.push_back("30 ($FDE)");
  items.push_back("31 ($FDF)");

  myBank =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("31 ($FE0) "),
                    myLineHeight, items, "Set bank: ",
                    _font.getStringWidth("Set bank: "), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
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
