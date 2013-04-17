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

#include "CartE7.hxx"
#include "PopUpWidget.hxx"
#include "CartE7Widget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeE7Widget::CartridgeE7Widget(
      GuiObject* boss, const GUI::Font& font,
      int x, int y, int w, int h, CartridgeE7& cart)
  : CartDebugWidget(boss, font, x, y, w, h),
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
       << "  Always points to last 2K (1.5) of ROM\n"
       << "Startup slices = " << cart.myStartBank << " / 0\n";

#if 0
  // Eventually, we should query this from the debugger/disassembler
  uInt16 start = (cart.myImage[size-3] << 8) | cart.myImage[size-4];
  start -= start % 0x1000;
  info << "Bank RORG" << " = $" << HEX4 << start << "\n";
#endif
  int xpos = 10,
      ypos = addBaseInformation(size, "M-Network", info.str(), 15) + myLineHeight;

  StringMap items0, items1;
  items0.push_back("0 - ROM ($FE0)", "0");
  items0.push_back("1 - ROM ($FE1)", "1");
  items0.push_back("2 - ROM ($FE2)", "2");
  items0.push_back("3 - ROM ($FE3)", "3");
  items0.push_back("4 - ROM ($FE4)", "4");
  items0.push_back("5 - ROM ($FE5)", "5");
  items0.push_back("6 - ROM ($FE6)", "6");
  items0.push_back("7 - RAM ($FE7)", "7");

  items1.push_back("0 - RAM ($FE8)", "0");
  items1.push_back("1 - RAM ($FE9)", "1");
  items1.push_back("2 - RAM ($FEA)", "2");
  items1.push_back("3 - RAM ($FEB)", "3");

  const int lwidth = font.getStringWidth("Set slice for upper 256B: "),
            fwidth = font.getStringWidth("3 - RAM ($FEB)");
  myLower2K =
    new PopUpWidget(boss, font, xpos, ypos-2, fwidth, myLineHeight, items0,
                    "Set slice for lower 2K: ", lwidth, kLowerChanged);
  myLower2K->setTarget(this);
  addFocusWidget(myLower2K);
  ypos += myLower2K->getHeight() + 4;

  myUpper256B =
    new PopUpWidget(boss, font, xpos, ypos-2, fwidth, myLineHeight, items1,
                    "Set slice for upper 256B: ", lwidth, kUpperChanged);
  myUpper256B->setTarget(this);
  addFocusWidget(myUpper256B);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE7Widget::loadConfig()
{
  myLower2K->setSelected(myCart.myCurrentSlice[0]);
  myUpper256B->setSelected(myCart.myCurrentRAM);

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
