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

#include "CartDASH.hxx"
#include "PopUpWidget.hxx"
#include "CartDASHWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDASHWidget::CartridgeDASHWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeDASH& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  uInt32 size = cart.mySize;

  ostringstream info;
  info << "DASH cartridge - (64K ROM + RAM)\n"
       << "  4-64K ROM (1K banks), 32 1K RAM (512b banks)\n"
       << "Each 1K ROM selected by writing to $3F\n"
          "Each 512b RAM selected by writing to $3E\n"
          "  First 512B of bank x (R)\n"
          "  First 512B of bank x+4 (+$800) (W)\n"
       << "Startup bank = 0/-1/-1/0 (ROM)\n";

  // Eventually, we should query this from the debugger/disassembler
  uInt16 start = (cart.myImage[size-3] << 8) | cart.myImage[size-4];
  start -= start % 0x1000;
  info << "Bank RORG" << " = $" << Common::Base::HEX4 << start << "\n";

  int xpos = 10,
      ypos = addBaseInformation(size, "A. Davie", info.str()) + myLineHeight;

  VariantList bankno;
  for(uInt32 i = 0; i < myCart.ROM_BANK_COUNT; ++i)
    bankno.push_back(i);

  VariantList banktype;
  banktype.push_back("ROM", "ROM");
  banktype.push_back("RAM", "RAM");

  for(uInt32 i = 0; i < 4; ++i)
  {
    int xpos_s = xpos, ypos_s = ypos;

    ostringstream label;
    uInt32 addr1 = start + (i*0x400),
           addr2 = addr1 + 0x3FF;
    label << "Set bank " << i << " ($" << Common::Base::HEX4 << addr1 << " - $"
          << addr2 << ") as: ";

    ypos_s = ypos;

    new StaticTextWidget(boss, _font, xpos, ypos, _font.getStringWidth(label.str()),
      myFontHeight, label.str(), kTextAlignLeft);
    ypos += myLineHeight + 8;

    xpos += 20;
    myBankNumber[i] =
      new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("Slot "),
                      myLineHeight, bankno, "Slot ",
                      6*_font.getMaxCharWidth());
    myBankNumber[i]->setTarget(this);
    addFocusWidget(myBankNumber[i]);

    xpos += myBankNumber[i]->getWidth();
    myBankType[i] =
      new PopUpWidget(boss, _font, xpos, ypos-2, 5*_font.getMaxCharWidth(),
                      myLineHeight, banktype, " of ", _font.getStringWidth(" of "));
    myBankType[i]->setTarget(this);
    addFocusWidget(myBankType[i]);

    xpos += myBankType[i]->getWidth() + 10;

    myBankCommit[i] = new ButtonWidget(boss, _font, xpos, ypos-4,
        _font.getStringWidth(" Commit "), myButtonHeight,
        "Commit", bankEnum[i]);
    myBankCommit[i]->setTarget(this);
    addFocusWidget(myBankCommit[i]);

    xpos_s = xpos + myBankCommit[i]->getWidth() + 20;

    myBankState[2*i] = new EditTextWidget(boss, _font, xpos_s, ypos_s,
              w - xpos_s - 10, myLineHeight, "");
    myBankState[2*i]->setEditable(false);
    ypos_s += myLineHeight + 4;
    myBankState[2*i+1] = new EditTextWidget(boss, _font, xpos_s, ypos_s,
              w - xpos_s - 10, myLineHeight, "");
    myBankState[2*i+1]->setEditable(false);

    xpos = 10;
    ypos+= 2 * myLineHeight;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDASHWidget::saveOldState()
{
  myOldState.internalram.clear();
  
  for(uInt32 i = 0; i < this->internalRamSize();i++)
  {
    myOldState.internalram.push_back(myCart.myRAM[i]);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDASHWidget::loadConfig()
{
  updateUIState();
  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDASHWidget::handleCommand(CommandSender* sender,
                                      int cmd, int data, int id)
{
  uInt8 bank = 0x00;

  switch(cmd)
  {
    case kBank0Changed:
cerr << " 0\n";
      break;
    case kBank1Changed:
cerr << " 1\n";
      break;
    case kBank2Changed:
cerr << " 2\n";
      break;
    case kBank3Changed:
cerr << " 3\n";
      break;
  }

#if 0
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
      myRAMBank->setSelectedIndex(0);
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
      myROMBank->setSelectedIndex(0);
    }
  }

  myCart.unlockBank();
  myCart.bank(bank);
  myCart.lockBank();
  invalidate();
  updateUIState();
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeDASHWidget::bankState()
{
  ostringstream& buf = buffer();

#if 0
  uInt16& bank = myCart.myCurrentBank;
  if(bank < 256)
    buf << "ROM bank " << dec << bank % myNumRomBanks << ", RAM inactive";
  else
    buf << "ROM inactive, RAM bank " << bank % myNumRomBanks;
#endif

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDASHWidget::updateUIState()
{
  for(int i = 0; i < 8; i+=2)
  {
    // We need to determine whether each 1K area is all ROM,
    // or a mixture of ROM and RAM
    uInt16 bank0 = myCart.bankInUse[i], bank1 = myCart.bankInUse[i+1];

    uInt8 vBank0 = (bank0 >> myCart.BANK_BITS) & 3;
    uInt8 pBank0 = bank0 & myCart.BIT_BANK_MASK;
    uInt8 vBank1 = (bank1 >> myCart.BANK_BITS) & 3;
    uInt8 pBank1 = bank1 & myCart.BIT_BANK_MASK;

cerr << "Bank " << i << ": " << Common::Base::HEX4 << (int)bank0 << " : "
     << "v = " << Common::Base::HEX2 << (int)vBank0 << ", "
     << "p = " << Common::Base::HEX2 << (int)pBank0 << endl
     << "Bank " << (i+1) << ": " << Common::Base::HEX4 << (int)bank1 << " : "
     << "v = " << Common::Base::HEX2 << (int)vBank1 << ", "
     << "p = " << Common::Base::HEX2 << (int)pBank1 << endl;

    if(bank0 == myCart.BANK_UNDEFINED)  // never accessed
    {
      // If lower bank is undefined, upper bank will be too
      myBankNumber[i/2]->clearSelection();
      myBankType[i/2]->clearSelection();
      myBankState[i]->setText("Undefined");
      myBankState[i+1]->setText("Undefined");
    }
    else if(bank0 & myCart.BITMASK_ROMRAM)  // was RAM mapped here?
    {
      if(bank0 & myCart.BITMASK_LOWERUPPER)
      {
        myBankState[i]->setText("RAM write");
      }
      else
      {
        myBankNumber[i/2]->setSelected(pBank0);
        myBankType[i/2]->setSelected("RAM");
        myBankState[i]->setText("RAM read");
      }
    }
    else
    {
      myBankType[i/2]->setSelected("ROM");
      myBankState[i]->setText("ROM x lower 1K");
      myBankState[i+1]->setText("ROM x upper 1K");
    }
  }
cerr << "--------------------------------------------------\n";

#if 0
  if(myCart.myCurrentBank < 256)
  {
    myROMBank->setSelectedIndex(myCart.myCurrentBank % myNumRomBanks);
    myRAMBank->setSelectedMax();
  }
  else
  {
    myROMBank->setSelectedMax();
    myRAMBank->setSelectedIndex(myCart.myCurrentBank - 256);
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeDASHWidget::internalRamSize() 
{
  return 32*1024;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeDASHWidget::internalRamRPort(int start)
{
  return 0x0000 + start;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeDASHWidget::internalRamDescription() 
{
  ostringstream desc;
  desc << "Accessible 512b at a time via:\n"
       << "  $F000/$F200/$F400/etc used for Read Access\n"
       << "  $F800/$FA00/$FC00/etc used for Write Access (+$800)";
  
  return desc.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeDASHWidget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  for(int i = 0; i < count; i++)
    myRamOld.push_back(myOldState.internalram[start + i]);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeDASHWidget::internalRamCurrent(int start, int count)
{
  myRamCurrent.clear();
  for(int i = 0; i < count; i++)
    myRamCurrent.push_back(myCart.myRAM[start + i]);
  return myRamCurrent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDASHWidget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myRAM[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeDASHWidget::internalRamGetValue(int addr)
{
  return myCart.myRAM[addr];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const CartridgeDASHWidget::BankID CartridgeDASHWidget::bankEnum[4] = {
  kBank0Changed, kBank1Changed, kBank2Changed, kBank3Changed
};
