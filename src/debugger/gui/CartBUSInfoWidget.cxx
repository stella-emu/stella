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

#include "CartBUSInfoWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeBUSInfoWidget::CartridgeBUSInfoWidget(
    GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
    CartridgeBUS& cart)
  : CartDebugWidget(boss, lfont, nfont)
{
  constexpr uInt16 size = 8 * 4096;

  const string info = (cart.myBUSSubtype == CartridgeBUS::BUSSubtype::BUS0)
    ? std::format(
        "BUS Stuffing cartridge (EXPERIMENTAL)\n"
        "32K ROM, six 4K banks are accessible to 2600\n"
        "8K BUS RAM\n"
        "BUS registers accessible @ $F000 - $F03F\n"
        "Banks accessible at hotspots $FFF6 to $FFFB\n"
        "Startup bank = {}\n",
        cart.startBank())
    : std::format(
        "BUS Stuffing cartridge (EXPERIMENTAL)\n"
        "32K ROM, seven 4K banks are accessible to 2600\n"
        "8K BUS RAM\n"
        "{}"
        "Banks accessible at hotspots $FFF5 to $FFFB\n"
        "Startup bank = {}\n",
        cart.myBUSSubtype == CartridgeBUS::BUSSubtype::BUS3
          ? "BUS registers accessible @ $FFEE - $FFF3\n"  // BUS3
          : "BUS registers accessible @ $F000 - $F01A\n", // BUS1, BUS2
        cart.startBank());

  // This tab is nothing but the ROM info block; reflow() lays it out
  createBaseInformation(size, "AtariAge", info);
  reflow();
}
