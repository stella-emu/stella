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

#include "Cart3F.hxx"
#include "PopUpWidget.hxx"
#include "Cart3FWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3FWidget::Cartridge3FWidget(
      GuiObject* boss, const GUI::Font& font,
      int x, int y, int w, int h, Cartridge3F& cart)
  : CartDebugWidget(boss, font, x, y, w, h),
    myCart(cart)
{
  uInt32 size = cart.mySize;

  ostringstream info;
  info << "Tigervision 3F cartridge, 2-256 2K banks\n"
       << "Startup bank = " << cart.myStartBank << "\n"
       << "First 2K bank selected by writing to $3F\n"
       << "Last 2K always points to last 2K of ROM\n";

  // Eventually, we should query this from the debugger/disassembler
  uInt16 start = (cart.myImage[size-3] << 8) | cart.myImage[size-4];
  start -= start % 0x1000;
  info << "Bank RORG" << " = $" << Common::Base::HEX4 << start << "\n";

  int xpos = 10,
      ypos = addBaseInformation(size, "TigerVision", info.str()) + myLineHeight;

  VariantList items;
  for(uInt16 i = 0; i < cart.bankCount(); ++i)
  {
    const string& b = BSPF_toString(i);
    items.push_back(b + " ($3F)");
  }
  ostringstream label;
  label << "Set bank ($" << Common::Base::HEX4 << start << " - $" <<
           (start+0x7FF) << "): ";
  myBank =
    new PopUpWidget(boss, font, xpos, ypos-2, font.getStringWidth("0 ($3F) "),
                    myLineHeight, items, label.str(),
                    font.getStringWidth(label.str()), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3FWidget::loadConfig()
{
  myBank->setSelected(myCart.myCurrentBank);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3FWidget::handleCommand(CommandSender* sender,
                                      int cmd, int data, int id)
{
  if(cmd == kBankChanged)
  {
    myCart.unlockBank();
    myCart.bank(myBank->getSelected());
    myCart.lockBank();
    invalidate();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge3FWidget::bankState()
{
  ostringstream& buf = buffer();

  buf << "Bank = " << myCart.myCurrentBank << ", hotspot = $3F";

  return buf.str();
}
