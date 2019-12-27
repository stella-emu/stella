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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "CartUA.hxx"
#include "PopUpWidget.hxx"
#include "CartUAWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeUAWidget::CartridgeUAWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeUA& cart, bool swapHotspots)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart),
    mySwappedHotspots(swapHotspots)
{
  uInt16 size = 2 * 4096;

  ostringstream info;
  info << "8K UA cartridge" << (mySwappedHotspots ? " (swapped banks)" : "") << ", two 4K banks\n"
       << "Startup bank = " << cart.startBank() << " or undetermined\n";

  // Eventually, we should query this from the debugger/disassembler
  for(uInt32 i = 0, offset = 0xFFC, spot = mySwappedHotspots ? 0x240 : 0x220; i < 2;
      ++i, offset += 0x1000, spot += mySwappedHotspots ? -0x20 : 0x20)
  {
    uInt16 start = (cart.myImage[offset+1] << 8) | cart.myImage[offset];
    start -= start % 0x1000;
    info << "Bank " << i << " @ $" << Common::Base::HEX4 << start << " - "
         << "$" << (start + 0xFFF) << " (hotspots = $" << spot << ", $" << (spot | 0x80) << ")\n";
  }

  int xpos = 2,
      ypos = addBaseInformation(size, "UA Limited", info.str()) + myLineHeight;

  VariantList items;
  if (swapHotspots)
  {
    VarList::push_back(items, "0 ($240, $2C0)");
    VarList::push_back(items, "1 ($220, $2A0)");
  }
  else
  {
    VarList::push_back(items, "0 ($220, $2A0)");
    VarList::push_back(items, "1 ($240, $2C0)");
  }
  myBank =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("0 ($FFx, $FFx)"),
                    myLineHeight, items, "Set bank     ",
                    0, kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeUAWidget::loadConfig()
{
  Debugger& dbg = instance().debugger();
  CartDebug& cart = dbg.cartDebug();
  const CartState& state = static_cast<const CartState&>(cart.getState());
  const CartState& oldstate = static_cast<const CartState&>(cart.getOldState());

  myBank->setSelectedIndex(myCart.getBank(), state.bank != oldstate.bank);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeUAWidget::handleCommand(CommandSender* sender,
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
string CartridgeUAWidget::bankState()
{
  ostringstream& buf = buffer();

  static constexpr std::array<const char*, 2> spot = { "$220, $2A0", "$240, $2C0" };
  buf << "Bank = " << std::dec << myCart.getBank()
      << ", hotspots = " << spot[myCart.getBank() ^ (mySwappedHotspots ? 1U : 0U)];

  return buf.str();
}
