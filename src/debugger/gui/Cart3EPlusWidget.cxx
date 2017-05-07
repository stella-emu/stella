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

#include "Cart3EPlus.hxx"
#include "PopUpWidget.hxx"
#include "Cart3EPlusWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3EPlusWidget::Cartridge3EPlusWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, Cartridge3EPlus& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  uInt32 size = cart.mySize;

  ostringstream info;
  info << "3EPlus cartridge - (64K ROM + RAM)\n"
       << "  4-64K ROM (1K banks), 32K RAM (512b banks)\n"
       << "Each 1K ROM selected by writing to $3F\n"
          "Each 512b RAM selected by writing to $3E\n"
          "  Lower 512B of bank x (R)\n"
          "  Upper 512B of bank x (+$200) (W)\n"
       << "Startup bank = 0/-1/-1/0 (ROM)\n";

  // Eventually, we should query this from the debugger/disassembler
  uInt16 start = (cart.myImage[size-3] << 8) | cart.myImage[size-4];
  start -= start % 0x1000;
  info << "Bank RORG" << " = $" << Common::Base::HEX4 << start << "\n";

  int xpos = 10,
      ypos = addBaseInformation(size, "T. Jentzsch", info.str()) +
                                myLineHeight;

  VariantList bankno;
  for(uInt32 i = 0; i < myCart.ROM_BANK_COUNT; ++i)
    VarList::push_back(bankno, i, i);

  VariantList banktype;
  VarList::push_back(banktype, "ROM", "ROM");
  VarList::push_back(banktype, "RAM", "RAM");

  for(uInt32 i = 0; i < 4; ++i)
  {
    int xpos_s, ypos_s = ypos;

    ostringstream label;
    label << "Set segment " << i << " as ";

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
    myBankState[2*i]->setEditable(false, true);
    ypos_s += myLineHeight + 4;

    label.str("");
    label << Common::Base::HEX4 << (addr2 + 1) << "-" << Common::Base::HEX4 << (addr2 + 1 + 0x1FF);
    new StaticTextWidget(boss, _font, xpos_s, ypos_s+2,
        _font.getStringWidth(label.str()), myFontHeight, label.str(), kTextAlignLeft);

    myBankState[2*i+1] = new EditTextWidget(boss, _font, xoffset, ypos_s,
              w - xoffset - 10, myLineHeight, "");
    myBankState[2*i+1]->setEditable(false, true);

    xpos = 10;
    ypos+= 2 * myLineHeight;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3EPlusWidget::saveOldState()
{
  myOldState.internalram.clear();

  for(uInt32 i = 0; i < this->internalRamSize();i++)
    myOldState.internalram.push_back(myCart.myRAM[i]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3EPlusWidget::loadConfig()
{
  updateUIState();
  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3EPlusWidget::handleCommand(CommandSender* sender,
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
string Cartridge3EPlusWidget::bankState()
{
  ostringstream& buf = buffer();

  // In this scheme, consecutive 512b segments are either both ROM or both RAM;
  // we only need to look at the lower segment to determine what the 1K bank is
  for(int i = 0; i < 4; ++i)
  {
    uInt16 bank = myCart.bankInUse[i*2];

    if(bank == myCart.BANK_UNDEFINED)  // never accessed
    {
      buf << " U!";
    }
    else
    {
      int bankno = bank & myCart.BIT_BANK_MASK;

      if(bank & myCart.BITMASK_ROMRAM) // was RAM mapped here?
        buf << " RAM " << bankno;
      else
        buf << " ROM " << bankno;
    }
    if(i < 3)
      buf << " /";
  }

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3EPlusWidget::updateUIState()
{
  // Set description for each 512b bank state (@ each index)
  // Set contents for actual banks number and type (@ each even index)
  for(int i = 0; i < 8; ++i)
  {
    uInt16 bank = myCart.bankInUse[i];

    if(bank == myCart.BANK_UNDEFINED)  // never accessed
    {
      myBankState[i]->setText("Undefined");
      if(i % 2 == 0)
      {
        myBankNumber[i/2]->clearSelection();
        myBankType[i/2]->clearSelection();
      }
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

        if(i % 2 == 0)
        {
          myBankNumber[i/2]->setSelected(bankno);
          myBankType[i/2]->setSelected("RAM");
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

        if(i % 2 == 0)
        {
          myBankNumber[i/2]->setSelected(bankno);
          myBankType[i/2]->setSelected("ROM");
        }
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Cartridge3EPlusWidget::internalRamSize()
{
  return 32*1024;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Cartridge3EPlusWidget::internalRamRPort(int start)
{
  return 0x0000 + start;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge3EPlusWidget::internalRamDescription()
{
  ostringstream desc;
  desc << "Accessible 512b at a time via:\n"
       << "  $F000/$F400/$F800/etc used for Read Access\n"
       << "  $F200/$F600/$FA00/etc used for Write Access (+$200)";

  return desc.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& Cartridge3EPlusWidget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  for(int i = 0; i < count; i++)
    myRamOld.push_back(myOldState.internalram[start + i]);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& Cartridge3EPlusWidget::internalRamCurrent(int start, int count)
{
  myRamCurrent.clear();
  for(int i = 0; i < count; i++)
    myRamCurrent.push_back(myCart.myRAM[start + i]);
  return myRamCurrent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3EPlusWidget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myRAM[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Cartridge3EPlusWidget::internalRamGetValue(int addr)
{
  return myCart.myRAM[addr];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Cartridge3EPlusWidget::BankID Cartridge3EPlusWidget::bankEnum[4] = {
  kBank0Changed, kBank1Changed, kBank2Changed, kBank3Changed
};
