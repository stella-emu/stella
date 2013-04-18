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

#include "CartFA2.hxx"
#include "PopUpWidget.hxx"
#include "CartFA2Widget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFA2Widget::CartridgeFA2Widget(
      GuiObject* boss, const GUI::Font& font,
      int x, int y, int w, int h, CartridgeFA2& cart)
  : CartDebugWidget(boss, font, x, y, w, h),
    myCart(cart)
{
  uInt16 size = cart.mySize;

  ostringstream info;
  info << "Modified FA RAM+, six or seven 4K banks\n"
       << "256 bytes RAM @ $F000 - $F1FF\n"
       << "  $F100 - $F1FF (R), $F000 - $F0FF (W)\n"
       << "RAM can be loaded/saved to Harmony flash by accessing $FF4\n"
       << "Startup bank = " << cart.myStartBank << "\n";

  // Eventually, we should query this from the debugger/disassembler
  for(uInt32 i = 0, offset = 0xFFC, spot = 0xFF5; i < cart.bankCount();
      ++i, offset += 0x1000)
  {
    uInt16 start = (cart.myImage[offset+1] << 8) | cart.myImage[offset];
    start -= start % 0x1000;
    info << "Bank " << i << " @ $" << HEX4 << (start + 0x200) << " - "
         << "$" << (start + 0xFFF) << " (hotspot = $" << (spot+i) << ")\n";
  }

  int xpos = 10,
      ypos = addBaseInformation(size, "Chris D. Walton (Star Castle 2600)",
                info.str(), 15) + myLineHeight;

  StringMap items;
  items.push_back("0 ($FF5)", "0");
  items.push_back("1 ($FF6)", "1");
  items.push_back("2 ($FF7)", "2");
  items.push_back("3 ($FF8)", "3");
  items.push_back("4 ($FF9)", "4");
  items.push_back("5 ($FFA)", "5");
  if(cart.bankCount() == 7)
    items.push_back("6 ($FFB)", "6");

  myBank =
    new PopUpWidget(boss, font, xpos, ypos-2, font.getStringWidth("0 ($FFx) "),
                    myLineHeight, items, "Set bank: ",
                    font.getStringWidth("Set bank: "), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
  ypos += myLineHeight + 20;

  const int bwidth = font.getStringWidth("Erase") + 20;

  StaticTextWidget* t = new StaticTextWidget(boss, font, xpos, ypos,
      font.getStringWidth("Harmony Flash: "),
      myFontHeight, "Harmony Flash: ", kTextAlignLeft);

  xpos += t->getWidth() + 4;
  myFlashErase =
    new ButtonWidget(boss, font, xpos, ypos-4, bwidth, myButtonHeight,
                     "Erase", kFlashErase);
  myFlashErase->setTarget(this);
  addFocusWidget(myFlashErase);
  xpos += myFlashErase->getWidth() + 8;

  myFlashLoad =
    new ButtonWidget(boss, font, xpos, ypos-4, bwidth, myButtonHeight,
                     "Load", kFlashLoad);
  myFlashLoad->setTarget(this);
  addFocusWidget(myFlashLoad);
  xpos += myFlashLoad->getWidth() + 8;

  myFlashSave =
    new ButtonWidget(boss, font, xpos, ypos-4, bwidth, myButtonHeight,
                     "Save", kFlashSave);
  myFlashSave->setTarget(this);
  addFocusWidget(myFlashSave);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFA2Widget::loadConfig()
{
  myBank->setSelected(myCart.myCurrentBank);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFA2Widget::handleCommand(CommandSender* sender,
                                      int cmd, int data, int id)
{
  switch(cmd)
  {
    case kBankChanged:
      myCart.unlockBank();
      myCart.bank(myBank->getSelected());
      myCart.lockBank();
      invalidate();
      break;

    case kFlashErase:
      myCart.flash(0);
      break;

    case kFlashLoad:
      myCart.flash(1);
      break;

    case kFlashSave:
      myCart.flash(2);
      break;
  }
}
