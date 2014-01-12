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

#include "Cart2K.hxx"
#include "Cart2KWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge2KWidget::Cartridge2KWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, Cartridge2K& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h)
{
  // Eventually, we should query this from the debugger/disassembler
  uInt16 size = cart.mySize;
  uInt16 start = (cart.myImage[size-3] << 8) | cart.myImage[size-4];
  start -= start % size;

  ostringstream info;
  info << "Standard 2K cartridge, non-bankswitched\n"
       << "Accessible @ $" << Common::Base::HEX4 << start << " - " << "$" << (start + size - 1);
  addBaseInformation(size, "Atari", info.str());
}
