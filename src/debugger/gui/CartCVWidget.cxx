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

#include "CartCV.hxx"
#include "CartCVWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCVWidget::CartridgeCVWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeCV& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  // Eventually, we should query this from the debugger/disassembler
  uInt16 size = 2048;
  uInt16 start = (cart.myImage[size-3] << 8) | cart.myImage[size-4];
  start -= start % size;

  ostringstream info;
  info << "CV 2K ROM + 1K RAM , non-bankswitched\n"
       << "1024 bytes RAM @ $F000 - $F7FF\n"
       << "  $F000 - $F3FF (R), $F400 - $F7FF (W)\n"
       << "ROM accessible @ $" << Common::Base::HEX4 << start << " - "
       << "$" << (start + size - 1);

  addBaseInformation(cart.mySize, "CommaVid", info.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCVWidget::saveOldState()
{
  myOldState.internalram.clear();
  
  for(uInt32 i = 0; i < this->internalRamSize();i++)
  {
    myOldState.internalram.push_back(myCart.myRAM[i]);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCVWidget::internalRam()
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeCVWidget::internalRamSize() 
{
  return 1024;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeCVWidget::internalRamDescription() 
{
  ostringstream desc;
  desc << "F000-F3FF used for Read Access\n"
  <<      "F400-F7FF used for Write Access";
  
  return desc.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ByteArray CartridgeCVWidget::internalRamOld(int start, int count)
{
  ByteArray ram;
  ram.clear();
  for (int i = 0;i<count;i++)
    ram.push_back(myOldState.internalram[start + i]);
  return ram;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ByteArray CartridgeCVWidget::internalRamCurrent(int start, int count)
{
  ByteArray ram;
  ram.clear();
  for (int i = 0;i<count;i++)
    ram.push_back(myCart.myRAM[start + i]);
  return ram;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCVWidget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myRAM[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeCVWidget::internalRamGetValue(int addr)
{
  return myCart.myRAM[addr];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeCVWidget::internalRamLabel(int addr) 
{
  CartDebug& dbg = instance().debugger().cartDebug();
  return dbg.getLabel(addr + 0x1080, false);
}

