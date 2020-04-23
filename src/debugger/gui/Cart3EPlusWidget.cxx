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
#include "CartEnhancedWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3EPlusWidget::Cartridge3EPlusWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, Cartridge3EPlus& cart)
  : CartridgeEnhancedWidget(boss, lfont, nfont, x, y, w, h, cart),
    myCart3EP(cart)
{
  initialize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge3EPlusWidget::description()
{
  ostringstream info;
  size_t size;
  const uInt8* image = myCart.getImage(size);
  uInt16 numRomBanks = myCart.romBankCount();
  uInt16 numRamBanks = myCart.ramBankCount();

  info << "3E+ cartridge - (4..64K ROM + RAM)\n"
    << "  " << numRomBanks << " 1K ROM banks + " << numRamBanks << " 512b RAM banks\n"
    << "  mapped into four segments\n"
       "ROM bank & segment selected by writing to $3F\n"
       "RAM bank & segment selected by writing to $3E\n"
       "  Lower 512b of segment for read access\n"
       "  Upper 512b of segment for write access\n"
       "Startup bank = 0/-1/-1/0 (ROM)\n";

  // Eventually, we should query this from the debugger/disassembler
  uInt16 start = (image[0x400 - 3] << 8) | image[0x400 - 4];
  start -= start % 0x1000;
  info << "Bank RORG" << " = $" << Common::Base::HEX4 << start << "\n";

  return info.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3EPlusWidget::bankSelect(int& ypos)
{
  size_t size;
  const uInt8* image = myCart.getImage(size);
  VariantList banktype;

  VarList::push_back(banktype, "ROM", "ROM");
  VarList::push_back(banktype, "RAM", "RAM");

  myBankWidgets = make_unique<PopUpWidget* []>(bankSegs());

  for(uInt32 seg = 0; seg < bankSegs(); ++seg)
  {
    int xpos = 2, xpos_s, ypos_s = ypos + 1, width;
    ostringstream label;
    VariantList items;

    label << "Set segment " << seg << " as ";

    new StaticTextWidget(_boss, _font, xpos, ypos, label.str());
    ypos += myLineHeight + 8;

    xpos += _font.getMaxCharWidth() * 2;

    CartridgeEnhancedWidget::bankList(myCart.romBankCount(), seg, items, width);
    myBankWidgets[seg] =
      new PopUpWidget(_boss, _font, xpos, ypos - 2, width,
                      myLineHeight, items, "Bank ");
    addFocusWidget(myBankWidgets[seg]);

    xpos += myBankWidgets[seg]->getWidth();
    myBankType[seg] =
      new PopUpWidget(_boss, _font, xpos, ypos - 2, 3 * _font.getMaxCharWidth(),
                      myLineHeight, banktype, " of ");
    addFocusWidget(myBankType[seg]);

    xpos = myBankType[seg]->getRight() + _font.getMaxCharWidth();

    // add "Commit" button (why required?)
    myBankCommit[seg] = new ButtonWidget(_boss, _font, xpos, ypos - 4,
                                         _font.getStringWidth(" Commit "), myButtonHeight,
                                         "Commit", bankEnum[seg]);
    myBankCommit[seg]->setTarget(this);
    addFocusWidget(myBankCommit[seg]);

    xpos_s = myBankCommit[seg]->getRight() + _font.getMaxCharWidth() * 2;

    StaticTextWidget* t;
    uInt16 start = (image[0x400 - 3] << 8) | image[0x400 - 4];
    start -= start % 0x1000;
    int addr1 = start + (seg * 0x400), addr2 = addr1 + 0x200;

    label.str("");
    label << "$" << Common::Base::HEX4 << addr1 << "-$" << Common::Base::HEX4 << (addr1 + 0x1FF);
    t = new StaticTextWidget(_boss, _font, xpos_s, ypos_s + 2, label.str());

    int xoffset = t->getRight() + _font.getMaxCharWidth();
    myBankState[2 * seg] = new EditTextWidget(_boss, _font, xoffset, ypos_s,
                                              _w - xoffset - 10, myLineHeight, "");
    myBankState[2 * seg]->setEditable(false, true);
    ypos_s += myLineHeight + 4;

    label.str("");
    label << "$" << Common::Base::HEX4 << addr2 << "-$" << Common::Base::HEX4 << (addr2 + 0x1FF);
    new StaticTextWidget(_boss, _font, xpos_s, ypos_s + 2, label.str());

    myBankState[2 * seg + 1] = new EditTextWidget(_boss, _font, xoffset, ypos_s,
                                                  _w - xoffset - 10, myLineHeight, "");
    myBankState[2 * seg + 1]->setEditable(false, true);

    ypos += 2 * myLineHeight;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3EPlusWidget::loadConfig()
{
  CartridgeEnhancedWidget::loadConfig();
  updateUIState();
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
  if(myBankWidgets[segment]->getSelected() < 0 ||
     myBankType[segment]->getSelected() < 0)
    return;

  uInt8 bank = myBankWidgets[segment]->getSelected();

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
void Cartridge3EPlusWidget::updateUIState()
{
  // Set description for each 1K segment state (@ each index)
  // Set contents for actual banks number and type (@ each even index)
  for(int seg = 0; seg < myCart3EP.myBankSegs; ++seg)
  {
    uInt16 bank = myCart.getSegmentBank(seg);
    ostringstream buf;

    if(bank >= myCart.romBankCount()) // was RAM mapped here?
    {
      uInt16 ramBank = bank - myCart.romBankCount();

      buf << "RAM @ $" << Common::Base::HEX4
        << (ramBank << myCart3EP.myBankShift) << " (R)";
      myBankState[seg * 2]->setText(buf.str());

      buf.str("");
      buf << "RAM @ $" << Common::Base::HEX4
        << ((ramBank << myCart3EP.myBankShift) + myCart3EP.myBankSize) << " (W)";
      myBankState[seg * 2 + 1]->setText(buf.str());

      myBankWidgets[seg]->setSelectedIndex(ramBank);
      myBankType[seg]->setSelected("RAM");
    }
    else
    {
      buf << "ROM @ $" << Common::Base::HEX4
        << ((bank << myCart3EP.myBankShift));
      myBankState[seg * 2]->setText(buf.str());

      buf.str("");
      buf << "ROM @ $" << Common::Base::HEX4
        << ((bank << myCart3EP.myBankShift) + myCart3EP.myBankSize);
      myBankState[seg * 2 + 1]->setText(buf.str());

      myBankWidgets[seg]->setSelectedIndex(bank);
      myBankType[seg]->setSelected("ROM");
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge3EPlusWidget::internalRamDescription()
{
  ostringstream desc;

  desc << "Accessible 512b at a time via:\n"
       << "  $f000/$f400/$f800/$fc00 for read access\n"
       << "  $f200/$f600/$fa00/$fe00 for write access";

  return desc.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const std::array<Cartridge3EPlusWidget::BankID, 4> Cartridge3EPlusWidget::bankEnum = {
  kBank0Changed, kBank1Changed, kBank2Changed, kBank3Changed
};
