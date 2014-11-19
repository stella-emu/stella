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

#include "CartFA2.hxx"
#include "PopUpWidget.hxx"
#include "CartFA2Widget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFA2Widget::CartridgeFA2Widget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeFA2& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
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
    info << "Bank " << i << " @ $" << Common::Base::HEX4 << (start + 0x200) << " - "
         << "$" << (start + 0xFFF) << " (hotspot = $" << (spot+i) << ")\n";
  }

  int xpos = 10,
      ypos = addBaseInformation(size, "Chris D. Walton (Star Castle 2600)",
                info.str(), 15) + myLineHeight;

  VariantList items;
  VarList::push_back(items, "0 ($FF5)");
  VarList::push_back(items, "1 ($FF6)");
  VarList::push_back(items, "2 ($FF7)");
  VarList::push_back(items, "3 ($FF8)");
  VarList::push_back(items, "4 ($FF9)");
  VarList::push_back(items, "5 ($FFA)");
  if(cart.bankCount() == 7)
    VarList::push_back(items, "6 ($FFB)");

  myBank =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("0 ($FFx) "),
                    myLineHeight, items, "Set bank: ",
                    _font.getStringWidth("Set bank: "), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
  ypos += myLineHeight + 20;

  const int bwidth = _font.getStringWidth("Erase") + 20;

  StaticTextWidget* t = new StaticTextWidget(boss, _font, xpos, ypos,
      _font.getStringWidth("Harmony Flash: "),
      myFontHeight, "Harmony Flash: ", kTextAlignLeft);

  xpos += t->getWidth() + 4;
  myFlashErase =
    new ButtonWidget(boss, _font, xpos, ypos-4, bwidth, myButtonHeight,
                     "Erase", kFlashErase);
  myFlashErase->setTarget(this);
  addFocusWidget(myFlashErase);
  xpos += myFlashErase->getWidth() + 8;

  myFlashLoad =
    new ButtonWidget(boss, _font, xpos, ypos-4, bwidth, myButtonHeight,
                     "Load", kFlashLoad);
  myFlashLoad->setTarget(this);
  addFocusWidget(myFlashLoad);
  xpos += myFlashLoad->getWidth() + 8;

  myFlashSave =
    new ButtonWidget(boss, _font, xpos, ypos-4, bwidth, myButtonHeight,
                     "Save", kFlashSave);
  myFlashSave->setTarget(this);
  addFocusWidget(myFlashSave);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFA2Widget::saveOldState()
{
  myOldState.internalram.clear();
  
  for(uInt32 i = 0; i < this->internalRamSize();i++)
  {
    myOldState.internalram.push_back(myCart.myRAM[i]);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFA2Widget::loadConfig()
{
  myBank->setSelectedIndex(myCart.myCurrentBank);

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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeFA2Widget::bankState()
{
  ostringstream& buf = buffer();

  static const char* spot[] = {
    "$FF5", "$FF6", "$FF7", "$FF8", "$FF9", "$FFA", "$FFB"
  };
  buf << "Bank = " << dec << myCart.myCurrentBank
      << ", hotspot = " << spot[myCart.myCurrentBank];

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeFA2Widget::internalRamSize() 
{
  return 256;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeFA2Widget::internalRamRPort(int start)
{
  return 0xF100 + start;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeFA2Widget::internalRamDescription() 
{
  ostringstream desc;
  desc << "$F000 - $F0FF used for Write Access\n"
       << "$F100 - $F1FF used for Read Access";
  
  return desc.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeFA2Widget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  for(int i = 0; i < count; i++)
    myRamOld.push_back(myOldState.internalram[start + i]);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeFA2Widget::internalRamCurrent(int start, int count)
{
  myRamCurrent.clear();
  for(int i = 0; i < count; i++)
    myRamCurrent.push_back(myCart.myRAM[start + i]);
  return myRamCurrent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFA2Widget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myRAM[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeFA2Widget::internalRamGetValue(int addr)
{
  return myCart.myRAM[addr];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeFA2Widget::internalRamLabel(int addr) 
{
  CartDebug& dbg = instance().debugger().cartDebug();
  return dbg.getLabel(addr + 0xF100, false);
}
