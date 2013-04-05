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
// Copyright (c) 1995-2013 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "Cart4K.hxx"
#include "Cart4KWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge4KWidget::Cartridge4KWidget(
      GuiObject* boss, const GUI::Font& font,
      int x, int y, int w, int h, Cartridge4K& cart)
  : CartDebugWidget(boss, font, x, y, w, h)
{
  // Eventually, we should query this from the debugger/disassembler
  uInt16 start = (cart.myImage[0xFFD] << 8) | cart.myImage[0xFFC];
  start -= start % 0x1000;

  ostringstream info;
  info << "Standard 4K cartridge, non-bankswitched\n"
       << "Accessible @ $" << HEX4 << start << " - " << "$" << (start + 0xFFF);
  addBaseInformation(4096, "Atari", info.str());
}
