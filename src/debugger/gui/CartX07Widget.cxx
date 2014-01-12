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

#include "CartX07.hxx"
#include "PopUpWidget.hxx"
#include "CartX07Widget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeX07Widget::CartridgeX07Widget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeX07& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  uInt32 size = 16 * 4096;

  ostringstream info;
  info << "64K X07 cartridge, 16 4K banks\n"
       << "Startup bank = " << cart.myStartBank << "\n"
       << "Multiple hotspots, all below $1000\n"
       << "See documentation for further details\n";

  // Eventually, we should query this from the debugger/disassembler
  for(uInt32 i = 0, offset = 0xFFC; i < 16; ++i, offset += 0x1000)
  {
    uInt16 start = (cart.myImage[offset+1] << 8) | cart.myImage[offset];
    start -= start % 0x1000;
    info << "Bank " << dec << i << " @ $" << Common::Base::HEX4 << start
         << " - " << "$" << (start + 0xFFF) << "\n";
  }

  int xpos = 10,
      ypos = addBaseInformation(size, "AtariAge / John Payson / Fred Quimby",
                                info.str()) + myLineHeight;

  VariantList items;
  items.push_back("  0");
  items.push_back("  1");
  items.push_back("  2");
  items.push_back("  3");
  items.push_back("  4");
  items.push_back("  5");
  items.push_back("  6");
  items.push_back("  7");
  items.push_back("  8");
  items.push_back("  9");
  items.push_back(" 10");
  items.push_back(" 11");
  items.push_back(" 12");
  items.push_back(" 13");
  items.push_back(" 14");
  items.push_back(" 15");
  myBank =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth(" 15 "),
                    myLineHeight, items, "Set bank: ",
                    _font.getStringWidth("Set bank: "), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeX07Widget::loadConfig()
{
  myBank->setSelectedIndex(myCart.myCurrentBank);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeX07Widget::handleCommand(CommandSender* sender,
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
string CartridgeX07Widget::bankState()
{
  ostringstream& buf = buffer();

  buf << "Bank = " << dec << myCart.myCurrentBank;

  return buf.str();
}
