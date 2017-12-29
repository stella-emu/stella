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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "CartFE.hxx"
#include "PopUpWidget.hxx"
#include "CartFEWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFEWidget::CartridgeFEWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeFE& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  string info =
    "FE cartridge, two 4K banks\n"
    "Monitors access to hotspot $01FE, and uses "
    "upper 3 bits of databus for bank number:\n"
    "Bank 0 @ $F000 - $FFFF (DATA = 111, D5 = 1)\n"
    "Bank 1 @ $D000 - $DFFF (DATA = 110, D5 = 0)\n";

  int xpos = 10,
      ypos = addBaseInformation(2 * 4096, "Activision", info) + myLineHeight;

  VariantList items;
  VarList::push_back(items, "0 ($01FE, D5=1)");
  VarList::push_back(items, "1 ($01FE, D5=0)");
  myBank =
    new PopUpWidget(boss, _font, xpos, ypos-2,
                    _font.getStringWidth("0 ($01FE, D5=1)"),
                    myLineHeight, items, "Set bank ",
                    _font.getStringWidth("Set bank "), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFEWidget::loadConfig()
{
  Debugger& dbg = instance().debugger();
  CartDebug& cart = dbg.cartDebug();
  const CartState& state = static_cast<const CartState&>(cart.getState());
  const CartState& oldstate = static_cast<const CartState&>(cart.getOldState());

  myBank->setSelectedIndex(myCart.getBank(), state.bank != oldstate.bank);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFEWidget::handleCommand(CommandSender* sender,
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
string CartridgeFEWidget::bankState()
{
  ostringstream& buf = buffer();

  static const char* const range[] = { "$F000", "$D000" };
  buf << "Bank = " << std::dec << myCart.getBank()
      << ", address range = " << range[myCart.getBank()];

  return buf.str();
}
