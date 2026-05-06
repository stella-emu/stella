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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFCWidget::CartridgeFCWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeFC& cart)
  : CartridgeEnhancedWidget(boss, lfont, nfont, x, y, w, h, cart)
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
    "  ${:04X} (defines low 2 bits)\n"
    "  ${:04X} (defines high bits)\n"
    "  ${:04X} (triggers bank switch)\n"
    "{}",
    hotspot, hotspot + 1, hotspot + 4,
    CartridgeEnhancedWidget::description());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeFCWidget::hotspotStr(int bank, int, bool prefix)
{
  const uInt16 hotspot = myCart.hotspot() | ADDR_BASE;
  return std::format("({}${:04X} = {}, ${:04X} = {})",
    prefix ? "hotspots " : "",
    hotspot, bank & 0b11,
    hotspot + 1, bank >> 2);
}
