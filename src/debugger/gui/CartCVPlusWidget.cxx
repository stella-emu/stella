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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "CartCVPlus.hxx"
#include "PopUpWidget.hxx"
#include "CartCVPlusWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCVPlusWidget::CartridgeCVPlusWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeCVPlus& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  uInt32 size = cart.mySize;

  ostringstream info;
  info << "LS_Dracon CV+ cartridge, 1K RAM, 2-256 2K ROM\n"
       << "1024 bytes RAM @ $F000 - $F7FF\n"
       << "  $F000 - $F3FF (R), $F400 - $F7FF (W)\n"
       << "2048 bytes ROM @ $F800 - $FFFF, by writing to $3D\n"
       << "Startup bank = " << cart.myStartBank << "\n";

  int xpos = 10,
      ypos = addBaseInformation(size, "LS_Dracon / Stephen Anthony",
                                info.str()) + myLineHeight;

  VariantList items;
  for(uInt16 i = 0; i < cart.bankCount(); ++i)
      VarList::push_back(items, Variant(i).toString() + " ($3D)");

  ostringstream label;
  label << "Set bank ($F800 - $FFFF) ";
  myBank =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("xxx ($3D) "),
                    myLineHeight, items, label.str(),
                    _font.getStringWidth(label.str()), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCVPlusWidget::loadConfig()
{
  myBank->setSelectedIndex(myCart.myCurrentBank);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCVPlusWidget::handleCommand(CommandSender* sender,
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
string CartridgeCVPlusWidget::bankState()
{
  ostringstream& buf = buffer();

  buf << "Bank = " << std::dec << myCart.myCurrentBank << ", hotspot = $3D";

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCVPlusWidget::saveOldState()
{
  myOldState.internalram.clear();
  for(uInt32 i = 0; i < this->internalRamSize();i++)
    myOldState.internalram.push_back(myCart.myRAM[i]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeCVPlusWidget::internalRamSize()
{
  return 1024;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeCVPlusWidget::internalRamRPort(int start)
{
  return 0xF000 + start;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeCVPlusWidget::internalRamDescription()
{
  ostringstream desc;
  desc << "$F000 - $F3FF used for Read Access\n"
       << "$F400 - $F7FF used for Write Access";

  return desc.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeCVPlusWidget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  for(int i = 0; i < count; i++)
    myRamOld.push_back(myOldState.internalram[start + i]);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeCVPlusWidget::internalRamCurrent(int start, int count)
{
  myRamCurrent.clear();
  for(int i = 0; i < count; i++)
    myRamCurrent.push_back(myCart.myRAM[start + i]);
  return myRamCurrent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCVPlusWidget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myRAM[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeCVPlusWidget::internalRamGetValue(int addr)
{
  return myCart.myRAM[addr];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeCVPlusWidget::internalRamLabel(int addr)
{
  CartDebug& dbg = instance().debugger().cartDebug();
  return dbg.getLabel(addr + 0xF000, false);
}
