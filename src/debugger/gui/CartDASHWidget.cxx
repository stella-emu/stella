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
       << "  4-64K ROM (1K banks), 32K RAM (512b banks)\n"
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
      ypos = addBaseInformation(size, "A. Davie & T. Jentzsch", info.str()) +
                                myLineHeight;

  VariantList bankno;
  for(uInt32 i = 0; i < myCart.ROM_BANK_COUNT; ++i)
    bankno.push_back(i, i);

  VariantList banktype;
  banktype.push_back("ROM", "ROM");
  banktype.push_back("RAM", "RAM");

  for(uInt32 i = 0; i < 4; ++i)
  {
    int xpos_s = xpos, ypos_s = ypos;

    ostringstream label;
    label << "Set segment " << i << " as: ";

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

    StaticTextWidget* t;
    int addr1 = start + (i*0x400), addr2 = addr1 + 0x1FF;

    label.str("");
    label << Common::Base::HEX4 << addr1 << "-" << Common::Base::HEX4 << addr2;
    t = new StaticTextWidget(boss, _font, xpos_s, ypos_s+2,
          _font.getStringWidth(label.str()), myFontHeight, label.str(), kTextAlignLeft);

    int xoffset = xpos_s+t->getWidth() + 10;
    myBankState[2*i] = new EditTextWidget(boss, _font, xoffset, ypos_s,
              w - xoffset - 10, myLineHeight, "");
    myBankState[2*i]->setEditable(false);
    ypos_s += myLineHeight + 4;

    label.str("");
    label << Common::Base::HEX4 << (addr2 + 1) << "-" << Common::Base::HEX4 << (addr2 + 1 + 0x1FF);
    t = new StaticTextWidget(boss, _font, xpos_s, ypos_s+2,
          _font.getStringWidth(label.str()), myFontHeight, label.str(), kTextAlignLeft);

    myBankState[2*i+1] = new EditTextWidget(boss, _font, xoffset, ypos_s,
              w - xoffset - 10, myLineHeight, "");
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
    myOldState.internalram.push_back(myCart.myRAM[i]);
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
//  uInt8 bank = 0x00;

  switch(cmd)
  {
    case kBank0Changed:
cerr << " 0" << endl;
      break;
    case kBank1Changed:
cerr << " 1" << endl;
      break;
    case kBank2Changed:
cerr << " 2" << endl;
      break;
    case kBank3Changed:
cerr << " 3" << endl;
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

  for(int i = 0; i < 4; ++i)
  {
    uInt16 segment = myCart.segmentInUse[i];

    if(segment == myCart.BANK_UNDEFINED)
    {
      buf << "undefined";
    }
    else
    {
      int number = segment & myCart.BIT_BANK_MASK;
      const char* type = segment & myCart.BITMASK_ROMRAM ? "RAM" : "ROM";

      buf << type << " " << number;
    }
    if(i < 3)
      buf << " / ";
  }

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDASHWidget::updateUIState()
{
  // Set contents for actual banks number and type
  for(int i = 0; i < 4; ++i)
  {
    uInt16 segment = myCart.segmentInUse[i];

    if(segment == myCart.BANK_UNDEFINED)
    {
      myBankNumber[i]->clearSelection();
      myBankType[i]->clearSelection();
    }
    else
    {
      int bankno = segment & myCart.BIT_BANK_MASK;
      const char* banktype = segment & myCart.BITMASK_ROMRAM ? "RAM" : "ROM";

      myBankNumber[i]->setSelected(bankno);
      myBankType[i]->setSelected(banktype);
    }
  }

  // Set description for each 512b bank state
  for(int i = 0; i < 8; ++i)
  {
    uInt16 bank = myCart.bankInUse[i];

    if(bank == myCart.BANK_UNDEFINED)  // never accessed
    {
      myBankState[i]->setText("Undefined");
    }
    else
    {
      ostringstream buf;
      int bankno = bank & myCart.BIT_BANK_MASK;

      if(bank & myCart.BITMASK_ROMRAM)  // was RAM mapped here?
      {
        if(bank & myCart.BITMASK_LOWERUPPER)  // upper is write port
        {
          buf << "RAM " << bankno << " @ $" << Common::Base::HEX4
              << (bankno << myCart.RAM_BANK_TO_POWER) << " (W)";
          myBankState[i]->setText(buf.str());
        }
        else
        {
          buf << "RAM " << bankno << " @ $" << Common::Base::HEX4
              << (bankno << myCart.RAM_BANK_TO_POWER) << " (R)";
          myBankState[i]->setText(buf.str());
        }
      }
      else
      {
        if(bank & myCart.BITMASK_LOWERUPPER)  // upper is high 512b
        {
          buf << "ROM " << bankno << " @ $" << Common::Base::HEX4
              << ((bankno << myCart.RAM_BANK_TO_POWER) + myCart.RAM_BANK_SIZE);
          myBankState[i]->setText(buf.str());
        }
        else
        {
          buf << "ROM " << bankno << " @ $" << Common::Base::HEX4
              << (bankno << myCart.RAM_BANK_TO_POWER);
          myBankState[i]->setText(buf.str());
        }
      }
    }
  }

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
