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
// Copyright (c) 1995-2024 by Bradford W. Mott, Stephen Anthony
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
  ostringstream info;
  size_t size = 0;
  const ByteBuffer& image = myCart.getImage(size);

  info << "Coleco (white label) 8K cartridge, two 4K banks\n"
    << "Banks selected by D3 of value written to " << hotspotStr() << "\n"
    << "Startup bank = undetermined\n";

  uInt16 start = (image[size - 3] << 8) | image[size - 4];
  start -= start % 0x1000;
  info << "Bank RORG $" << Common::Base::HEX4 << start << "\n";

  return info.str();
}
