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

#include "CartE0.hxx"
#include "PopUpWidget.hxx"
#include "CartE0Widget.hxx"

static const char* seg0[] = {
  "0 ($FE0)", "1 ($FE1)", "2 ($FE2)", "3 ($FE3)",
  "4 ($FE4)", "5 ($FE5)", "6 ($FE6)", "7 ($FE7)"
};
static const char* seg1[] = {
  "0 ($FE8)", "1 ($FE9)", "2 ($FEA)", "3 ($FEB)",
  "4 ($FEC)", "5 ($FED)", "6 ($FEE)", "7 ($FEF)"
};
static const char* seg2[] = {
  "0 ($FF0)", "1 ($FF1)", "2 ($FF2)", "3 ($FF3)",
  "4 ($FF4)", "5 ($FF5)", "6 ($FF6)", "7 ($FF7)"
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeE0Widget::CartridgeE0Widget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeE0& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  uInt32 size = 8 * 1024;

  string info =
    "E0 cartridge, eight 1K slices\n"
    "Segment 0 accessible @ $F000 - $F3FF\n"
    "  Hotspots $FE0 to $FE7\n"
    "Segment 1 accessible @ $F400 - $F7FF\n"
    "  Hotspots $FE8 to $FEF\n"
    "Segment 2 accessible @ $F800 - $FBFF\n"
    "  Hotspots $FF0 to $FF7\n"
    "Segment 3 accessible @ $FC00 - $FFFF\n"
    "  Always points to last 1K of ROM\n"
    "Startup slices = 0 / 1 / 2\n";

#if 0
  // Eventually, we should query this from the debugger/disassembler
  uInt16 start = (cart.myImage[size-3] << 8) | cart.myImage[size-4];
  start -= start % 0x1000;
  info << "Bank RORG" << " = $" << HEX4 << start << "\n";
#endif
  int xpos = 10,
      ypos = addBaseInformation(size, "Parker Brothers", info) + myLineHeight;

  VariantList items0, items1, items2;
  for(int i = 0; i < 8; ++i)
  {
    items0.push_back(seg0[i]);
    items1.push_back(seg1[i]);
    items2.push_back(seg2[i]);
  }

  const int lwidth = _font.getStringWidth("Set slice for segment X: ");
  mySlice0 =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("7 ($FF7)"),
                    myLineHeight, items0, "Set slice for segment 0: ",
                    lwidth, kSlice0Changed);
  mySlice0->setTarget(this);
  addFocusWidget(mySlice0);
  ypos += mySlice0->getHeight() + 4;

  mySlice1 =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("7 ($FF7)"),
                    myLineHeight, items1, "Set slice for segment 1: ",
                    lwidth, kSlice1Changed);
  mySlice1->setTarget(this);
  addFocusWidget(mySlice1);
  ypos += mySlice1->getHeight() + 4;

  mySlice2 =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("7 ($FF7)"),
                    myLineHeight, items2, "Set slice for segment 2: ",
                    lwidth, kSlice2Changed);
  mySlice2->setTarget(this);
  addFocusWidget(mySlice2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE0Widget::loadConfig()
{
  mySlice0->setSelectedIndex(myCart.myCurrentSlice[0]);
  mySlice1->setSelectedIndex(myCart.myCurrentSlice[1]);
  mySlice2->setSelectedIndex(myCart.myCurrentSlice[2]);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE0Widget::handleCommand(CommandSender* sender,
                                      int cmd, int data, int id)
{
  myCart.unlockBank();

  switch(cmd)
  {
    case kSlice0Changed:
      myCart.segmentZero(mySlice0->getSelected());
      break;
    case kSlice1Changed:
      myCart.segmentOne(mySlice1->getSelected());
      break;
    case kSlice2Changed:
      myCart.segmentTwo(mySlice2->getSelected());
      break;
  }

  myCart.lockBank();
  invalidate();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeE0Widget::bankState()
{
  ostringstream& buf = buffer();

  buf << "Slices: " << dec
      << seg0[myCart.myCurrentSlice[0]] << " / "
      << seg1[myCart.myCurrentSlice[1]] << " / "
      << seg2[myCart.myCurrentSlice[2]];

  return buf.str();
}
