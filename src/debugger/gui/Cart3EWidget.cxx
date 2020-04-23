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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Cart3E.hxx"
#include "PopUpWidget.hxx"
#include "Cart3EWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3EWidget::Cartridge3EWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, Cartridge3E& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart),
    myNumRomBanks(uInt32(cart.mySize >> 11)),
    myNumRamBanks(32)
{
  size_t size = cart.mySize;

  ostringstream info;
  info << "3E cartridge - (3F + RAM)\n"
       << "  2-256 2K ROM (currently " << myNumRomBanks << "), 32 1K RAM\n"
       << "First 2K (ROM) selected by writing to $3F\n"
          "First 2K (RAM) selected by writing to $3E\n"
          "  $F000 - $F3FF (R), $F400 - $F7FF (W)\n"
          "Last 2K always points to last 2K of ROM\n";
  if(cart.startBank() < myNumRomBanks)
    info << "Startup bank = " << cart.startBank() << " (ROM)\n";
  else
    info << "Startup bank = " << (cart.startBank()-myNumRomBanks) << " (RAM)\n";

  // Eventually, we should query this from the debugger/disassembler
  uInt16 start = (cart.myImage[size-3] << 8) | cart.myImage[size-4];
  start -= start % 0x1000;
  info << "Bank RORG" << " = $" << Common::Base::HEX4 << start << "\n";

  int xpos = 2,
      ypos = addBaseInformation(size, "TigerVision", info.str()) + myLineHeight;

  VariantList romitems;
  for(uInt32 i = 0; i < myNumRomBanks; ++i)
    VarList::push_back(romitems, i);
  VarList::push_back(romitems, "Inactive", "");

  VariantList ramitems;
  for(uInt32 i = 0; i < myNumRamBanks; ++i)
    VarList::push_back(ramitems, i);
  VarList::push_back(ramitems, "Inactive", "");

  ostringstream label;
  label << "Set bank ($" << Common::Base::HEX4 << start << " - $"
        << (start+0x7FF) << "): ";

  new StaticTextWidget(_boss, _font, xpos, ypos, _font.getStringWidth(label.str()),
    myFontHeight, label.str(), TextAlign::Left);
  ypos += myLineHeight + 8;

  xpos += 40;
  myROMBank =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("0 ($3E) "),
                    myLineHeight, romitems, "ROM ($3F) ",
                    _font.getStringWidth("ROM ($3F) "), kROMBankChanged);
  myROMBank->setTarget(this);
  addFocusWidget(myROMBank);

  xpos += myROMBank->getWidth() + 20;
  myRAMBank =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("0 ($3E) "),
                    myLineHeight, ramitems, "RAM ($3E) ",
                    _font.getStringWidth("RAM ($3E) "), kRAMBankChanged);
  myRAMBank->setTarget(this);
  addFocusWidget(myRAMBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3EWidget::saveOldState()
{
  myOldState.internalram.clear();

  for(uInt32 i = 0; i < internalRamSize(); ++i)
    myOldState.internalram.push_back(myCart.myRAM[i]);

  myOldState.bank = myCart.myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3EWidget::loadConfig()
{
  if(myCart.myCurrentBank < 256)
  {
    myROMBank->setSelectedIndex(myCart.myCurrentBank % myNumRomBanks, myOldState.bank != myCart.myCurrentBank);
    myRAMBank->setSelectedMax(myOldState.bank >= 256);
  }
  else
  {
    myROMBank->setSelectedMax(myOldState.bank < 256);
    myRAMBank->setSelectedIndex(myCart.myCurrentBank - 256, myOldState.bank != myCart.myCurrentBank);
  }

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3EWidget::handleCommand(CommandSender* sender,
                                      int cmd, int data, int id)
{
  uInt16 bank = 0;

  if(cmd == kROMBankChanged)
  {
    if(myROMBank->getSelected() < int(myNumRomBanks))
    {
      bank = myROMBank->getSelected();
      myRAMBank->setSelectedMax();
    }
    else
    {
      bank = 256;  // default to first RAM bank
      myRAMBank->setSelectedIndex(0);
    }
  }
  else if(cmd == kRAMBankChanged)
  {
    if(myRAMBank->getSelected() < int(myNumRamBanks))
    {
      myROMBank->setSelectedMax();
      bank = myRAMBank->getSelected() + 256;
    }
    else
    {
      bank = 0;  // default to first ROM bank
      myROMBank->setSelectedIndex(0);
    }
  }

  myCart.unlockBank();
  myCart.bank(bank);
  myCart.lockBank();
  invalidate();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge3EWidget::bankState()
{
  ostringstream& buf = buffer();

  uInt16& bank = myCart.myCurrentBank;
  if(bank < 256)
    buf << "ROM bank #" << std::dec << bank % myNumRomBanks << ", RAM inactive";
  else
    buf << "ROM inactive, RAM bank #" << std::dec << bank % myNumRamBanks;

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Cartridge3EWidget::internalRamSize()
{
  return 32*1024;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Cartridge3EWidget::internalRamRPort(int start)
{
  return 0x0000 + start;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge3EWidget::internalRamDescription()
{
  ostringstream desc;
  desc << "Accessible 1K at a time via:\n"
       << "  $F000 - $F3FF used for Read Access\n"
       << "  $F400 - $F7FF used for Write Access";

  return desc.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& Cartridge3EWidget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  for(int i = 0; i < count; i++)
    myRamOld.push_back(myOldState.internalram[start + i]);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& Cartridge3EWidget::internalRamCurrent(int start, int count)
{
  myRamCurrent.clear();
  for(int i = 0; i < count; i++)
    myRamCurrent.push_back(myCart.myRAM[start + i]);
  return myRamCurrent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3EWidget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myRAM[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Cartridge3EWidget::internalRamGetValue(int addr)
{
  return myCart.myRAM[addr];
}
