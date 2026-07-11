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

#include "Font.hxx"
#include "Cart3EPlus.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "Layout.hxx"
#include "CartEnhancedWidget.hxx"

using Common::Base;

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
  const ByteSpan image = myCart.getImage();
  const uInt16 numRomBanks = myCart.romBankCount();
  const uInt16 numRamBanks = myCart.ramBankCount();

  // Eventually, we should query this from the debugger/disassembler
  const uInt16 start = (((static_cast<uInt16>(image[0x400 - 3]) << 8) |
                                image[0x400 - 4]) / 0x1000) * 0x1000;

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
void Cartridge3EPlusWidget::createBankWidgets()
{
  const ByteSpan image = myCart.getImage();
  VariantList banktype;

  VarList::push_back(banktype, "ROM", "ROM");
  VarList::push_back(banktype, "RAM", "RAM");

  myBankWidgets = std::make_unique<PopUpWidget* []>(bankSegs());

  const uInt16 start = (((static_cast<uInt16>(image[0x400 - 3]) << 8) |
                              image[0x400 - 4]) / 0x1000) * 0x1000;

  for(uInt32 seg = 0; seg < bankSegs(); ++seg)
  {
    VariantList items;
    int width = 0;
    const size_t bank_off = static_cast<size_t>(seg) * 2;

    mySegLabel[seg] = new StaticTextWidget(_boss, _font, 0, 0,
        std::format("Set segment {} as ", seg));

    CartridgeEnhancedWidget::bankList(std::max(myCart.romBankCount(), myCart.ramBankCount()),
                                      seg, items, width);
    myBankWidgets[seg] =
      new PopUpWidget(_boss, _font, 0, 0, width,
                      myLineHeight, items, "Bank ", 0, kBankChanged);
    myBankWidgets[seg]->setID(seg);
    myBankWidgets[seg]->setTarget(this);
    addFocusWidget(myBankWidgets[seg]);

    myBankType[seg] =
      new PopUpWidget(_boss, _font, 0, 0, 3 * _fontWidth,
                      myLineHeight, banktype, " of ", 0, kRomRamChanged);
    myBankType[seg]->setID(seg);
    myBankType[seg]->setTarget(this);
    addFocusWidget(myBankType[seg]);

    // add "Commit" button (why required?)
    myBankCommit[seg] = new ButtonWidget(_boss, _font, 0, 0,
                                         _font.getStringWidth(" Commit "), myButtonHeight,
                                         "Commit", kChangeBank);
    myBankCommit[seg]->setID(seg);
    myBankCommit[seg]->setTarget(this);
    addFocusWidget(myBankCommit[seg]);

    const int addr1 = start + (seg * 0x400), addr2 = addr1 + 0x200;

    myAddrLabel[bank_off] = new StaticTextWidget(_boss, _font, 0, 0,
        std::format("${}-${}", Base::hex4(addr1), Base::hex4(addr1 + 0x1FF)));
    myBankState[bank_off] = new EditTextWidget(_boss, _font, 0, 0, 1, myLineHeight, "");
    myBankState[bank_off]->setEditable(false, true);

    myAddrLabel[bank_off + 1] = new StaticTextWidget(_boss, _font, 0, 0,
        std::format("${}-${}", Base::hex4(addr2), Base::hex4(addr2 + 0x1FF)));
    myBankState[bank_off + 1] = new EditTextWidget(_boss, _font, 0, 0, 1, myLineHeight, "");
    myBankState[bank_off + 1]->setEditable(false, true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3EPlusWidget::layoutBankSelect(GUI::BoxLayout& col)
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::labeledRow;
  using Dir = BoxLayout::Dir;

  const int editH = myBankState[0]->getHeight();
  // All the address labels share a width ("$XXXX-$XXXX")
  const int addrW = myAddrLabel[0]->getWidth() + _fontWidth;

  for(uInt32 seg = 0; seg < bankSegs(); ++seg)
  {
    const size_t off = static_cast<size_t>(seg) * 2;

    // The bank / type / commit controls along one row
    auto controls = std::make_unique<BoxLayout>(Dir::Horizontal, _fontWidth);
    controls->addFixed(anchoredItem(myBankWidgets[seg]), myBankWidgets[seg]->getWidth());
    controls->addFixed(anchoredItem(myBankType[seg]), myBankType[seg]->getWidth());
    controls->addFixed(anchoredItem(myBankCommit[seg]), myBankCommit[seg]->getWidth());
    const int controlsW = myBankWidgets[seg]->getWidth() + myBankType[seg]->getWidth() +
                          myBankCommit[seg]->getWidth() + _fontWidth * 2;

    // The two address rows on the right, each an address label + filling field
    auto addrCol = std::make_unique<BoxLayout>(Dir::Vertical, VGAP);
    addrCol->addFixed(labeledRow(myAddrLabel[off], myBankState[off],
                                 addrW, 0, true), editH);
    addrCol->addFixed(labeledRow(myAddrLabel[off + 1], myBankState[off + 1],
                                 addrW, 0, true), editH);

    // The controls on the left (top row), address rows filling the right
    auto block = std::make_unique<BoxLayout>(Dir::Horizontal, _fontWidth * 2);
    block->addFixed(std::move(controls), controlsW);
    block->addStretch(std::move(addrCol));

    col.addFixed(anchoredItem(mySegLabel[seg]), mySegLabel[seg]->getHeight());
    col.addFixed(std::move(block), editH * 2 + VGAP);
    col.addSpace(VGAP * 2);
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

      myBankState[bank_off]->setText(std::format("RAM @ ${} (R)",
          Base::hex4(ramBank << myCart3EP.myBankShift)));
      myBankState[bank_off + 1]->setText(std::format("RAM @ ${} (W)",
          Base::hex4((ramBank << myCart3EP.myBankShift) + myCart3EP.myBankSize)));

      myBankWidgets[seg]->setSelectedIndex(ramBank);
      myBankType[seg]->setSelected("RAM");
    }
    else
    {
      myBankState[bank_off]->setText(std::format("ROM @ ${}",
          Base::hex4(bank << myCart3EP.myBankShift)));
      myBankState[bank_off + 1]->setText(std::format("ROM @ ${}",
          Base::hex4((bank << myCart3EP.myBankShift) + myCart3EP.myBankSize)));

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
