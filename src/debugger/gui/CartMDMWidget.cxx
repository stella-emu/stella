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

#include "CartMDM.hxx"
#include "PopUpWidget.hxx"
#include "Layout.hxx"
#include "CartMDMWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeMDMWidget::CartridgeMDMWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeMDM& cart)
  : CartridgeEnhancedWidget(boss, lfont, nfont, x, y, w, h, cart),
    myCartMDM{cart}
{
  initialize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeMDMWidget::description()
{
  return std::format(
    "Menu Driven Megacart, {} 4K banks\n"
    "Banks are selected by reading from $800 - ${:X}, "
    "where the lower byte determines the 4K bank to use.\n"
    "{}",
    myCart.romBankCount(), 0xBFF,
    CartridgeEnhancedWidget::description());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMDMWidget::createExtras()
{
  if(myCart.romBankCount() > 1)
  {
    myBankDisabled = new CheckboxWidget(_boss, _font, 0, 0,
                                        "Bankswitching is locked/disabled",
                                        kBankDisabled);
    myBankDisabled->setTarget(this);
    addFocusWidget(myBankDisabled);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMDMWidget::layoutBankSelect(GUI::BoxLayout& col)
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
  col.addAuto(indentedItem(myBankDisabled, _fontWidth * 2));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMDMWidget::loadConfig()
{
  if(!myBankWidgets.empty())
  {
    myBankWidgets[0]->setEnabled(!myCartMDM.myBankingDisabled);
    myBankDisabled->setState(myCartMDM.myBankingDisabled);
  }
  CartridgeEnhancedWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMDMWidget::handleCommand(CommandSender* sender,
                                       int cmd, int data, int id)
{
  if(cmd == kBankDisabled)
  {
    myCartMDM.myBankingDisabled = myBankDisabled->getState();
    myBankWidgets[0]->setEnabled(!myCartMDM.myBankingDisabled);
  }
  else
    CartridgeEnhancedWidget::handleCommand(sender, cmd, data, id);
}
