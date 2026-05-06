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

#include "CartWD.hxx"
#include "CartWDWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeWDWidget::CartridgeWDWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeWD& cart)
  : CartridgeEnhancedWidget(boss, lfont, nfont, x, y, w, h, cart)
{
  initialize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeWDWidget::description()
{
  return std::format(
    "8K + RAM Wickstead Design cartridge, \n"
    "  eight 1K banks, mapped into four segments\n"
    "Hotspots ${:X} - ${:X}, "
    "each hotspot selects a [predefined bank mapping]\n"
    "{}",
    myCart.hotspot(),
    myCart.hotspot() + 7,
    ramDescription());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeWDWidget::hotspotStr(int bank, int, bool prefix)
{
  const CartridgeWD::BankOrg banks = CartridgeWD::ourBankOrg[bank];
  return std::format("({}${:X}) [{}, {}, {}, {}]",
    prefix ? "hotspot " : "",
    myCart.hotspot() + bank,
    static_cast<uInt16>(banks.zero),
    static_cast<uInt16>(banks.one),
    static_cast<uInt16>(banks.two),
    static_cast<uInt16>(banks.three));
}
