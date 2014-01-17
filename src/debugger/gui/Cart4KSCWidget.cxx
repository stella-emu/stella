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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "Cart4KSC.hxx"
#include "Cart4KSCWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge4KSCWidget::Cartridge4KSCWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, Cartridge4KSC& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h)
{
  // Eventually, we should query this from the debugger/disassembler
  uInt16 start = (cart.myImage[0xFFD] << 8) | cart.myImage[0xFFC];
  start -= start % 0x1000;

  ostringstream info;
  info << "4KSC cartridge, non-bankswitched\n"
       << "128 bytes RAM @ $F000 - $F0FF\n"
       << "  $F080 - $F0FF (R), $F000 - $F07F (W)\n"
       << "Accessible @ $" << Common::Base::HEX4 << start << " - "
       << "$" << (start + 0xFFF);

  addBaseInformation(4096, "homebrew intermediate format", info.str());
}
