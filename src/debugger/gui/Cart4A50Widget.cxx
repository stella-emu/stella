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
  const auto addRegion = [&](StaticTextWidget*& label, string_view heading,
                             PopUpWidget*& rom, const VariantList& romItems, int romCmd,
                             PopUpWidget*& ram, const VariantList& ramItems, int ramCmd) {
    label = new StaticTextWidget(_boss, _font, 0, 0, heading);

    rom = new PopUpWidget(boss, _font, 0, 0, romItems, "ROM", 0, romCmd);
    rom->setTarget(this);
    addFocusWidget(rom);

    ram = new PopUpWidget(boss, _font, 0, 0, ramItems, "RAM", 0, ramCmd);
    ram->setTarget(this);
    addFocusWidget(ram);
  };

  addRegion(myLowerLabel,  "Set lower 2K region ($F000 - $F7FF):",
            myROMLower,  items32,  kROMLowerChanged,
            myRAMLower,  items16,  kRAMLowerChanged);
  addRegion(myMiddleLabel, "Set middle 1.5K region ($F800 - $FDFF):",
            myROMMiddle, items32,  kROMMiddleChanged,
            myRAMMiddle, items16,  kRAMMiddleChanged);
  addRegion(myHighLabel,   "Set high 256B region ($FE00 - $FEFF):",
            myROMHigh,   items256, kROMHighChanged,
            myRAMHigh,   items128, kRAMHighChanged);

  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge4A50Widget::layoutContent(GUI::BoxLayout& col) const
{
  // The ROM selectors form a column, as do the RAM ones: each gets one label
  // column, and all six get one box width, so the two columns line up down the tab
  GUI::alignLabels({{myROMLower}, {myROMMiddle}, {myROMHigh}});
  GUI::alignLabels({{myRAMLower}, {myRAMMiddle}, {myRAMHigh}});
  GUI::alignPopUps({myROMLower, myROMMiddle, myROMHigh,
                    myRAMLower, myRAMMiddle, myRAMHigh});

  layoutRegion(col, myLowerLabel,  myROMLower,  myRAMLower);
  col.addSpace(_lineHeight);
  layoutRegion(col, myMiddleLabel, myROMMiddle, myRAMMiddle);
  col.addSpace(_lineHeight);
  layoutRegion(col, myHighLabel,   myROMHigh,   myRAMHigh);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge4A50Widget::layoutRegion(GUI::BoxLayout& col, StaticTextWidget* label,
                                       PopUpWidget* rom, PopUpWidget* ram) const
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;

  auto row = std::make_unique<BoxLayout>(BoxLayout::Dir::Horizontal, _fontWidth * 2);
  row->addSpace(_fontWidth * 4);   // the selectors sit in from their heading
  row->addAuto(anchoredItem(rom));
  row->addAuto(anchoredItem(ram));

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
void Cartridge4A50Widget::handleCommand(CommandSender* sender,
                                      int cmd, int data, int id)
{
  myCart.unlockHotspots();

  switch(cmd)
  {
    case kROMLowerChanged:
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

    case kRAMLowerChanged:
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

    case kROMMiddleChanged:
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

    case kRAMMiddleChanged:
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

    case kROMHighChanged:
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

    case kRAMHighChanged:
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
