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
// Copyright (c) 1995-2025 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "CartJANE.hxx"
#include "CartJANEWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeJANEWidget::CartridgeJANEWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeJANE& cart)
  : CartridgeEnhancedWidget(boss, lfont, nfont, x, y, w, h, cart)
{
  initialize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeJANEWidget::description()
{
  ostringstream info;

  info << "Tarzan cartridge, four 4K banks\n"
       << CartridgeEnhancedWidget::description();

  return info.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeJANEWidget::hotspotStr(int bank, int, bool prefix)
{
  ostringstream info;
  const uInt16 hotspot = myCart.hotspot() | ADDR_BASE;

  info << (prefix ? "(hotspot " : "(")
    << "$" << Common::Base::HEX1 << (hotspot + (bank < 2 ? bank : bank + 6))
    << ")";
    // << (prefix ? ")" : ")");  TODO: misc-redundant-expression
    //                                 same logic for true and false

  return info.str();
}
