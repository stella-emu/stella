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

#include "CartDPC.hxx"
#include "DataGridWidget.hxx"
#include "PopUpWidget.hxx"
#include "CartDPCWidget.hxx"

using Common::Base;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDPCWidget::CartridgeDPCWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      CartridgeDPC& cart)
  : CartDebugWidget(boss, lfont, nfont),
    myCart{cart}
{
  const size_t size = cart.myImage.size();

  const auto bankStart = [&](uInt32 offset) -> uInt16 {
    const uInt16 s = (cart.myImage[offset + 1] << 8) | cart.myImage[offset];
    return s - s % 0x1000;
  };
  string info =
    std::format(
      "DPC cartridge, two 4K banks + 2K display bank\n"
      "DPC registers accessible @ $F000 - $F07F\n"
      "  $F000 - F03F (R), $F040 - $F07F (W)\n"
      "Startup bank = {} or undetermined\n",
      cart.startBank());

  // Eventually, we should query this from the debugger/disassembler
  constexpr uInt32 spot = 0xFF8;
  for(uInt32 i = 0, offset = 0xFFC; i < 2; ++i, offset += 0x1000)
  {
    const uInt16 start = bankStart(offset);
    info += std::format("Bank {} @ ${} - ${} (hotspot = ${})\n",
      i, Base::hex4(start + 0x80), Base::hex4(start + 0xFFF), Base::hex4(0xF000 + spot + i));
  }

  createBaseInformation(size, "Activision (Pitfall II)", info);

  VariantList items;
  for(int bank = 0; bank < 2; ++bank)
    VarList::push_back(items, std::format("#{} (${})", bank, Base::hex4(0xFFF8 + bank)));

  myBank = new PopUpWidget(boss, _font, 0, 0, items, "Set bank", 0, kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);

  // The selector's box lines up with the info fields above it
  myLabelColumn.emplace_back(myBank);

  // The data fetcher registers, each a labelled row of a grid
  myFetcherLabel = new StaticTextWidget(boss, _font, 0, 0, "Data fetchers");

  const auto addRegisters = [&](StaticTextWidget*& label, string_view text,
                                DataGridWidget*& grid, int cols, int colchars,
                                int bits, Common::Base::Fmt fmt) {
    label = new StaticTextWidget(boss, _font, 0, 0, text);

    grid = new DataGridWidget(boss, _nfont, 0, 0, cols, 1, colchars, bits, fmt);
    grid->setTarget(this);
    grid->setEditable(false);
  };

  addRegisters(myTopsLabel,     "Top registers",     myTops,     8, 2,  8,
               Common::Base::Fmt::_16);
  addRegisters(myBottomsLabel,  "Bottom registers",  myBottoms,  8, 2,  8,
               Common::Base::Fmt::_16);
  addRegisters(myCountersLabel, "Counter registers", myCounters, 8, 4, 16,
               Common::Base::Fmt::_16_4);
  addRegisters(myFlagsLabel,    "Flag registers",    myFlags,    8, 2,  8,
               Common::Base::Fmt::_16);

  addRegisters(myMusicModeLabel, "Music mode (DF5/DF6/DF7)", myMusicMode, 3, 2, 8,
               Common::Base::Fmt::_16);
  addRegisters(myRandomLabel,    "Current random number",    myRandom,    1, 2, 8,
               Common::Base::Fmt::_16);

  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPCWidget::layoutContent(GUI::BoxLayout& col) const
{
  using GUI::anchoredItem;
  using GUI::labeledRow;

  // The four register rows are indented under the "Data fetchers" heading and
  // share a label column; the two rows below it share the tab's left edge
  const int indent = _fontWidth * 2;

  GUI::alignLabels({{myTopsLabel, indent}, {myBottomsLabel, indent},
                    {myCountersLabel, indent}, {myFlagsLabel, indent}});
  GUI::alignLabels({{myMusicModeLabel}, {myRandomLabel}});

  col.addAuto(anchoredItem(myBank));

  col.addSpace(_lineHeight);
  col.addAuto(anchoredItem(myFetcherLabel));
  col.addAuto(labeledRow(myTopsLabel,     myTops,     0, indent));
  col.addAuto(labeledRow(myBottomsLabel,  myBottoms,  0, indent));
  col.addAuto(labeledRow(myCountersLabel, myCounters, 0, indent));
  col.addAuto(labeledRow(myFlagsLabel,    myFlags,    0, indent));

  col.addSpace(_lineHeight);
  col.addAuto(labeledRow(myMusicModeLabel, myMusicMode));
  col.addAuto(labeledRow(myRandomLabel,    myRandom));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPCWidget::saveOldState()
{
  myOldState.tops.clear();
  myOldState.bottoms.clear();
  myOldState.counters.clear();
  myOldState.flags.clear();
  myOldState.music.clear();

  std::ranges::copy(myCart.myTops,      std::back_inserter(myOldState.tops));
  std::ranges::copy(myCart.myBottoms,   std::back_inserter(myOldState.bottoms));
  std::ranges::copy(myCart.myCounters,  std::back_inserter(myOldState.counters));
  std::ranges::copy(myCart.myFlags,     std::back_inserter(myOldState.flags));
  std::ranges::copy(myCart.myMusicMode, std::back_inserter(myOldState.music));

  myOldState.random = myCart.myRandomNumber;

  myOldState.internalram.assign(myCart.myDisplayImage.begin(),
                                myCart.myDisplayImage.begin() + internalRamSize());

  myOldState.bank = myCart.getBank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPCWidget::loadConfig()
{
  myBank->setSelectedIndex(myCart.getBank(), myCart.getBank() != myOldState.bank);

  // Get registers, using change tracking
  IntArray alist;
  IntArray vlist;
  BoolArray changed;

  const auto clearAll = [&]() {
    alist.clear(); vlist.clear(); changed.clear();
  };

  clearAll();
  for(int i = 0; i < 8; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myTops[i]);
    changed.push_back(std::cmp_not_equal(myCart.myTops[i],
                                         myOldState.tops[i]));
  }
  myTops->setList(alist, vlist, changed);

  clearAll();
  for(int i = 0; i < 8; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myBottoms[i]);
    changed.push_back(std::cmp_not_equal(myCart.myBottoms[i],
                                         myOldState.bottoms[i]));
  }
  myBottoms->setList(alist, vlist, changed);

  clearAll();
  for(int i = 0; i < 8; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myCounters[i]);
    changed.push_back(std::cmp_not_equal(myCart.myCounters[i],
                                         myOldState.counters[i]));
  }
  myCounters->setList(alist, vlist, changed);

  clearAll();
  for(int i = 0; i < 8; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myFlags[i]);
    changed.push_back(std::cmp_not_equal(myCart.myFlags[i],
                                         myOldState.flags[i]));
  }
  myFlags->setList(alist, vlist, changed);

  clearAll();
  for(int i = 0; i < 3; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myMusicMode[i]);
    changed.push_back(myCart.myMusicMode[i] != myOldState.music[i]);
  }
  myMusicMode->setList(alist, vlist, changed);

  myRandom->setList(0, myCart.myRandomNumber,
      myCart.myRandomNumber != myOldState.random);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPCWidget::handleCommand(CommandSender* sender,
                                       int cmd, int data, int id)
{
  if(cmd == kBankChanged)
  {
    myCart.unlockHotspots();
    myCart.bank(myBank->getSelected());
    myCart.lockHotspots();
    invalidate();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeDPCWidget::bankState()
{
  return std::format("Bank #{} (hotspot ${})",
    myCart.getBank(), Base::hex4(0xFFF8 + myCart.getBank()));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeDPCWidget::internalRamSize()
{
  return 2*1024;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeDPCWidget::internalRamRPort(int start)
{
  return 0x0000 + start;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeDPCWidget::internalRamDescription()
{
  return
    "2K display data @ $0000 - $07FF\n"
    "  indirectly accessible to 6507 via DPC's\n"
    "  data fetcher registers\n";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeDPCWidget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  for(int i = 0; i < count; i++)
    myRamOld.push_back(myOldState.internalram[start + i]);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeDPCWidget::internalRamCurrent(int start, int count)
{
  myRamCurrent.clear();
  for(int i = 0; i < count; i++)
    myRamCurrent.push_back(myCart.myDisplayImage[start + i]);
  return myRamCurrent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPCWidget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myDisplayImage[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeDPCWidget::internalRamGetValue(int addr)
{
  return myCart.myDisplayImage[addr];
}
