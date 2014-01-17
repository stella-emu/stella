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

#include "CartE7.hxx"
#include "PopUpWidget.hxx"
#include "CartE7Widget.hxx"

static const char* spot_lower[] = {
  "0 - ROM ($FE0)", "1 - ROM ($FE1)", "2 - ROM ($FE2)", "3 - ROM ($FE3)",
  "4 - ROM ($FE4)", "5 - ROM ($FE5)", "6 - ROM ($FE6)", "7 - RAM ($FE7)"
};
static const char* spot_upper[] = {
  "0 - RAM ($FE8)", "1 - RAM ($FE9)", "2 - RAM ($FEA)", "3 - RAM ($FEB)"
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeE7Widget::CartridgeE7Widget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeE7& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  uInt32 size = 8 * 2048;

  ostringstream info;
  info << "E7 cartridge, 8 2K slices ROM + 2 1K RAM\n"
       << "Lower 2K accessible @ $F000 - $F7FF\n"
       << "  Slice 0 - 6 of ROM (hotspots $FE0 to $FE6)\n"
       << "  Slice 0 (1K) of RAM (hotspot $FE7)\n"
       << "    $F400 - $F7FF (R), $F000 - $F3FF (W)\n"
       << "256B RAM accessible @ $F800 - $F9FF\n"
       << "  Hotspots $FE8 - $FEB (256B of RAM slice 1)\n"
       << "    $F400 - $F7FF (R), $F000 - $F3FF (W)\n"
       << "Upper 1.5K ROM accessible @ $FA00 - $FFFF\n"
       << "  Always points to last 1.5K of ROM\n"
       << "Startup slices = " << cart.myStartBank << " / 0\n";

#if 0
  // Eventually, we should query this from the debugger/disassembler
  uInt16 start = (cart.myImage[size-3] << 8) | cart.myImage[size-4];
  start -= start % 0x1000;
  info << "Bank RORG" << " = $" << HEX4 << start << "\n";
#endif
  int xpos = 10,
      ypos = addBaseInformation(size, "M-Network", info.str(), 15) +
              myLineHeight;

  VariantList items0, items1;
  for(int i = 0; i < 8; ++i)
    items0.push_back(spot_lower[i]);
  for(int i = 0; i < 4; ++i)
    items1.push_back(spot_upper[i]);

  const int lwidth = _font.getStringWidth("Set slice for upper 256B: "),
            fwidth = _font.getStringWidth("3 - RAM ($FEB)");
  myLower2K =
    new PopUpWidget(boss, _font, xpos, ypos-2, fwidth, myLineHeight, items0,
                    "Set slice for lower 2K: ", lwidth, kLowerChanged);
  myLower2K->setTarget(this);
  addFocusWidget(myLower2K);
  ypos += myLower2K->getHeight() + 4;

  myUpper256B =
    new PopUpWidget(boss, _font, xpos, ypos-2, fwidth, myLineHeight, items1,
                    "Set slice for upper 256B: ", lwidth, kUpperChanged);
  myUpper256B->setTarget(this);
  addFocusWidget(myUpper256B);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE7Widget::loadConfig()
{
  myLower2K->setSelectedIndex(myCart.myCurrentSlice[0]);
  myUpper256B->setSelectedIndex(myCart.myCurrentRAM);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE7Widget::handleCommand(CommandSender* sender,
                                      int cmd, int data, int id)
{
  myCart.unlockBank();

  switch(cmd)
  {
    case kLowerChanged:
      myCart.bank(myLower2K->getSelected());
      break;
    case kUpperChanged:
      myCart.bankRAM(myUpper256B->getSelected());
      break;
  }

  myCart.lockBank();
  invalidate();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeE7Widget::bankState()
{
  ostringstream& buf = buffer();

  buf << "Slices: " << dec
      << spot_lower[myCart.myCurrentSlice[0]] << " / "
      << spot_upper[myCart.myCurrentRAM];

  return buf.str();
}
