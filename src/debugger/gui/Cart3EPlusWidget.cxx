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

#include "Cart3EPlus.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "Cart3EPlusWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3EPlusWidget::Cartridge3EPlusWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, Cartridge3EPlus& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  size_t size = cart.mySize;

  ostringstream info;
  info << "3EPlus cartridge - (4..64K ROM + RAM)\n"
       << "  4..64K ROM (1K banks), ..32K RAM (512b banks)\n"
       << "Each 1K ROM selected by writing to $3F\n"
          "Each 512b RAM selected by writing to $3E\n"
          "  Lower 512b of bank x (R)\n"
          "  Upper 512b of bank x (+$200) (W)\n"
       << "Startup bank = 0/-1/-1/0 (ROM)\n";

  // Eventually, we should query this from the debugger/disassembler
  // Currently the cart starts at bank 0. If we change that, we have to change this too.
  uInt16 start = (cart.myImage[0x400-3] << 8) | cart.myImage[0x400 - 4];
  start &= 0xF000;
  info << "Bank RORG = $" << Common::Base::HEX4 << start << "\n";

  int xpos = 2,
    ypos = addBaseInformation(size, "Thomas Jentzsch", info.str()) + 8;

  VariantList bankno;
  for(uInt32 i = 0; i < myCart.romBankCount(); ++i)
    VarList::push_back(bankno, i, i);

  VariantList banktype;
  VarList::push_back(banktype, "ROM", "ROM");
  VarList::push_back(banktype, "RAM", "RAM");

  for(uInt32 seg = 0; seg < myCart.myBankSegs; ++seg)
  {
    int xpos_s, ypos_s = ypos + 1;

    ostringstream label;
    label << "Set segment " << seg << " as ";

    new StaticTextWidget(boss, _font, xpos, ypos, label.str());
    ypos += myLineHeight + 8;

    xpos += _font.getMaxCharWidth() * 2;
    myBankNumber[seg] =
      new PopUpWidget(boss, _font, xpos, ypos-2, 2 *_font.getMaxCharWidth(),
                      myLineHeight, bankno, "Bank ");
    addFocusWidget(myBankNumber[seg]);

    xpos += myBankNumber[seg]->getWidth();
    myBankType[seg] =
      new PopUpWidget(boss, _font, xpos, ypos-2, 3 *_font.getMaxCharWidth(),
                      myLineHeight, banktype, " of ");
    addFocusWidget(myBankType[seg]);

    xpos = myBankType[seg]->getRight() + _font.getMaxCharWidth();

    // add "Commit" button (why required?)
    myBankCommit[seg] = new ButtonWidget(boss, _font, xpos, ypos-4,
        _font.getStringWidth(" Commit "), myButtonHeight,
        "Commit", bankEnum[seg]);
    myBankCommit[seg]->setTarget(this);
    addFocusWidget(myBankCommit[seg]);

    xpos_s = myBankCommit[seg]->getRight() + _font.getMaxCharWidth() * 2;

    StaticTextWidget* t;
    int addr1 = start + (seg * 0x400), addr2 = addr1 + 0x200;

    label.str("");
    label << "$" << Common::Base::HEX4 << addr1 << "-$" << Common::Base::HEX4 << (addr1 + 0x1FF);
    t = new StaticTextWidget(boss, _font, xpos_s, ypos_s+2, label.str());

    int xoffset = t->getRight() + _font.getMaxCharWidth();
    myBankState[2*seg] = new EditTextWidget(boss, _font, xoffset, ypos_s,
              w - xoffset - 10, myLineHeight, "");
    myBankState[2*seg]->setEditable(false, true);
    ypos_s += myLineHeight + 4;

    label.str("");
    label << "$" << Common::Base::HEX4 << addr2 << "-$" << Common::Base::HEX4 << (addr2 + 0x1FF);
    new StaticTextWidget(boss, _font, xpos_s, ypos_s+2, label.str());

    myBankState[2*seg+1] = new EditTextWidget(boss, _font, xoffset, ypos_s,
              w - xoffset - 10, myLineHeight, "");
    myBankState[2*seg+1]->setEditable(false, true);

    xpos = 2;
    ypos += 2 * myLineHeight;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3EPlusWidget::saveOldState()
{
  myOldState.internalram.clear();

  for(uInt32 i = 0; i < internalRamSize(); ++i)
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
    default:
      break;
  }

  // Ignore bank if either number or type hasn't been selected
  if(myBankNumber[segment]->getSelected() < 0 ||
     myBankType[segment]->getSelected() < 0)
    return;

  uInt8 bank = myBankNumber[segment]->getSelected();

  myCart.unlockBank();

  if(myBankType[segment]->getSelectedTag() == "ROM")
    myCart.bank(bank, segment);
  else
    myCart.bank(bank + myCart.romBankCount(), segment);

  myCart.lockBank();
  invalidate();
  updateUIState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge3EPlusWidget::bankState()
{
  ostringstream& buf = buffer();

  for(int seg = 0; seg < myCart.myBankSegs; ++seg)
  {
    int bank = myCart.getSegmentBank(seg);

    if(bank >= myCart.romBankCount()) // was RAM mapped here?
      buf << " RAM " << bank - myCart.romBankCount();
    else
      buf << " ROM " << bank;
    if(seg < myCart.myBankSegs - 1)
      buf << " /";
  }

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3EPlusWidget::updateUIState()
{
  // Set description for each 1K bank state (@ each index)
  // Set contents for actual banks number and type (@ each even index)
  for(int seg = 0; seg < myCart.myBankSegs; ++seg)
  {
    uInt16 bank = myCart.getSegmentBank(seg);
    ostringstream buf;

    if(bank >= myCart.romBankCount())  // was RAM mapped here?
    {
      uInt16 ramBank = bank - myCart.romBankCount();

      buf << "RAM " << std::dec << ramBank << " @ $" << Common::Base::HEX4
        << (ramBank << myCart.myBankShift) << "(R)";
      myBankState[seg * 2]->setText(buf.str());

      buf.str("");
      buf << "RAM " << std::dec << ramBank << " @ $" << Common::Base::HEX4
        << ((ramBank << myCart.myBankShift) + myCart.myBankSize) << "(W)";
      myBankState[seg * 2 + 1]->setText(buf.str());

      myBankNumber[seg]->setSelected(ramBank);
      myBankType[seg]->setSelected("RAM");
    }
    else
    {
      buf << "ROM " << std::dec << bank << " @ $" << Common::Base::HEX4
        << ((bank << myCart.myBankShift));
      myBankState[seg * 2]->setText(buf.str());

      buf.str("");
      buf << "ROM " << std::dec << bank << " @ $" << Common::Base::HEX4
        << ((bank << myCart.myBankShift) + myCart.myBankSize);
      myBankState[seg * 2 + 1]->setText(buf.str());

      myBankNumber[seg]->setSelected(bank);
      myBankType[seg]->setSelected("ROM");
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
       << "  $f000/$f400/$f800/$fc00 for Read Access\n"
       << "  $f200/$f600/$fa00/$fe00 for Write Access";

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
const std::array<Cartridge3EPlusWidget::BankID, 4> Cartridge3EPlusWidget::bankEnum = {
  kBank0Changed, kBank1Changed, kBank2Changed, kBank3Changed
};
