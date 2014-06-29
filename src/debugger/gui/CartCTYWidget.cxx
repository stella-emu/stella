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

#include "CartCTY.hxx"
#include "PopUpWidget.hxx"
#include "CartCTYWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCTYWidget::CartridgeCTYWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeCTY& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  uInt16 size = 8 * 4096;

  string info =
    "Chetiry cartridge, eight 4K banks (bank 0 is ARM code and is ignored)\n"
    "64 bytes RAM @ $F000 - $F080\n"
    "  $F040 - $F07F (R), $F000 - $F03F (W)\n"
    "\nTHIS SCHEME IS NOT FULLY IMPLEMENTED OR TESTED\n";

  int xpos = 10,
      ypos = addBaseInformation(size, "Chris D. Walton", info) + myLineHeight;

  VariantList items;
  items.push_back("1 ($FF5)");
  items.push_back("2 ($FF6)");
  items.push_back("3 ($FF7)");
  items.push_back("4 ($FF8)");
  items.push_back("5 ($FF9)");
  items.push_back("6 ($FFA)");
  items.push_back("7 ($FFB)");
  myBank =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("0 ($FFx) "),
                    myLineHeight, items, "Set bank: ",
                    _font.getStringWidth("Set bank: "), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCTYWidget::saveOldState()
{
  myOldState.internalram.clear();
  
  for(uInt32 i = 0; i < this->internalRamSize();i++)
  {
    myOldState.internalram.push_back(myCart.myRAM[i]);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCTYWidget::loadConfig()
{
  myBank->setSelectedIndex(myCart.bank()-1);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCTYWidget::handleCommand(CommandSender* sender,
                                      int cmd, int data, int id)
{
  if(cmd == kBankChanged)
  {
    myCart.unlockBank();
    myCart.bank(myBank->getSelected()+1);
    myCart.lockBank();
    invalidate();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeCTYWidget::bankState()
{
  ostringstream& buf = buffer();

  static const char* spot[] = {
    "", "$FF5", "$FF6", "$FF7", "$FF8", "$FF9", "$FFA", "$FFB"
  };
  uInt16 bank = myCart.bank();
  buf << "Bank = " << dec << bank << ", hotspot = " << spot[bank];

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeCTYWidget::internalRamSize() 
{
  return 64;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeCTYWidget::internalRamRPort(int start)
{
  return 0xF040 + start;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeCTYWidget::internalRamDescription() 
{
  ostringstream desc;
  desc << "$F000 - $F03F used for Write Access\n"
       << "$F040 - $F07F used for Read Access";
  
  return desc.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeCTYWidget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  for(int i = 0; i < count; i++)
    myRamOld.push_back(myOldState.internalram[start + i]);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeCTYWidget::internalRamCurrent(int start, int count)
{
  myRamCurrent.clear();
  for(int i = 0; i < count; i++)
    myRamCurrent.push_back(myCart.myRAM[start + i]);
  return myRamCurrent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCTYWidget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myRAM[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeCTYWidget::internalRamGetValue(int addr)
{
  return myCart.myRAM[addr];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeCTYWidget::internalRamLabel(int addr) 
{
  CartDebug& dbg = instance().debugger().cartDebug();
  return dbg.getLabel(addr + 0xF040, false);
}
