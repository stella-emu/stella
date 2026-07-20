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
#include "Layout.hxx"
#include "CartTVBoyWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeTVBoyWidget::CartridgeTVBoyWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      CartridgeTVBoy& cart)
  : CartridgeEnhancedWidget(boss, lfont, nfont, cart),
    myCartTVBoy{cart}
{
  // A single-bank cart has no bank selector, so nothing to lock either
  if(myCart.romBankCount() > 1)
  {
    myBankLocked = new CheckboxWidget(_boss, _font, 0, 0,
                                      "Bankswitching is locked",
                                      kBankLocked);
    myBankLocked->setTarget(this);
    addFocusWidget(myBankLocked);
  }
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
void CartridgeTVBoyWidget::layoutBankSelect(GUI::BoxLayout& col) const
{
  using GUI::anchoredItem;
  using GUI::indentedItem;

  // A single-bank cart has neither a selector nor a checkbox to lock it with
  if(myBankWidgets.empty())
    return;

  col.addAuto(anchoredItem(myBankWidgets[0]));

  // The lock checkbox goes UNDER the selector rather than beside it: its label is
  // long and the selector's width follows the bank labels, so a single row would
  // crowd the right border on a cart with many banks.  It is an option BELONGING
  // to the selector, so it is indented under it
  col.addAuto(indentedItem(myBankLocked, _fontWidth * 2));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeTVBoyWidget::loadConfig()
{
  if(!myBankWidgets.empty())
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
