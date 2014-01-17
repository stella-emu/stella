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

#include "CartAR.hxx"
#include "PopUpWidget.hxx"
#include "CartARWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeARWidget::CartridgeARWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeAR& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  uInt16 size = myCart.mySize;

  string info =
    "Supercharger cartridge, four 2K slices (3 RAM, 1 ROM)\n"
    "\nTHIS SCHEME IS NOT FULLY IMPLEMENTED OR TESTED\n";

  int xpos = 10,
      ypos = addBaseInformation(size, "Starpath", info) + myLineHeight;

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
  items.push_back(" 16");
  items.push_back(" 17");
  items.push_back(" 18");
  items.push_back(" 19");
  items.push_back(" 20");
  items.push_back(" 21");
  items.push_back(" 22");
  items.push_back(" 23");
  items.push_back(" 24");
  items.push_back(" 25");
  items.push_back(" 26");
  items.push_back(" 27");
  items.push_back(" 28");
  items.push_back(" 29");
  items.push_back(" 30");
  items.push_back(" 31");
  myBank =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth(" XX "),
                    myLineHeight, items, "Set bank: ",
                    _font.getStringWidth("Set bank: "), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARWidget::loadConfig()
{
  myBank->setSelectedIndex(myCart.myCurrentBank);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARWidget::handleCommand(CommandSender* sender,
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
string CartridgeARWidget::bankState()
{
  ostringstream& buf = buffer();

  buf << "Bank = " << dec << myCart.myCurrentBank;

  return buf.str();
}
