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

#include "CartMVC.hxx"
#include "CartMVCWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeMVCWidget::CartridgeMVCWidget(
    GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
    CartridgeMVC& cart)
  : CartDebugWidget(boss, lfont, nfont)
{
  const string info = std::format(
      "MovieCart\n"
      "Audio and video are streamed from the movie file, so there is no ROM "
      "image: the 1K window the 2600 sees is rewritten on the fly, in 512 byte "
      "chunks, as the 6502 enters successive {} byte field regions.\n"
      "Movie length is limited only by the size of the file.\n"
      "Audio is single-channel 4-bit PCM; video uses TIA colour dithering.\n",
      CartridgeMVC::MVC_FIELD_SIZE);

  // This tab is nothing but the ROM info block; reflow() lays it out
  createBaseInformation(cart.getImage().size(), "lodefmode", info);
  reflow();
}
