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

#include "CartE0.hxx"
#include "CartE0Widget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeE0Widget::CartridgeE0Widget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeE0& cart)
  : CartridgeEnhancedWidget(boss, lfont, nfont, x, y, w, h, cart)
{
  initialize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeE0Widget::description()
{
  return std::format("E0 cartridge,\n  eight 1K banks mapped into four segments\n{}",
    CartridgeEnhancedWidget::description());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeE0Widget::romDescription()
{
  string info;
  info.reserve(256);
  for(int seg = 0; seg < 4; ++seg)
  {
    const uInt16 segmentOffset = seg << 10; // myCart.myBankShift;
    info += std::format("Segment #{} accessible @ ${:X} - ${:X},\n",
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
string CartridgeE0Widget::hotspotStr(int bank, int segment, bool noBrackets)
{
  const uInt16 hotspot = myCart.hotspot();
  return std::format("{0}${1:X}{2}",
    noBrackets ? "" : "(",
    hotspot + bank + segment * 8,
    noBrackets ? "" : ")");
}
