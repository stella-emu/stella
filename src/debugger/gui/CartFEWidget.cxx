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

#include "CartFE.hxx"
#include "CartFEWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFEWidget::CartridgeFEWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeFE& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  string info = 
    "FE cartridge, two 4K banks\n"
    "Doesn't support bankswitching with hotspots, "
    "but instead watches A13 of called addresses:\n"
    "Bank 0 @ $F000 - $FFFF (A13 = 1)\n"
    "Bank 1 @ $D000 - $DFFF (A13 = 0)\n"
    "\n"
    "Changing banks is not currently supported, since it "
    "would immediately switch on the next address change\n";

  addBaseInformation(2 * 4096, "Activision", info);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeFEWidget::bankState()
{
  ostringstream& buf = buffer();

  static const char* range[] = { "$F000", "$D000" };
  buf << "Bank = " << dec << myCart.bank()
      << ", address range = " << range[myCart.bank()];

  return buf.str();
}
