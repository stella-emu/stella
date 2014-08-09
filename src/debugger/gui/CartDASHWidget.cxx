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
    addFocusWidget(myBankNumber[i]);

    xpos += myBankNumber[i]->getWidth();
    myBankType[i] =
      new PopUpWidget(boss, _font, xpos, ypos-2, 5*_font.getMaxCharWidth(),
                      myLineHeight, banktype, " of ", _font.getStringWidth(" of "));
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
  uInt16 segment = 0;
  switch(cmd)
  {
    case kBank0Changed:
      segment = 0;
      break;
    case kBank1Changed:
      segment = 1;
      break;
    case kBank2Changed:
      segment = 2;
      break;
    case kBank3Changed:
      segment = 3;
      break;
  }

  // Ignore bank if either number or type hasn't been selected
  if(myBankNumber[segment]->getSelected() < 0 ||
     myBankType[segment]->getSelected() < 0)
    return;

  uInt8 bank = (segment << myCart.BANK_BITS) |
               (myBankNumber[segment]->getSelected() & myCart.BIT_BANK_MASK);

  myCart.unlockBank();

  if(myBankType[segment]->getSelectedTag() == "ROM")
    myCart.bankROM(bank);
  else
    myCart.bankRAM(bank);

  myCart.lockBank();
  invalidate();
  updateUIState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeDASHWidget::bankState()
{
  ostringstream& buf = buffer();
  int lastROMBank = -1;
  bool lastSlotRAM = false;

  for(int i = 0; i < 8; ++i)
  {
    uInt16 bank = myCart.bankInUse[i];

    if(bank == myCart.BANK_UNDEFINED)  // never accessed
    {
      buf << " U!";
    }
    else
    {
      int bankno = bank & myCart.BIT_BANK_MASK;

      if(bank & myCart.BITMASK_ROMRAM)  // was RAM mapped here?
      {
        // RAM will always need a '+' placed somewhere, since it always
        // consists of 512B segments
        bool inFirstSlot = (i % 2 == 0);
        if(!(inFirstSlot || lastSlotRAM))
        {
          lastSlotRAM = false;
          buf << " +";
        }

        if(bank & myCart.BITMASK_LOWERUPPER)  // upper is write port
          buf << " RAM " << bankno << "W";
        else
          buf << " RAM " << bankno << "R";

        if(inFirstSlot)
        {
          buf << " +";
          lastSlotRAM = true;
        }
      }
      else
      {
        // ROM can be contiguous, since 2 512B segments can form a single
        // 1K bank; in this case we only show the info once
        bool highBankSame = (i % 2 == 1) && (bankno == lastROMBank);
        if(!highBankSame)
        {
          buf << " ROM " << bankno;
          lastROMBank = bankno;
        }
        else
          lastROMBank = -1;

        lastSlotRAM = false;
      }
    }

    if((i+1) % 2 == 0 && i < 7)
      buf << " /";
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
