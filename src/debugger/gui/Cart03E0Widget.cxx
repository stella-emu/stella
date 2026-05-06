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

#include "Cart03E0.hxx"
#include "Cart03E0Widget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge03E0Widget::Cartridge03E0Widget(
  GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
  int x, int y, int w, int h, Cartridge03E0& cart)
  : CartridgeEnhancedWidget(boss, lfont, nfont, x, y, w, h, cart)
{
  initialize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge03E0Widget::description()
{
  return std::format("03E0 cartridge,\n  eight 1K banks mapped into four segments\n{}",
    CartridgeEnhancedWidget::description());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge03E0Widget::romDescription()
{
  string info;
  for(int seg = 0; seg < 4; ++seg)
  {
    const uInt16 segmentOffset = seg << 10; // myCart.myBankShift;
    info += std::format("Segment #{} accessible @ ${:04X} - ${:04X},\n",
      seg,
      ADDR_BASE | segmentOffset,
      ADDR_BASE | (segmentOffset + /*myCart.myBankSize - 1*/ 0x3FF));
    if(seg < 3)
      info += std::format("  Hotspots {} - {}\n",
        hotspotStr(0, seg, true), hotspotStr(7, seg, true));
    else
      info += "  Always points to last 1K bank of ROM\n";
  }
  info += "Startup banks = 4 / 5 / 6 or undetermined";
  return info;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge03E0Widget::hotspotStr(int bank, int segment, bool noBrackets)
{
  static constexpr uInt16 hotspots[3] = {0x03E0, 0x03D0, 0x03B0};
  return std::format("{}${:X}{}",
    noBrackets ? "" : "(",
    hotspots[segment] + bank,
    noBrackets ? "" : ")");
}
