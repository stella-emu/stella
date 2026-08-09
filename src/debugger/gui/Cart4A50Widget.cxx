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

#include "Cart4A50.hxx"
#include "PopUpWidget.hxx"
#include "Layout.hxx"
#include "Cart4A50Widget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge4A50Widget::Cartridge4A50Widget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      Cartridge4A50& cart)
  : CartDebugWidget(boss, lfont, nfont),
    myCart{cart}
{
  constexpr string_view info =
    "4A50 cartridge - 128K ROM and 32K RAM, split in various bank configurations\n"
    "Multiple hotspots, see documentation for further details\n"
    "Lower bank region (2K)   : $F000 - $F7FF\n"
    "Middle bank region (1.5K): $F800 - $FDFF\n"
    "High bank region (256B)  : $FE00 - $FEFF\n"
    "Fixed (last 256B of ROM) : $FF00 - $FFFF\n";

  createBaseInformation(cart.mySize, "John Payson / Supercat", info);

  VariantList items16, items32, items128, items256;
  for(uInt32 i = 0; i < 16; ++i)
    VarList::push_back(items16, i);
  VarList::push_back(items16, "Inactive", "");

  for(uInt32 i = 0; i < 32; ++i)
    VarList::push_back(items32, i);
  VarList::push_back(items32, "Inactive", "");

  for(uInt32 i = 0; i < 128; ++i)
    VarList::push_back(items128, i);
  VarList::push_back(items128, "Inactive", "");

  for(uInt32 i = 0; i < 256; ++i)
    VarList::push_back(items256, i);
  VarList::push_back(items256, "Inactive", "");

  // Each region: a heading, with a ROM and a RAM selector indented beneath it.
  // Every widget is created at a placeholder position; reflow() positions them
  const auto addRegion = [&](LabelWidget*& label, string_view heading,
                             LabelWidget*& romLabel, PopUpWidget*& rom,
                             const VariantList& romItems, GuiCmd::Code romCmd,
                             LabelWidget*& ramLabel, PopUpWidget*& ram,
                             const VariantList& ramItems, GuiCmd::Code ramCmd) {
    label = new LabelWidget(_boss, _font, heading);

    romLabel = new LabelWidget(_boss, _font, "ROM");
    rom = new PopUpWidget(boss, _font, romItems, romCmd);
    rom->setTarget(this);
    addFocusWidget(rom);

    ramLabel = new LabelWidget(_boss, _font, "RAM");
    ram = new PopUpWidget(boss, _font, ramItems, ramCmd);
    ram->setTarget(this);
    addFocusWidget(ram);
  };

  addRegion(myLowerLbl,  "Set lower 2K region ($F000 - $F7FF):",
            myROMLowerLbl,  myROMLower,  items32,  Cmd::RomLowerChanged,
            myRAMLowerLbl,  myRAMLower,  items16,  Cmd::RamLowerChanged);
  addRegion(myMiddleLbl, "Set middle 1.5K region ($F800 - $FDFF):",
            myROMMiddleLbl, myROMMiddle, items32,  Cmd::RomMiddleChanged,
            myRAMMiddleLbl, myRAMMiddle, items16,  Cmd::RamMiddleChanged);
  addRegion(myHighLbl,   "Set high 256B region ($FE00 - $FEFF):",
            myROMHighLbl,   myROMHigh,   items256, Cmd::RomHighChanged,
            myRAMHighLbl,   myRAMHigh,   items128, Cmd::RamHighChanged);

  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge4A50Widget::layoutContent(GUI::BoxLayout& col) const
{
  // The ROM selectors' labels form a column, as do the RAM ones: each gets one
  // label column, and all six pop-ups get one box width, so the two columns
  // line up down the tab
  GUI::alignLabels({{myROMLowerLbl}, {myROMMiddleLbl}, {myROMHighLbl}});
  GUI::alignLabels({{myRAMLowerLbl}, {myRAMMiddleLbl}, {myRAMHighLbl}});
  GUI::alignPopUps({myROMLower, myROMMiddle, myROMHigh,
                    myRAMLower, myRAMMiddle, myRAMHigh});

  layoutRegion(col, myLowerLbl,  myROMLowerLbl,  myROMLower,  myRAMLowerLbl,  myRAMLower);
  col.addSpace(_lineHeight);
  layoutRegion(col, myMiddleLbl, myROMMiddleLbl, myROMMiddle, myRAMMiddleLbl, myRAMMiddle);
  col.addSpace(_lineHeight);
  layoutRegion(col, myHighLbl,   myROMHighLbl,   myROMHigh,   myRAMHighLbl,   myRAMHigh);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge4A50Widget::layoutRegion(GUI::BoxLayout& col, LabelWidget* label,
                                       LabelWidget* romLabel, PopUpWidget* rom,
                                       LabelWidget* ramLabel, PopUpWidget* ram) const
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::labeledRow;

  auto row = std::make_unique<BoxLayout>(BoxLayout::Dir::Horizontal, _fontWidth * 2);
  row->addSpace(_fontWidth * 4);   // the selectors sit in from their heading
  row->addAuto(labeledRow(romLabel, rom));
  row->addAuto(labeledRow(ramLabel, ram));

  col.addAuto(anchoredItem(label));
  col.addAuto(std::move(row));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge4A50Widget::loadConfig()
{
  const auto setRomRam = [](PopUpWidget* rom, PopUpWidget* ram,
                            bool isRom, int romIdx, int ramIdx) {
    if(isRom) { rom->setSelectedIndex(romIdx); ram->setSelectedMax(); }
    else      { rom->setSelectedMax(); ram->setSelectedIndex(ramIdx); }
  };

  // Lower bank
  setRomRam(myROMLower,  myRAMLower,  myCart.myIsRomLow,
    (myCart.mySliceLow    >> 11) & 0x1F, (myCart.mySliceLow    >> 11) & 0x0F);
  // Middle bank
  setRomRam(myROMMiddle, myRAMMiddle, myCart.myIsRomMiddle,
    (myCart.mySliceMiddle >> 11) & 0x1F, (myCart.mySliceMiddle >> 11) & 0x0F);
  // High bank
  setRomRam(myROMHigh,   myRAMHigh,   myCart.myIsRomHigh,
    (myCart.mySliceHigh   >> 11) & 0xFF, (myCart.mySliceHigh   >> 11) & 0x7F);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge4A50Widget::handleCommand(CommandSender* sender, GuiCmd::Code cmd,
                                        int data, int id)
{
  myCart.unlockHotspots();

  switch(cmd)
  {
    case Cmd::RomLowerChanged:
      if(myROMLower->getSelected() < 32)
      {
        myCart.bankROMLower(myROMLower->getSelected());
        myRAMLower->setSelectedMax();
      }
      else
      {
        // default to first RAM bank
        myRAMLower->setSelectedIndex(0);
        myCart.bankRAMLower(0);
      }
      break;

    case Cmd::RamLowerChanged:
      if(myRAMLower->getSelected() < 16)
      {
        myROMLower->setSelectedMax();
        myCart.bankRAMLower(myRAMLower->getSelected());
      }
      else
      {
        // default to first ROM bank
        myROMLower->setSelectedIndex(0);
        myCart.bankROMLower(0);
      }
      break;

    case Cmd::RomMiddleChanged:
      if(myROMMiddle->getSelected() < 32)
      {
        myCart.bankROMMiddle(myROMMiddle->getSelected());
        myRAMMiddle->setSelectedMax();
      }
      else
      {
        // default to first RAM bank
        myRAMMiddle->setSelectedIndex(0);
        myCart.bankRAMMiddle(0);
      }
      break;

    case Cmd::RamMiddleChanged:
      if(myRAMMiddle->getSelected() < 16)
      {
        myROMMiddle->setSelectedMax();
        myCart.bankRAMMiddle(myRAMMiddle->getSelected());
      }
      else
      {
        // default to first ROM bank
        myROMMiddle->setSelectedIndex(0);
        myCart.bankROMMiddle(0);
      }
      break;

    case Cmd::RomHighChanged:
      if(myROMHigh->getSelected() < 256)
      {
        myCart.bankROMHigh(myROMHigh->getSelected());
        myRAMHigh->setSelectedMax();
      }
      else
      {
        // default to first RAM bank
        myRAMHigh->setSelectedIndex(0);
        myCart.bankRAMHigh(0);
      }
      break;

    case Cmd::RamHighChanged:
      if(myRAMHigh->getSelected() < 128)
      {
        myROMHigh->setSelectedMax();
        myCart.bankRAMHigh(myRAMHigh->getSelected());
      }
      else
      {
        // default to first ROM bank
        myROMHigh->setSelectedIndex(0);
        myCart.bankROMHigh(0);
      }
      break;

    default:
      break;
  }

  myCart.lockHotspots();
  invalidate();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge4A50Widget::bankState()
{
  return std::format("L/M/H = {} bank {} / {} bank {} / {} bank {}",
    myCart.myIsRomLow    ? "ROM" : "RAM",
    myCart.myIsRomLow    ? (myCart.mySliceLow    >> 11) & 0x1F
                         : (myCart.mySliceLow    >> 11) & 0x0F,
    myCart.myIsRomMiddle ? "ROM" : "RAM",
    myCart.myIsRomMiddle ? (myCart.mySliceMiddle >> 11) & 0x1F
                         : (myCart.mySliceMiddle >> 11) & 0x0F,
    myCart.myIsRomHigh   ? "ROM" : "RAM",
    myCart.myIsRomHigh   ? (myCart.mySliceHigh   >> 11) & 0xFF
                         : (myCart.mySliceHigh   >> 11) & 0x7F);
}
