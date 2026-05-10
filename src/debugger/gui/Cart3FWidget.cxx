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

#include "Cart3F.hxx"
#include "Cart3FWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3FWidget::Cartridge3FWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, Cartridge3F& cart)
  : CartridgeEnhancedWidget(boss, lfont, nfont, x, y, w, h, cart)
{
  myHotspotDelta = 0;
  initialize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge3FWidget::description()
{
  const ByteSpan image = myCart.getImage();
  const auto* end = image.data() + image.size();
  const uInt16 start = ((static_cast<uInt16>(end[-3]) << 8) |
                            end[-4]) & ~uInt16{0xFFF};

  return std::format(
    "Tigervision 3F cartridge, 2 - 256 2K banks\n"
    "First 2K bank selected by writing to {}\n"
    "Last 2K always points to last 2K of ROM\n"
    "Startup bank = {} or undetermined\n"
    "Bank RORG ${}\n",
    hotspotStr(),
    myCart.startBank(),
    Common::Base::toString(start, Common::Base::Fmt::_16_4));
}
