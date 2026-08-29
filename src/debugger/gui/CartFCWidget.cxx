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

#include "CartFC.hxx"
#include "CartFCWidget.hxx"

using Common::Base;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFCWidget::CartridgeFCWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      CartridgeFC& cart)
  : CartridgeEnhancedWidget(boss, lfont, nfont, cart)
{
  initialize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeFCWidget::description()
{
  const uInt16 hotspot = myCart.hotspot() | ADDR_BASE;
  return std::format(
    "FC cartridge, up to eight 4K banks\n"
    "Bank selected by hotspots\n"
    "  ${} (defines low 2 bits)\n"
    "  ${} (defines high bits)\n"
    "  ${} (triggers bank switch)\n"
    "{}",
    Base::hex4(hotspot), Base::hex4(hotspot + 1), Base::hex4(hotspot + 4),
    CartridgeEnhancedWidget::description());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeFCWidget::hotspotStr(int bank, int, bool prefix)
{
  const uInt16 hotspot = myCart.hotspot() | ADDR_BASE;
  return std::format("({}${} = {}, ${} = {})",
    prefix ? "hotspots " : "",
    Base::hex4(hotspot), bank & 0b11,
    Base::hex4(hotspot + 1), bank >> 2);
}
