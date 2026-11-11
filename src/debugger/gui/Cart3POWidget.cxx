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
#include "Cart3PO.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "Layout.hxx"
#include "CartEnhancedWidget.hxx"

using Common::Base;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3POWidget::Cartridge3POWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      Cartridge3PO& cart)
  : CartridgeEnhancedWidget(boss, lfont, nfont, cart),
    myCart3PO{cart}
{
  initialize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge3POWidget::description()
{
  const ByteSpan image = myCart.getImage();
  const uInt16 numRomBanks = myCart.romBankCount();
  const uInt16 numRamBanks = myCart.ramBankCount();

  // Eventually, we should query this from the debugger/disassembler
  const uInt16 start = (((U32(image[0x400 - 3]) << 8U) |
                          U32(image[0x400 - 4])) / 0x1000) * 0x1000;

  return std::format(
    "3PO cartridge - (3E+ with outer banks, 1K{}16MB ROM + 32K{}8MB RAM)\n"
    "  {} 1K ROM banks + {} 512b RAM banks\n"
    "  mapped into four segments\n"
    "Outer bank (bank / 64) selected by writing to $2F\n"
    "ROM bank % 64 & segment selected by writing to $3F\n"
    "RAM outer bank (bank / 64) selected by writing to $2E\n"
    "RAM bank % 64 & segment selected by writing to $3E\n"
    "  Lower 512b of segment for read access\n"
    "  Upper 512b of segment for write access\n"
    "Startup bank = -1/-1/-1/0 (ROM), outer banks 0\n"
    "Bank RORG = ${:04X}\n",
    ELLIPSIS, ELLIPSIS, numRomBanks, numRamBanks, start);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3POWidget::createBankWidgets()
{
  const ByteSpan image = myCart.getImage();
  VariantList banktype;

  VarList::push_back(banktype, "ROM", "ROM");
  VarList::push_back(banktype, "RAM", "RAM");

  // The segment selectors sit in rows of their own, so they align in their own groups
  // (see layoutBankSelect()) rather than joining myLabelColumn
  myBankWidgets.resize(bankSegs());

  const uInt16 start = (((U32(image[0x400 - 3]) << 8U) |
                          U32(image[0x400 - 4])) / 0x1000) * 0x1000;

  for(uInt32 seg = 0; seg < bankSegs(); ++seg)
  {
    VariantList items;
    const size_t bank_off = SZT(seg) * 2;

    mySegLbl[seg] = new LabelWidget(_boss, _font,
        std::format("Set segment {} as", seg));

    CartridgeEnhancedWidget::bankList(std::max(myCart.romBankCount(), myCart.ramBankCount()),
                                      seg, items);
    myBankWidgetLbl[seg] = new LabelWidget(_boss, _font, "Bank");
    myBankWidgets[seg] = new PopUpWidget(_boss, _font, items,
                                         CartridgeEnhancedWidget::Cmd::BankChanged);
    myBankWidgets[seg]->setID(seg);
    myBankWidgets[seg]->setTarget(this);
    addFocusWidget(myBankWidgets[seg]);

    myBankTypeLbl[seg] = new LabelWidget(_boss, _font, "of");
    myBankType[seg] = new PopUpWidget(_boss, _font, banktype, Cmd::RomRamChanged);
    myBankType[seg]->setID(seg);
    myBankType[seg]->setTarget(this);
    addFocusWidget(myBankType[seg]);

    // add "Commit" button (why required?)
    myBankCommit[seg] = new ButtonWidget(_boss, _font,
                                         "Commit", Cmd::ChangeBank);
    myBankCommit[seg]->setID(seg);
    myBankCommit[seg]->setTarget(this);
    addFocusWidget(myBankCommit[seg]);

    const int addr1 = start + (seg * 0x400), addr2 = addr1 + 0x200;

    myAddrLbl[bank_off] = new LabelWidget(_boss, _font,
        std::format("${}-${}", Base::hex4(addr1), Base::hex4(addr1 + 0x1FF)));
    myBankState[bank_off] = new EditTextWidget(_boss, _font, 1);
    myBankState[bank_off]->setEditable(false, true);

    myAddrLbl[bank_off + 1] = new LabelWidget(_boss, _font,
        std::format("${}-${}", Base::hex4(addr2), Base::hex4(addr2 + 0x1FF)));
    myBankState[bank_off + 1] = new EditTextWidget(_boss, _font, 1);
    myBankState[bank_off + 1]->setEditable(false, true);
  }

  // The outer bank register: the bank numbers the next writes to $3F reach
  myOuterLbl = new LabelWidget(_boss, _font, "Outer bank ($2F)");
  myOuterState = new EditTextWidget(_boss, _font, 1);
  myOuterState->setEditable(false, true);

  // The RAM outer register: the RAM bank numbers the next writes to $3E reach
  myRamOuterLbl = new LabelWidget(_boss, _font, "RAM outer ($2E)");
  myRamOuterState = new EditTextWidget(_boss, _font, 1);
  myRamOuterState->setEditable(false, true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3POWidget::layoutBankSelect(GUI::BoxLayout& col) const
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::labeledRow;
  using GUI::LabeledControl;
  using Dir = BoxLayout::Dir;

  // Each kind of control aligns down its own column: the bank and type pop-ups'
  // labels, and the address labels beside the state fields
  std::vector<LabeledControl> banks, types, addrs;
  for(auto* label: myBankWidgetLbl)
    banks.emplace_back(label);
  for(auto* label: myBankTypeLbl)
    types.emplace_back(label);
  for(auto* label: myAddrLbl)
    addrs.emplace_back(label);
  GUI::alignLabels(banks);
  GUI::alignLabels(types);
  GUI::alignLabels(addrs);

  for(uInt32 seg = 0; seg < bankSegs(); ++seg)
  {
    const size_t off = SZT(seg) * 2;

    // The bank / type / commit controls along one row
    auto controls = std::make_unique<BoxLayout>(Dir::Horizontal, _fontWidth);
    controls->addAuto(labeledRow(myBankWidgetLbl[seg], myBankWidgets[seg]));
    controls->addAuto(labeledRow(myBankTypeLbl[seg], myBankType[seg]));
    controls->addAuto(anchoredItem(myBankCommit[seg]));

    // The two address rows on the right, each an address label + filling field
    auto addrCol = std::make_unique<BoxLayout>(Dir::Vertical, VGAP);
    addrCol->addAuto(labeledRow(myAddrLbl[off], myBankState[off], 0, 0, true));
    addrCol->addAuto(labeledRow(myAddrLbl[off + 1], myBankState[off + 1], 0, 0, true));

    // The controls on the left (top row), address rows filling the right
    auto block = std::make_unique<BoxLayout>(Dir::Horizontal, _fontWidth * 2);
    block->addAuto(std::move(controls));
    block->addStretch(std::move(addrCol));

    col.addAuto(anchoredItem(mySegLbl[seg]));
    col.addAuto(std::move(block));
    col.addSpace(VGAP * 2);
  }

  // The two outer registers, their labels aligned with each other
  std::vector<LabeledControl> outers;
  outers.emplace_back(myOuterLbl);
  outers.emplace_back(myRamOuterLbl);
  GUI::alignLabels(outers);
  col.addAuto(labeledRow(myOuterLbl, myOuterState, 0, 0, true));
  col.addSpace(VGAP);
  col.addAuto(labeledRow(myRamOuterLbl, myRamOuterState, 0, 0, true));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3POWidget::loadConfig()
{
  CartridgeEnhancedWidget::loadConfig();
  updateUIState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3POWidget::handleCommand(CommandSender* sender, GuiCmd::Code cmd,
                                       int data, int id)
{
  const uInt16 segment = id;

  switch(cmd)
  {
    case CartridgeEnhancedWidget::Cmd::BankChanged:
    case Cmd::RomRamChanged:
    {
      const bool isROM = myBankType[segment]->getSelectedTag() == "ROM";
      const int bank = myBankWidgets[segment]->getSelected();

      myBankCommit[segment]->setEnabled(
        (isROM  && std::cmp_less(bank, myCart.romBankCount())) ||
        (!isROM && std::cmp_less(bank, myCart.ramBankCount())));
      break;
    }
    case Cmd::ChangeBank:
    {
      // Ignore bank if either number or type hasn't been selected
      if(myBankWidgets[segment]->getSelected() < 0 ||
         myBankType[segment]->getSelected() < 0)
        return;

      // Up to 16384 ROM banks; the outer register is left as it is
      const auto bank = static_cast<uInt16>(myBankWidgets[segment]->getSelected());

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
void Cartridge3POWidget::updateUIState()
{
  // Set description for each 1K segment state (@ each index)
  // Set contents for actual banks number and type (@ each even index)
  for(int seg = 0; std::cmp_less(seg, myCart3PO.myBankSegs); ++seg)
  {
    const uInt16 bank = myCart.getSegmentBank(seg);
    const size_t bank_off = SZT(seg) * 2;

    if(bank >= myCart.romBankCount()) // was RAM mapped here?
    {
      // The RAM bank's number and where it starts in the cart RAM (512 bytes
      // a bank: past $FFFF it takes more than four digits)
      const uInt16 ramBank = bank - myCart.romBankCount();
      const uInt32 ramOffset = U32(ramBank) << 9U;

      myBankState[bank_off]->setText(std::format("RAM #{} @ ${:04X} (R)",
          ramBank, ramOffset));
      myBankState[bank_off + 1]->setText(std::format("RAM #{} @ ${:04X} (W)",
          ramBank, ramOffset));

      myBankWidgets[seg]->setSelectedIndex(ramBank);
      myBankType[seg]->setSelected("RAM");
    }
    else
    {
      // ROM image offsets past $FFFF take more than four digits
      const uInt32 offset = U32(bank) << myCart3PO.myBankShift;

      myBankState[bank_off]->setText(std::format("ROM #{} @ ${:04X}",
          bank, offset));
      myBankState[bank_off + 1]->setText(std::format("ROM #{} @ ${:04X}",
          bank, offset + myCart3PO.myBankSize));

      myBankWidgets[seg]->setSelectedIndex(bank);
      myBankType[seg]->setSelected("ROM");
    }
  }

  const uInt32 first = myCart3PO.outer() * 64U;

  myOuterState->setText(std::format("${}: $3F picks ROM #{}-{}",
      Base::hex2(myCart3PO.outer()), first % myCart.romBankCount(),
      (first + 63) % myCart.romBankCount()));

  const uInt32 ramFirst = myCart3PO.ramOuter() * 64U;

  myRamOuterState->setText(std::format("${}: $3E picks RAM #{}-{}",
      Base::hex2(myCart3PO.ramOuter()), ramFirst % myCart.ramBankCount(),
      (ramFirst + 63) % myCart.ramBankCount()));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge3POWidget::internalRamDescription()
{
  return "Accessible 512b at a time via:\n"
         "  $f000/$f400/$f800/$fc00 for read access\n"
         "  $f200/$f600/$fa00/$fe00 for write access";
}
