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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "CartMNetwork.hxx"
#include "CartE7Widget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeE7Widget::CartridgeE7Widget(
    GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
    int x, int y, int w, int h,
    CartridgeMNetwork& cart)
  : CartridgeMNetworkWidget(boss, lfont, nfont, x, y, w, h, cart)
{
  ostringstream info;
  info << "E7 cartridge, eight 2K banks ROM + 2K RAM,\n"
       << "  mapped into three segments\n"
       << "Lower 2K accessible @ $F000 - $F7FF\n"
       << "  ROM Banks 0 - 6 (hotspots $FFE0 to $FFE6)\n"
       << "  1K RAM Bank 7 (hotspot $FFE7)\n"
       << "    $F400 - $F7FF (R), $F000 - $F3FF (W)\n"
       << "256B RAM accessible @ $F800 - $F9FF\n"
       << "  RAM banks 0 - 3 (hotspots $FFE8 - $FFEB)\n"
       << "    $F900 - $F9FF (R), $F800 - $F8FF (W)\n"
       << "Upper 1.5K ROM accessible @ $FA00 - $FFFF\n"
       << "  Always points to last 1.5K of ROM\n"
       << "Startup segments = 0 / 0 or undetermined\n";

#if 0
  // Eventually, we should query this from the debugger/disassembler
  uInt16 start = (cart.myImage[size-3] << 8) | cart.myImage[size-4];
  start -= start % 0x1000;
  info << "Bank RORG" << " = $" << HEX4 << start << "\n";
#endif

  initialize(boss, cart, info);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* CartridgeE7Widget::getSpotLower(int idx)
{
  static constexpr std::array<const char*, 8> spot_lower = {
    "#0 - ROM ($FFE0)", "#1 - ROM ($FFE1)", "#2 - ROM ($FFE2)", "#3 - ROM ($FFE3)",
    "#4 - ROM ($FFE4)", "#5 - ROM ($FFE5)", "#6 - ROM ($FFE6)", "#7 - RAM ($FFE7)"
  };

  return spot_lower[idx];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* CartridgeE7Widget::getSpotUpper(int idx)
{
  static constexpr std::array<const char*, 4> spot_upper = {
    "#0 - RAM ($FFE8)", "#1 - RAM ($FFE9)", "#2 - RAM ($FFEA)", "#3 - RAM ($FFEB)"
  };

  return spot_upper[idx];
}
