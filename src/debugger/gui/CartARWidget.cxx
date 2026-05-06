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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "CartAR.hxx"
#include "OSystem.hxx"
#include "Debugger.hxx"
#include "CartDebug.hxx"
#include "PopUpWidget.hxx"
#include "CartARWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeARWidget::CartridgeARWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeAR& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart{cart}
{
  const size_t size = myCart.mySize;

  constexpr string_view info =
    "Supercharger cartridge, four 2K slices (3 RAM, 1 ROM)\n"
    "\nTHIS SCHEME IS NOT FULLY IMPLEMENTED OR TESTED\n";

  constexpr int xpos = 2;
  const int ypos = addBaseInformation(size, "Starpath", info) + myLineHeight;

  VariantList items;
  for(int i = 0; i < 32; ++i)
    VarList::push_back(items, std::format("{:3}", i));
  myBank = new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth(" XX"),
                           myLineHeight, items, "Set bank     ",
                           0, kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARWidget::loadConfig()
{
  CartDebug& cart = instance().debugger().cartDebug();
  const auto& state = static_cast<const CartState&>(cart.getState());
  const auto& oldstate = static_cast<const CartState&>(cart.getOldState());

  myBank->setSelectedIndex(myCart.getBank(), state.bank != oldstate.bank);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARWidget::handleCommand(CommandSender* sender,
                                      int cmd, int data, int id)
{
  if(cmd == kBankChanged)
  {
    myCart.unlockHotspots();
    myCart.bank(myBank->getSelected());
    myCart.lockHotspots();
    invalidate();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeARWidget::bankState()
{
  return std::format("Bank = {}", myCart.myCurrentBank);
}
