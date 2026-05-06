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

#include "CartCDFInfoWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCDFInfoWidget::CartridgeCDFInfoWidget(
    GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
    int x, int y, int w, int h, CartridgeCDF& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h)
{
  string fetchers = "LDA #";
  if(cart.myLDXenabled)
    fetchers += ", LDX #";
  if(cart.myLDYenabled)
    fetchers += ", LDY #";

  const string info = std::format(
    "{} cartridge\n"
    "{}K ROM\n"
    "{}K RAM\n"
    "Seven 4K banks are available to 2600\n"
    "Functions accessible @ $FFF0 - $FFF3\n"
    "{}"
    "Startup bank = {}\n"
    "Fast Fetcher(s): {}\n",
    describeCDFVersion(cart.myCDFSubtype),
    cart.romSize() / 1024,
    cart.ramSize() / 1024,
    cart.isCDFJplus() ? "Banks accessible @ $FFF4 to $FFFA\n"
                      : "Banks accessible @ $FFF5 to $FFFB\n",
    cart.startBank(),
    fetchers);

#if 0
  // Eventually, we should query this from the debugger/disassembler
  for(uInt32 i = 0, offset = 0xFFC, spot = 0xFF5; i < 7; ++i, offset += 0x1000)
  {
    uInt16 start = (cart.myImage[offset+1] << 8) | cart.myImage[offset];
    start -= start % 0x1000;
    info << "Bank " << i << " @ $" << HEX4 << (start + 0x80) << " - "
    << "$" << (start + 0xFFF) << " (hotspot = $" << (spot+i) << ")\n";
  }
#endif
  addBaseInformation(cart.romSize(), "AtariAge", info);
}
