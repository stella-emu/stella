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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
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
    myCart3EP{cart}
{
  initialize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge3EPlusWidget::description()
{
  size_t size{0};
  const ByteBuffer& image = myCart.getImage(size);
  const uInt16 numRomBanks = myCart.romBankCount();
  const uInt16 numRamBanks = myCart.ramBankCount();

  // Eventually, we should query this from the debugger/disassembler
  uInt16 start = (image[0x400 - 3] << 8) | image[0x400 - 4];
  start -= start % 0x1000;

  return std::format(
    "3E+ cartridge - (1{}64K ROM + RAM)\n"
    "  {} 1K ROM banks + {} 512b RAM banks\n"
    "  mapped into four segments\n"
    "ROM bank & segment selected by writing to $3F\n"
    "RAM bank & segment selected by writing to $3E\n"
    "  Lower 512b of segment for read access\n"
    "  Upper 512b of segment for write access\n"
    "Startup bank = -1/-1/-1/0 (ROM)\n"
    "Bank RORG = ${:04X}\n",
    ELLIPSIS, numRomBanks, numRamBanks, start);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3EPlusWidget::bankSelect(int& ypos)
{
  size_t size{0};
  const ByteBuffer& image = myCart.getImage(size);
  const int VGAP = myFontHeight / 4;
  VariantList banktype;

  VarList::push_back(banktype, "ROM", "ROM");
  VarList::push_back(banktype, "RAM", "RAM");

  myBankWidgets = std::make_unique<PopUpWidget* []>(bankSegs());

  ypos -= VGAP * 2;

  for(uInt32 seg = 0; seg < bankSegs(); ++seg)
  {
    int xpos = 2, ypos_s = ypos + 1, width = 0;
    VariantList items;

    const string segLabel = std::format("Set segment {} as ", seg);
    new StaticTextWidget(_boss, _font, xpos, ypos, segLabel);
    ypos += myLineHeight + VGAP * 2;

    xpos += _font.getMaxCharWidth() * 2;

    CartridgeEnhancedWidget::bankList(std::max(myCart.romBankCount(), myCart.ramBankCount()),
                                      seg, items, width);
    myBankWidgets[seg] =
      new PopUpWidget(_boss, _font, xpos, ypos - 2, width,
                      myLineHeight, items, "Bank ", 0, kBankChanged);
    myBankWidgets[seg]->setID(seg);
    myBankWidgets[seg]->setTarget(this);
    addFocusWidget(myBankWidgets[seg]);

    xpos += myBankWidgets[seg]->getWidth();
    myBankType[seg] =
      new PopUpWidget(_boss, _font, xpos, ypos - 2, 3 * _font.getMaxCharWidth(),
                      myLineHeight, banktype, " of ", 0, kRomRamChanged);
    myBankType[seg]->setID(seg);
    myBankType[seg]->setTarget(this);
    addFocusWidget(myBankType[seg]);

    xpos = myBankType[seg]->getRight() + _font.getMaxCharWidth();

    // add "Commit" button (why required?)
    myBankCommit[seg] = new ButtonWidget(_boss, _font, xpos, ypos - 4,
                                         _font.getStringWidth(" Commit "), myButtonHeight,
                                         "Commit", kChangeBank);
    myBankCommit[seg]->setID(seg);
    myBankCommit[seg]->setTarget(this);
    addFocusWidget(myBankCommit[seg]);

    const int xpos_s = myBankCommit[seg]->getRight() + _font.getMaxCharWidth() * 2;

    uInt16 start = (image[0x400 - 3] << 8) | image[0x400 - 4];
    start -= start % 0x1000;
    const int addr1 = start + (seg * 0x400), addr2 = addr1 + 0x200;

    const string addrLabel1 = std::format("${:04X}-${:04X}", addr1, addr1 + 0x1FF);
    const auto* t = new StaticTextWidget(_boss, _font, xpos_s, ypos_s + 2, addrLabel1);

    const int xoffset = t->getRight() + _font.getMaxCharWidth();
    const size_t bank_off = static_cast<size_t>(seg) * 2;
    myBankState[bank_off] = new EditTextWidget(_boss, _font, xoffset, ypos_s,
                                               _w - xoffset - 10, myLineHeight, "");
    myBankState[bank_off]->setEditable(false, true);
    ypos_s += myLineHeight + VGAP;

    const string addrLabel2 = std::format("${:04X}-${:04X}", addr2, addr2 + 0x1FF);
    new StaticTextWidget(_boss, _font, xpos_s, ypos_s + 2, addrLabel2);

    myBankState[bank_off + 1] = new EditTextWidget(_boss, _font,
        xoffset, ypos_s, _w - xoffset - 10, myLineHeight, "");
    myBankState[bank_off + 1]->setEditable(false, true);

    ypos += myLineHeight + VGAP * 4;
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
  const uInt16 segment = id;

  switch(cmd)
  {
    case kBankChanged:
    case kRomRamChanged:
    {
      const bool isROM = myBankType[segment]->getSelectedTag() == "ROM";
      const int bank = myBankWidgets[segment]->getSelected();

      myBankCommit[segment]->setEnabled(
        (isROM  && std::cmp_less(bank, myCart.romBankCount())) ||
        (!isROM && std::cmp_less(bank, myCart.ramBankCount())));
      break;
    }
    case kChangeBank:
    {
      // Ignore bank if either number or type hasn't been selected
      if(myBankWidgets[segment]->getSelected() < 0 ||
         myBankType[segment]->getSelected() < 0)
        return;

      const uInt8 bank = myBankWidgets[segment]->getSelected();

      myCart.unlockHotspots();

      if(myBankType[segment]->getSelectedTag() == "ROM")
        myCart.bank(bank, segment);
      else
        myCart.bank(bank + myCart.romBankCount(), segment);

      myCart.lockHotspots();
      invalidate();
      updateUIState();
      break;
    }
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3EPlusWidget::updateUIState()
{
  // Set description for each 1K segment state (@ each index)
  // Set contents for actual banks number and type (@ each even index)
  for(int seg = 0; std::cmp_less(seg, myCart3EP.myBankSegs); ++seg)
  {
    const uInt16 bank = myCart.getSegmentBank(seg);
    const size_t bank_off = static_cast<size_t>(seg) * 2;

    if(bank >= myCart.romBankCount()) // was RAM mapped here?
    {
      const uInt16 ramBank = bank - myCart.romBankCount();

      myBankState[bank_off]->setText(std::format("RAM @ ${:04X} (R)",
        ramBank << myCart3EP.myBankShift));
      myBankState[bank_off + 1]->setText(std::format("RAM @ ${:04X} (W)",
        (ramBank << myCart3EP.myBankShift) + myCart3EP.myBankSize));

      myBankWidgets[seg]->setSelectedIndex(ramBank);
      myBankType[seg]->setSelected("RAM");
    }
    else
    {
      myBankState[bank_off]->setText(std::format("ROM @ ${:04X}",
        bank << myCart3EP.myBankShift));
      myBankState[bank_off + 1]->setText(std::format("ROM @ ${:04X}",
        (bank << myCart3EP.myBankShift) + myCart3EP.myBankSize));

      myBankWidgets[seg]->setSelectedIndex(bank);
      myBankType[seg]->setSelected("ROM");
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge3EPlusWidget::internalRamDescription()
{
  return "Accessible 512b at a time via:\n"
         "  $f000/$f400/$f800/$fc00 for read access\n"
         "  $f200/$f600/$fa00/$fe00 for write access";
}
