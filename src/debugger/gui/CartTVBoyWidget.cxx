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

#include "CartTVBoy.hxx"
#include "PopUpWidget.hxx"
#include "CartTVBoyWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeTVBoyWidget::CartridgeTVBoyWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeTVBoy& cart)
  : CartridgeEnhancedWidget(boss, lfont, nfont, x, y, w, h, cart),
    myCartTVBoy{cart}
{
  initialize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeTVBoyWidget::description()
{
  return std::format(
    "TV Boy, {} 4K banks\n"
    "Hotspots are from ${:X} to ${:X}\n"
    "{}",
    myCart.romBankCount(),
    0xf800,
    0xf800 + myCart.romBankCount() - 1,
    CartridgeEnhancedWidget::description());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeTVBoyWidget::createExtras()
{
  if(myCart.romBankCount() > 1)
  {
    myBankLocked = new CheckboxWidget(_boss, _font, 0, 0,
                                      "Bankswitching is locked",
                                      kBankLocked);
    myBankLocked->setTarget(this);
    addFocusWidget(myBankLocked);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeTVBoyWidget::reflowExtra()
{
  // The checkbox sits to the right of the bank selector
  if(myBankLocked != nullptr)
    myBankLocked->setPos(myBankWidgets[0]->getRight() + _fontWidth * 4,
                         myBankWidgets[0]->getTop() + 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeTVBoyWidget::loadConfig()
{
  if(myBankWidgets != nullptr)
  {
    myBankWidgets[0]->setEnabled(!myCartTVBoy.myBankingDisabled);
    myBankLocked->setState(myCartTVBoy.myBankingDisabled);
  }
  CartridgeEnhancedWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeTVBoyWidget::handleCommand(CommandSender* sender,
                                         int cmd, int data, int id)
{
  if(cmd == kBankLocked)
  {
    myCartTVBoy.myBankingDisabled = myBankLocked->getState();
    myBankWidgets[0]->setEnabled(!myCartTVBoy.myBankingDisabled);
  }
  else
    CartridgeEnhancedWidget::handleCommand(sender, cmd, data, id);
}
