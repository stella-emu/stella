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
// Copyright (c) 1995-2013 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "CartEFSC.hxx"
#include "PopUpWidget.hxx"
#include "CartEFSCWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeEFSCWidget::CartridgeEFSCWidget(
      GuiObject* boss, const GUI::Font& font,
      int x, int y, int w, int h, CartridgeEFSC& cart)
  : CartDebugWidget(boss, font, x, y, w, h),
    myCart(cart)
{
  uInt32 size = 16 * 4096;

  ostringstream info;
  info << "64K H. Runner EFSC + RAM, 16 4K banks\n"
       << "128 bytes RAM @ $F000 - $F0FF\n"
       << "  $F080 - $F0FF (R), $F000 - $F07F (W)\n"
       << "Startup bank = " << cart.myStartBank << "\n";

  // Eventually, we should query this from the debugger/disassembler
  for(uInt32 i = 0, offset = 0xFFC, spot = 0xFE0; i < 16; ++i, offset += 0x1000)
  {
    uInt16 start = (cart.myImage[offset+1] << 8) | cart.myImage[offset];
    start -= start % 0x1000;
    info << "Bank " << dec << i << " @ $" << HEX4 << start << " - "
         << "$" << (start + 0xFFF) << " (hotspot = $" << (spot+i) << ")\n";
  }

  int xpos = 10,
      ypos = addBaseInformation(size, "Paul Slocum / Homestar Runner",
                                info.str()) + myLineHeight;

  StringMap items;
  items.push_back(" 0 ($FE0)", "0");
  items.push_back(" 1 ($FE1)", "1");
  items.push_back(" 2 ($FE2)", "2");
  items.push_back(" 3 ($FE3)", "3");
  items.push_back(" 4 ($FE4)", "4");
  items.push_back(" 5 ($FE5)", "5");
  items.push_back(" 6 ($FE6)", "6");
  items.push_back(" 7 ($FE7)", "7");
  items.push_back(" 8 ($FE8)", "8");
  items.push_back(" 9 ($FE9)", "9");
  items.push_back("10 ($FEA)", "10");
  items.push_back("11 ($FEB)", "11");
  items.push_back("12 ($FEC)", "12");
  items.push_back("13 ($FED)", "13");
  items.push_back("14 ($FEE)", "14");
  items.push_back("15 ($FEF)", "15");
  myBank =
    new PopUpWidget(boss, font, xpos, ypos-2, font.getStringWidth("15 ($FE0) "),
                    myLineHeight, items, "Set bank: ",
                    font.getStringWidth("Set bank: "), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEFSCWidget::loadConfig()
{
  myBank->setSelected(myCart.myCurrentBank);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEFSCWidget::handleCommand(CommandSender* sender,
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
