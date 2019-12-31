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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "CartFC.hxx"
#include "PopUpWidget.hxx"
#include "CartFCWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFCWidget::CartridgeFCWidget(
  GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
  int x, int y, int w, int h, CartridgeFC& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
  myCart(cart)
{
  uInt16 size = cart.bankCount() * 4096;

  ostringstream info;
  info << "FC cartridge, up to eight 4K banks\n"
    << "Startup bank = " << cart.startBank() << " or undetermined\n";

  // Eventually, we should query this from the debugger/disassembler
  info << "Bank selected by hotspots\n"
    << " $FFF8 (defines low 2 bits)\n"
    << " $FFF9 (defines high bits)\n"
    << " $FFFC (triggers bank switch)";

  int xpos = 2,
    ypos = addBaseInformation(size, "Amiga Corp.", info.str()) + myLineHeight;

  VariantList items;
  for (uInt16 i = 0; i < cart.bankCount(); ++i)
    VarList::push_back(items, Variant(i).toString() +
                       " ($FFF8 = " + Variant(i & 0b11).toString() +
                       "/$FFF9 = " + Variant(i >> 2).toString() +")");

  myBank = new PopUpWidget(boss, _font, xpos, ypos - 2,
                    _font.getStringWidth("7 ($FFF8 = 3/$FFF9 = 1)"),
                    myLineHeight, items, "Set bank     ",
                    0, kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFCWidget::loadConfig()
{
  Debugger& dbg = instance().debugger();
  CartDebug& cart = dbg.cartDebug();
  const CartState& state = static_cast<const CartState&>(cart.getState());
  const CartState& oldstate = static_cast<const CartState&>(cart.getOldState());

  myBank->setSelectedIndex(myCart.getBank(), state.bank != oldstate.bank);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFCWidget::handleCommand(CommandSender* sender,
                                      int cmd, int data, int id)
{
  if (cmd == kBankChanged)
  {
    myCart.unlockBank();
    myCart.bank(myBank->getSelected());
    myCart.lockBank();
    invalidate();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeFCWidget::bankState()
{
  ostringstream& buf = buffer();
  uInt16 bank = myCart.getBank();

  buf << "Bank = #" << std::dec << bank
    << ", hotspots $FFF8 = " << (bank & 0b11)
    << "/$FF99 = " << (bank >> 2);

  return buf.str();
}
