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

#include "Cart3E.hxx"
#include "PopUpWidget.hxx"
#include "Cart3EWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3EWidget::Cartridge3EWidget(
      GuiObject* boss, const GUI::Font& font,
      int x, int y, int w, int h, Cartridge3E& cart)
  : CartDebugWidget(boss, font, x, y, w, h),
    myCart(cart),
    myNumRomBanks(cart.mySize >> 11),
    myNumRamBanks(32)
{
  uInt32 size = cart.mySize;

  ostringstream info;
  info << "Tigervision 3E cartridge - (3E + RAM)\n"
       << "  2-256 2K ROM (currently " << myNumRomBanks << "), 32 1K RAM\n"
       << "(ROM) First 2K selected by writing to $3F\n"
       << "(RAM) First 2K selected by writing to $3E\n"
       << "  $F000 - $F3FF (R), $F400 - $F7FF (W)\n"
       << "Last 2K always points to last 2K of ROM\n";
  if(cart.myStartBank < myNumRomBanks)
    info << "Startup bank = " << cart.myStartBank << " (ROM)\n";
  else
    info << "Startup bank = " << (cart.myStartBank-myNumRomBanks) << " (RAM)\n";

  // Eventually, we should query this from the debugger/disassembler
  uInt16 start = (cart.myImage[size-3] << 8) | cart.myImage[size-4];
  start -= start % 0x1000;
  info << "Bank RORG" << " = $" << HEX4 << start << "\n";

  int xpos = 10,
      ypos = addBaseInformation(size, "TigerVision", info.str()) + myLineHeight;

  StringMap romitems;
  for(uInt32 i = 0; i < myNumRomBanks; ++i)
  {
    const string& b = BSPF_toString(i);
    romitems.push_back(b, b);
  }
  romitems.push_back("Inactive", "");

  StringMap ramitems;
  for(uInt32 i = 0; i < myNumRamBanks; ++i)
  {
    const string& b = BSPF_toString(i);
    ramitems.push_back(b, b);
  }
  ramitems.push_back("Inactive", "");

  ostringstream label;
  label << "Set bank ($" << HEX4 << start << " - $" << (start+0x7FF) << "): ";

  new StaticTextWidget(_boss, _font, xpos, ypos, font.getStringWidth(label.str()),
    myFontHeight, label.str(), kTextAlignLeft);
  ypos += myLineHeight + 8;

  xpos += 40;
  myROMBank =
    new PopUpWidget(boss, font, xpos, ypos-2, font.getStringWidth("0 ($3E) "),
                    myLineHeight, romitems, "ROM ($3F): ",
                    font.getStringWidth("ROM ($3F): "), kROMBankChanged);
  myROMBank->setTarget(this);
  addFocusWidget(myROMBank);

  xpos += myROMBank->getWidth() + 20;
  myRAMBank =
    new PopUpWidget(boss, font, xpos, ypos-2, font.getStringWidth("0 ($3E) "),
                    myLineHeight, ramitems, "RAM ($3E): ",
                    font.getStringWidth("RAM ($3E): "), kRAMBankChanged);
  myRAMBank->setTarget(this);
  addFocusWidget(myRAMBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3EWidget::loadConfig()
{
  if(myCart.myCurrentBank < 256)
  {
    myROMBank->setSelected(myCart.myCurrentBank % myNumRomBanks);
    myRAMBank->setSelectedMax();
  }
  else
  {
    myROMBank->setSelectedMax();
    myRAMBank->setSelected(myCart.myCurrentBank - 256);
  }

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3EWidget::handleCommand(CommandSender* sender,
                                      int cmd, int data, int id)
{
  int bank = -1;

  if(cmd == kROMBankChanged)
  {
    if(myROMBank->getSelected() < (int)myNumRomBanks)
    {
      bank = myROMBank->getSelected();
      myRAMBank->setSelectedMax();
    }
    else
    {
      bank = 256;  // default to first RAM bank
      myRAMBank->setSelected(0);
    }
  }
  else if(cmd == kRAMBankChanged)
  {
    if(myRAMBank->getSelected() < (int)myNumRamBanks)
    {
      myROMBank->setSelectedMax();
      bank = myRAMBank->getSelected() + 256;
    }
    else
    {
      bank = 0;  // default to first ROM bank
      myROMBank->setSelected(0);
    }
  }

  myCart.unlockBank();
  myCart.bank(bank);
  myCart.lockBank();
  invalidate();
}
