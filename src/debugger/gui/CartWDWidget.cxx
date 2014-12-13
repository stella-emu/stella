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

#include "CartWD.hxx"
#include "PopUpWidget.hxx"
#include "CartWDWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeWDWidget::CartridgeWDWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeWD& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  string info =
    "This scheme has eight 1K slices, which can be mapped into four 1K "
    "segments in various combinations.  Each 'bank' selects a predefined "
    "segment arrangement (indicated in square brackets)\n"
    "The last four banks swap in an extra 3 bytes from above the 8K "
    "cart boundary into the third (uppermost) segment at $3FC - $3FE\n\n"
    "64 bytes RAM @ $F000 - $F080\n"
    "  $F000 - $F03F (R), $F040 - $F07F (W)\n";

  int xpos = 10,
      ypos = addBaseInformation(myCart.mySize, "Wickstead Design", info) + myLineHeight;

  VariantList items;
  VarList::push_back(items, "0  ($30) [0,0,1,2]", 0);
  VarList::push_back(items, "1  ($31) [0,1,3,2]", 1);
  VarList::push_back(items, "2  ($32) [4,5,6,7]", 2);
  VarList::push_back(items, "3  ($33) [7,4,3,2]", 3);
  VarList::push_back(items, "4  ($34) [0,0,6,7]", 4);
  VarList::push_back(items, "5  ($35) [0,1,7,6]", 5);
  VarList::push_back(items, "6  ($36) [3,2,4,5]", 6);
  VarList::push_back(items, "7  ($37) [6,0,5,1]", 7);
  VarList::push_back(items, "8  ($38) [0,0,1,2]", 8);
  VarList::push_back(items, "9  ($39) [0,1,3,2]", 9);
  VarList::push_back(items, "10 ($3A) [4,5,6,7]", 10);
  VarList::push_back(items, "11 ($3B) [7,4,3,2]", 11);
  VarList::push_back(items, "12 ($3C) [0,0,6,7*]", 12);
  VarList::push_back(items, "13 ($3D) [0,1,7,6*]", 13);
  VarList::push_back(items, "14 ($3E) [3,2,4,5*]", 14);
  VarList::push_back(items, "15 ($3F) [6,0,5,1*]", 15);
  myBank = new PopUpWidget(boss, _font, xpos, ypos-2,
                    _font.getStringWidth("15 ($3F) [6,0,5,1*]"),
                    myLineHeight, items, "Set bank: ",
                    _font.getStringWidth("Set bank: "), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeWDWidget::saveOldState()
{
  myOldState.internalram.clear();
  
  for(uInt32 i = 0; i < internalRamSize(); ++i)
    myOldState.internalram.push_back(myCart.myRAM[i]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeWDWidget::loadConfig()
{
  myBank->setSelectedIndex(myCart.getBank());

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeWDWidget::handleCommand(CommandSender* sender,
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
string CartridgeWDWidget::bankState()
{
  ostringstream& buf = buffer();

  static const char* segments[] = {
    "[0,0,1,2]",  "[0,1,3,2]",  "[4,5,6,7]",  "[7,4,3,2]", 
    "[0,0,6,7]",  "[0,1,7,6]",  "[3,2,4,5]",  "[6,0,5,1]", 
    "[0,0,1,2]",  "[0,1,3,2]",  "[4,5,6,7]",  "[7,4,3,2]", 
    "[0,0,6,7*]", "[0,1,7,6*]", "[3,2,4,5*]", "[6,0,5,1*]"
  };
  uInt16 bank = myCart.getBank();
  buf << "Bank = " << dec << bank << ", segments = " << segments[bank];

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeWDWidget::internalRamSize() 
{
  return 64;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeWDWidget::internalRamRPort(int start)
{
  return 0xF000 + start;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeWDWidget::internalRamDescription() 
{
  ostringstream desc;
  desc << "$F000 - $F03F used for Read Access\n"
       << "$F040 - $F07F used for Write Access";
  
  return desc.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeWDWidget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  for(int i = 0; i < count; ++i)
    myRamOld.push_back(myOldState.internalram[start + i]);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeWDWidget::internalRamCurrent(int start, int count)
{
  myRamCurrent.clear();
  for(int i = 0; i < count; ++i)
    myRamCurrent.push_back(myCart.myRAM[start + i]);
  return myRamCurrent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeWDWidget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myRAM[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeWDWidget::internalRamGetValue(int addr)
{
  return myCart.myRAM[addr];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeWDWidget::internalRamLabel(int addr) 
{
  CartDebug& dbg = instance().debugger().cartDebug();
  return dbg.getLabel(addr + 0xF000, false);
}
