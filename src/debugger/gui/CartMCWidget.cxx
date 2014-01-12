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

#include "CartMC.hxx"
#include "PopUpWidget.hxx"
#include "CartMCWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeMCWidget::CartridgeMCWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeMC& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
#define ROM_BLOCKS

  uInt32 size = 128 * 1024;

  string info =
    "MC cartridge, 128 1K slices ROM + 32 1K RAM\n"
    "Write $80 - $FF into a hotspot for ROM (128)\n"
    "Write $00 - $3F into a hotspot for RAM (32)\n"
    "Segment 0 @ $F000 - $F3FF (hotspot = $3C)\n"
    "Segment 1 @ $F400 - $F7FF (hotspot = $3D)\n"
    "Segment 2 @ $F800 - $FBFF (hotspot = $3E)\n"
    "Segment 3 @ $FC00 - $FFFF (hotspot = $3F)\n"
    "\nTHIS SCHEME IS NOT FULLY IMPLEMENTED OR TESTED\n";

  int xpos = 10,
      ypos = addBaseInformation(size, "Chris Wilkson's Megacart", info) +
             myLineHeight;

  VariantList items;
  // Add 128 1K 'ROM' blocks
  for(uInt32 i = 0x80; i <= 0xFF; ++i)
  {
    const string& b = Variant(i).toString();
    items.push_back(b + " (ROM)", b);
  }
  // Add 64 512B 'RAM' blocks
  for(uInt32 i = 0x00; i <= 0x3F; ++i)
  {
    const string& b = Variant(i).toString();
    items.push_back(b + " (RAM)", b);
  }

  const int lwidth = _font.getStringWidth("Set slice for segment X ($3X): "),
            fwidth = _font.getStringWidth("255 (ROM)");

  mySlice0 =
    new PopUpWidget(boss, _font, xpos, ypos-2, fwidth,
                    myLineHeight, items, "Set slice for segment 0 ($3C): ",
                    lwidth, kSlice0Changed);
  mySlice0->setTarget(this);
  addFocusWidget(mySlice0);
  ypos += mySlice0->getHeight() + 4;

  mySlice1 =
    new PopUpWidget(boss, _font, xpos, ypos-2, fwidth,
                    myLineHeight, items, "Set slice for segment 1 ($3D): ",
                    lwidth, kSlice1Changed);
  mySlice1->setTarget(this);
  addFocusWidget(mySlice1);
  ypos += mySlice1->getHeight() + 4;

  mySlice2 =
    new PopUpWidget(boss, _font, xpos, ypos-2, fwidth,
                    myLineHeight, items, "Set slice for segment 2 ($3E): ",
                    lwidth, kSlice2Changed);
  mySlice2->setTarget(this);
  addFocusWidget(mySlice2);
  ypos += mySlice2->getHeight() + 4;

  mySlice3 =
    new PopUpWidget(boss, _font, xpos, ypos-2, fwidth,
                    myLineHeight, items, "Set slice for segment 3 ($3F): ",
                    lwidth, kSlice3Changed);
  mySlice3->setTarget(this);
  addFocusWidget(mySlice3);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMCWidget::loadConfig()
{
  mySlice0->setSelectedIndex(myCart.myCurrentBlock[0]);
  mySlice1->setSelectedIndex(myCart.myCurrentBlock[1]);
  mySlice2->setSelectedIndex(myCart.myCurrentBlock[2]);
  mySlice3->setSelectedIndex(myCart.myCurrentBlock[3]);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMCWidget::handleCommand(CommandSender* sender,
                                      int cmd, int data, int id)
{
  myCart.unlockBank();

  switch(cmd)
  {
    case kSlice0Changed:
      myCart.myCurrentBlock[0] = mySlice0->getSelectedTag().toInt();
      break;
    case kSlice1Changed:
      myCart.myCurrentBlock[1] = mySlice1->getSelectedTag().toInt();
      break;
    case kSlice2Changed:
      myCart.myCurrentBlock[2] = mySlice2->getSelectedTag().toInt();
      break;
    case kSlice3Changed:
      myCart.myCurrentBlock[3] = mySlice3->getSelectedTag().toInt();
      break;
  }

  myCart.lockBank();
  invalidate();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeMCWidget::bankState()
{
  ostringstream& buf = buffer();

  buf << "Slices: " << dec
      << myCart.myCurrentBlock[0] << " / "
      << myCart.myCurrentBlock[1] << " / "
      << myCart.myCurrentBlock[2] << " / "
      << myCart.myCurrentBlock[3];

  return buf.str();
}
