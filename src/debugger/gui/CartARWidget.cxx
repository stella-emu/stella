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
  VList::push_back(items, "  0");
  VList::push_back(items, "  1");
  VList::push_back(items, "  2");
  VList::push_back(items, "  3");
  VList::push_back(items, "  4");
  VList::push_back(items, "  5");
  VList::push_back(items, "  6");
  VList::push_back(items, "  7");
  VList::push_back(items, "  8");
  VList::push_back(items, "  9");
  VList::push_back(items, " 10");
  VList::push_back(items, " 11");
  VList::push_back(items, " 12");
  VList::push_back(items, " 13");
  VList::push_back(items, " 14");
  VList::push_back(items, " 15");
  VList::push_back(items, " 16");
  VList::push_back(items, " 17");
  VList::push_back(items, " 18");
  VList::push_back(items, " 19");
  VList::push_back(items, " 20");
  VList::push_back(items, " 21");
  VList::push_back(items, " 22");
  VList::push_back(items, " 23");
  VList::push_back(items, " 24");
  VList::push_back(items, " 25");
  VList::push_back(items, " 26");
  VList::push_back(items, " 27");
  VList::push_back(items, " 28");
  VList::push_back(items, " 29");
  VList::push_back(items, " 30");
  VList::push_back(items, " 31");
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
