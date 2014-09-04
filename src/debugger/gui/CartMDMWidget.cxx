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

#include "CartMDM.hxx"
#include "PopUpWidget.hxx"
#include "Widget.hxx"
#include "CartMDMWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeMDMWidget::CartridgeMDMWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeMDM& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  uInt32 size = myCart.mySize;

  ostringstream info;
  info << "Menu Driven Megacart, containing up to 128 4K banks\n"
       << "Startup bank = " << cart.myStartBank << "\n"
       << "\nBanks are selected by reading from $800 - $FFF, where the lower "
          "byte determines the 4K bank to use.";

  int xpos = 10,
      ypos = addBaseInformation(size, "Edwin Blink", info.str(), 15) + myLineHeight;

  VariantList items;
  for(uInt32 i = 0x800; i < (0x800u + myCart.bankCount()); ++i)
  {
    info.str("");
    info << dec << (i & 0xFF) << " ($" << Common::Base::HEX4 << i << ")";
    items.push_back(info.str());
  }

  myBank =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("xxx ($0FFF) "),
                    myLineHeight, items, "Set bank: ",
                    _font.getStringWidth("Set bank: "), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);

  xpos += myBank->getWidth() + 30;
  myBankDisabled = new CheckboxWidget(boss, _font, xpos, ypos,
                                      "Bankswitching is locked/disabled",
                                      kBankDisabled);
  myBankDisabled->setTarget(this);
  addFocusWidget(myBankDisabled);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMDMWidget::loadConfig()
{
  myBank->setSelectedIndex(myCart.myCurrentBank);
  myBankDisabled->setState(myCart.myBankingDisabled);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMDMWidget::handleCommand(CommandSender* sender,
                                       int cmd, int data, int id)
{
  if(cmd == kBankChanged)
  {
    myCart.unlockBank();
    myCart.bank(myBank->getSelected());
    myCart.lockBank();
    invalidate();
  }
  else if(cmd == kBankDisabled)
  {
    myCart.myBankingDisabled = myBankDisabled->getState();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeMDMWidget::bankState()
{
  ostringstream& buf = buffer();

  buf << "Bank = " << dec << myCart.myCurrentBank
      << ", hotspot = " << "$" << Common::Base::HEX4
      << (myCart.myCurrentBank+0x800);

  return buf.str();
}
