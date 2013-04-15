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

#include "CartSB.hxx"
#include "PopUpWidget.hxx"
#include "CartSBWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeSBWidget::CartridgeSBWidget(
      GuiObject* boss, const GUI::Font& font,
      int x, int y, int w, int h, CartridgeSB& cart)
  : CartDebugWidget(boss, font, x, y, w, h),
    myCart(cart)
{
  uInt32 size = myCart.mySize;

  StringMap items;
  ostringstream info, bank;
  info << "SB SUPERbanking, 32 or 64 4K banks\n"
       << "Hotspots are from $800 to $"
       << HEX2 << (0x800 + myCart.bankCount() - 1) << ", including\n"
       << "mirrors ($900, $A00, $B00, ...)\n"
       << "Startup bank = " << dec << cart.myStartBank << "\n";

  // Eventually, we should query this from the debugger/disassembler
  for(uInt32 i = 0, offset = 0xFFC, spot = 0x800; i < myCart.bankCount();
      ++i, offset += 0x1000, ++spot)
  {
    uInt16 start = (cart.myImage[offset+1] << 8) | cart.myImage[offset];
    start -= start % 0x1000;
    info << "Bank " << dec << i << " @ $" << HEX4 << start << " - "
         << "$" << (start + 0xFFF) << " (hotspot = $" << spot << ")\n";

    bank << dec << setw(2) << setfill(' ') << i << " ($" << HEX2 << spot << ")";
    items.push_back(bank.str(), BSPF_toString(i));
    bank.str("");
  }

  int xpos = 10,
      ypos = addBaseInformation(size, "Fred X. Quimby", info.str()) + myLineHeight;

  myBank =
    new PopUpWidget(boss, font, xpos, ypos-2, font.getStringWidth("XX ($800) "),
                    myLineHeight, items, "Set bank: ",
                    font.getStringWidth("Set bank: "), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeSBWidget::loadConfig()
{
  myBank->setSelected(myCart.myCurrentBank);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeSBWidget::handleCommand(CommandSender* sender,
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
