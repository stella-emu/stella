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

#include "Cart2K.hxx"
#include "Cart2KWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge2KWidget::Cartridge2KWidget(
      GuiObject* boss, const GUI::Font& font,
      int x, int y, int w, int h, Cartridge2K& cart)
  : CartDebugWidget(boss, font, x, y, w, h)
{
  addBaseInformation(2048, "Atari", "Standard 2K cartridge, non-bankswitched\n"
                     "Accessible @ $1000 - $1FFF");
}
