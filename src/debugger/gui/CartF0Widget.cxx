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

#include "CartF0.hxx"
#include "CartF0Widget.hxx"

using Common::Base;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeF0Widget::CartridgeF0Widget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      CartridgeF0& cart)
  : CartridgeEnhancedWidget(boss, lfont, nfont, cart)
{
  myHotspotDelta = 0;
  initialize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeF0Widget::description()
{
  return std::format(
    "Megaboy F0 cartridge, 16 4K banks\n"
    "Startup bank = #{} or undetermined\n"
    "Bankswitch triggered by accessing ${}\n",
    myCart.startBank(), Base::hex4(0xFFF0));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeF0Widget::bankState()
{
  return std::format("Bank #{} (hotspot ${})",
    myCart.getBank(), Base::hex4(0xFFF0));
}
