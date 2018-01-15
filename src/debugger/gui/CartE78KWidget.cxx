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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "CartMNetwork.hxx"
#include "PopUpWidget.hxx"
#include "CartE78KWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeE78KWidget::CartridgeE78KWidget(
    GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
    int x, int y, int w, int h,
    CartridgeMNetwork& cart)
  : CartridgeMNetworkWidget(boss, lfont, nfont, x, y, w, h, cart)
{
  ostringstream info;
  info << "E78K cartridge, 4 2K slices ROM + 2 1K RAM\n"
       << "Lower 2K accessible @ $F000 - $F7FF\n"
       << "  Slice 0 - 2 of ROM (hotspots $FE4 to $FE6)\n"
       << "  Slice 7 (1K) of RAM (hotspot $FE7)\n"
       << "    $F400 - $F7FF (R), $F000 - $F3FF (W)\n"
       << "256B RAM accessible @ $F800 - $F9FF\n"
       << "  Hotspots $FE8 - $FEB (256B of RAM slice 1)\n"
       << "    $F900 - $F9FF (R), $F800 - $F8FF (W)\n"
       << "Upper 1.5K ROM accessible @ $FA00 - $FFFF\n"
       << "  Always points to last 1.5K of ROM\n"
       << "Startup slices = 0 / 0 or undetermined\n";

#if 0
  // Eventually, we should query this from the debugger/disassembler
  uInt16 start = (cart.myImage[size - 3] << 8) | cart.myImage[size - 4];
  start -= start % 0x1000;
  info << "Bank RORG" << " = $" << HEX4 << start << "\n";
#endif

  initialize(boss, cart, info);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* CartridgeE78KWidget::getSpotLower(int idx)
{
  static const char* const spot_lower[] = {
    "0 - ROM ($FFE4)", "1 - ROM ($FFE5)", "2 - ROM ($FFE6)", "3 - RAM ($FFE7)"
  };

  return spot_lower[idx];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* CartridgeE78KWidget::getSpotUpper(int idx)
{
  static const char* const spot_upper[] = {
    "0 - RAM ($FFE8)", "1 - RAM ($FFE9)", "2 - RAM ($FFEA)", "3 - RAM ($FFEB)"
  };

  return spot_upper[idx];
}
