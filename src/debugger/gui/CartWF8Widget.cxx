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

#include "CartWF8.hxx"
#include "CartWF8Widget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeWF8Widget::CartridgeWF8Widget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeWF8& cart)
  : CartridgeEnhancedWidget(boss, lfont, nfont, x, y, w, h, cart)
{
  myHotspotDelta = 0;
  initialize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeWF8Widget::description()
{
  const auto image = myCart.getImage();
  const auto* end = image.data() + image.size();
  const uInt16 start = ((static_cast<uInt16>(end[-3]) << 8) | end[-4]) & ~uInt16{0xFFF};

  return std::format(
    "Coleco (some white carts) 8K cartridge, two 4K banks\n"
    "Banks selected by D3 of value written to {}\n"
    "Startup bank = undetermined\n"
    "Bank RORG ${}\n",
    hotspotStr(),
    Common::Base::toString(start, Common::Base::Fmt::_16_4));
}
