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

#include "CartDPCPlus.hxx"
#include "DataGridWidget.hxx"
#include "PopUpWidget.hxx"
#include "CartDPCPlusWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDPCPlusWidget::CartridgeDPCPlusWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      CartridgeDPCPlus& cart)
  : CartridgeARMWidget(boss, lfont, nfont, cart),
    myCart{cart}
{
  const size_t size = cart.mySize;

  const string info = std::format(
    "Extended DPC cartridge, six 4K banks, 4K display bank, 1K frequency table, "
    "8K DPC RAM\n"
    "DPC registers accessible @ $F000 - $F07F\n"
    "  $F000 - $F03F (R), $F040 - $F07F (W)\n"
    "Banks accessible at hotspots $FFF6 to $FFFB\n"
    "Startup bank = {}\n"
    "Ver = {}",
    cart.startBank(), cart.myDriverMD5);

  createBaseInformation(size, "Activision (Pitfall II)", info);

  VariantList items;
  VarList::push_back(items, "0 ($FFF6)");
  VarList::push_back(items, "1 ($FFF7)");
  VarList::push_back(items, "2 ($FFF8)");
  VarList::push_back(items, "3 ($FFF9)");
  VarList::push_back(items, "4 ($FFFA)");
  VarList::push_back(items, "5 ($FFFB)");

  myBankLabel = new StaticTextWidget(boss, _font, "Set bank");
  myBank = new PopUpWidget(boss, _font, items, kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);

  // The selector's box lines up with the info fields above it
  myLabelColumn.emplace_back(myBankLabel);

  // The DPC+ registers, each a labelled row of a grid
  const auto addRegisters = [&](StaticTextWidget*& label, string_view text,
                                DataGridWidget*& grid, int cols, int rows,
                                int colchars, int bits, Common::Base::Fmt fmt) {
    label = new StaticTextWidget(boss, _font, text);

    grid = new DataGridWidget(boss, _nfont, cols, rows, colchars, bits, fmt);
    grid->setTarget(this);
    grid->setEditable(false);
  };

  addRegisters(myTopsLabel,        "Top Registers",     myTops,        8, 1, 2,  8,
               Common::Base::Fmt::_16);
  addRegisters(myBottomsLabel,     "Bottom Registers",  myBottoms,     8, 1, 2,  8,
               Common::Base::Fmt::_16);
  addRegisters(myCountersLabel,    "Counter Registers", myCounters,    8, 1, 4, 16,
               Common::Base::Fmt::_16_4);
  addRegisters(myFracCountersLabel, "Frac Counters",    myFracCounters, 4, 2, 8, 32,
               Common::Base::Fmt::_16_8);
  addRegisters(myFracIncrementsLabel, "Frac Increments", myFracIncrements, 8, 1, 2, 8,
               Common::Base::Fmt::_16);
  addRegisters(myParameterLabel,   "Function Params",   myParameter,   8, 1, 2,  8,
               Common::Base::Fmt::_16);
  addRegisters(myMusicCountersLabel, "Music Counters",  myMusicCounters, 3, 1, 8, 32,
               Common::Base::Fmt::_16_8);
  addRegisters(myMusicFrequenciesLabel, "Music Frequencies", myMusicFrequencies, 3, 1, 8, 32,
               Common::Base::Fmt::_16_8);
  addRegisters(myMusicWaveformsLabel, "Music Waveforms", myMusicWaveforms, 3, 1, 4, 16,
               Common::Base::Fmt::_16_4);
  addRegisters(myRandomLabel,      "Current random number", myRandom,  1, 1, 8, 32,
               Common::Base::Fmt::_16_8);

  // Fast fetch and immediate mode LDA flags
  myFastFetch = new CheckboxWidget(boss, _font, "Fast Fetcher enabled");
  myFastFetch->setTarget(this);
  myFastFetch->setEditable(false);

  myIMLDA = new CheckboxWidget(boss, _font, "Immediate mode LDA");
  myIMLDA->setTarget(this);
  myIMLDA->setEditable(false);

  createCycleWidgets();

  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPCPlusWidget::layoutContent(GUI::BoxLayout& col) const
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::labeledRow;
  using Dir = BoxLayout::Dir;

  // Every register row shares one label column
  GUI::alignLabels({{myTopsLabel}, {myBottomsLabel}, {myCountersLabel},
                    {myFracCountersLabel}, {myFracIncrementsLabel},
                    {myParameterLabel}, {myMusicCountersLabel},
                    {myMusicFrequenciesLabel}, {myMusicWaveformsLabel},
                    {myRandomLabel}});

  col.addAuto(labeledRow(myBankLabel, myBank));

  col.addSpace(_lineHeight / 2);
  col.addAuto(labeledRow(myTopsLabel,           myTops));
  col.addAuto(labeledRow(myBottomsLabel,        myBottoms));
  col.addAuto(labeledRow(myCountersLabel,       myCounters));
  col.addAuto(labeledRow(myFracCountersLabel,   myFracCounters));
  col.addAuto(labeledRow(myFracIncrementsLabel, myFracIncrements));
  col.addAuto(labeledRow(myParameterLabel,      myParameter));
  col.addAuto(labeledRow(myMusicCountersLabel,  myMusicCounters));
  col.addAuto(labeledRow(myMusicFrequenciesLabel, myMusicFrequencies));
  col.addAuto(labeledRow(myMusicWaveformsLabel, myMusicWaveforms));

  // The random number, with the two fetcher flags stacked beside it
  auto flags = std::make_unique<BoxLayout>(Dir::Vertical, VGAP);
  flags->addAuto(anchoredItem(myFastFetch));
  flags->addAuto(anchoredItem(myIMLDA));

  auto row = std::make_unique<BoxLayout>(Dir::Horizontal, _fontWidth * 3);
  row->addAuto(labeledRow(myRandomLabel, myRandom));
  row->addAuto(std::move(flags));

  col.addAuto(std::move(row));

  // ...and the ARM cycle counters below everything
  CartridgeARMWidget::layoutContent(col);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPCPlusWidget::saveOldState()
{
  myOldState.tops.clear();
  myOldState.bottoms.clear();
  myOldState.counters.clear();
  myOldState.fraccounters.clear();
  myOldState.fracinc.clear();
  myOldState.param.clear();
  myOldState.mcounters.clear();
  myOldState.mfreqs.clear();
  myOldState.mwaves.clear();
  myOldState.internalram.clear();

  std::ranges::copy(myCart.myTops,
                    std::back_inserter(myOldState.tops));
  std::ranges::copy(myCart.myBottoms,
                    std::back_inserter(myOldState.bottoms));
  std::ranges::copy(myCart.myCounters,
                    std::back_inserter(myOldState.counters));
  std::ranges::copy(myCart.myFractionalCounters,
                    std::back_inserter(myOldState.fraccounters));
  std::ranges::copy(myCart.myFractionalIncrements,
                    std::back_inserter(myOldState.fracinc));
  std::ranges::copy(myCart.myParameter,
                    std::back_inserter(myOldState.param));
  std::ranges::copy(myCart.myMusicCounters,
                    std::back_inserter(myOldState.mcounters));
  std::ranges::copy(myCart.myMusicFrequencies,
                    std::back_inserter(myOldState.mfreqs));
  std::ranges::copy(myCart.myMusicWaveforms,
                    std::back_inserter(myOldState.mwaves));

  myOldState.random = myCart.myRandomNumber;

  myOldState.internalram.assign(myCart.myDisplayImage.begin(),
                                myCart.myDisplayImage.begin() + internalRamSize());

  myOldState.bank = myCart.getBank();

  CartridgeARMWidget::saveOldState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPCPlusWidget::loadConfig()
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
    changed.push_back(myCart.myTops[i] != myOldState.tops[i]);
  }
  myTops->setList(alist, vlist, changed);

  clearAll();
  for(int i = 0; i < 8; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myBottoms[i]);
    changed.push_back(myCart.myBottoms[i] != myOldState.bottoms[i]);
  }
  myBottoms->setList(alist, vlist, changed);

  clearAll();
  for(int i = 0; i < 8; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myCounters[i]);
    changed.push_back(std::cmp_not_equal(myCart.myCounters[i], myOldState.counters[i]));
  }
  myCounters->setList(alist, vlist, changed);

  clearAll();
  for(int i = 0; i < 8; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myFractionalCounters[i]);
    changed.push_back(std::cmp_not_equal(myCart.myFractionalCounters[i],
                                         myOldState.fraccounters[i]));
  }
  myFracCounters->setList(alist, vlist, changed);

  clearAll();
  for(int i = 0; i < 8; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myFractionalIncrements[i]);
    changed.push_back(myCart.myFractionalIncrements[i] != myOldState.fracinc[i]);
  }
  myFracIncrements->setList(alist, vlist, changed);

  clearAll();
  for(int i = 0; i < 8; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myParameter[i]);
    changed.push_back(myCart.myParameter[i] != myOldState.param[i]);
  }
  myParameter->setList(alist, vlist, changed);

  clearAll();
  for(int i = 0; i < 3; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myMusicCounters[i]);
    changed.push_back(std::cmp_not_equal(myCart.myMusicCounters[i],
                                         myOldState.mcounters[i]));
  }
  myMusicCounters->setList(alist, vlist, changed);

  clearAll();
  for(int i = 0; i < 3; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myMusicFrequencies[i]);
    changed.push_back(std::cmp_not_equal(myCart.myMusicFrequencies[i],
                                         myOldState.mfreqs[i]));
  }
  myMusicFrequencies->setList(alist, vlist, changed);

  clearAll();
  for(int i = 0; i < 3; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myMusicWaveforms[i]);
    changed.push_back(std::cmp_not_equal(myCart.myMusicWaveforms[i],
                                         myOldState.mwaves[i]));
  }
  myMusicWaveforms->setList(alist, vlist, changed);

  myRandom->setList(0, myCart.myRandomNumber,
      myCart.myRandomNumber != myOldState.random);

  myFastFetch->setState(myCart.myFastFetch);
  myIMLDA->setState(myCart.myLDAimmediate);

  CartridgeARMWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPCPlusWidget::handleCommand(CommandSender* sender,
                                           int cmd, int data, int id)
{
  if(cmd == kBankChanged)
  {
    myCart.unlockHotspots();
    myCart.bank(myBank->getSelected());
    myCart.lockHotspots();
    invalidate();
  }
  else
    CartridgeARMWidget::handleCommand(sender, cmd, data, id);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeDPCPlusWidget::bankState()
{
  static constexpr std::array<string_view, 6> spot = {
    "$FFF6", "$FFF7", "$FFF8", "$FFF9", "$FFFA", "$FFFB"
  };
  return std::format("Bank = {}, hotspot = {}",
    myCart.getBank(), spot[myCart.getBank()]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeDPCPlusWidget::internalRamSize()
{
  return 5*1024;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeDPCPlusWidget::internalRamRPort(int start)
{
  return 0x0000 + start;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeDPCPlusWidget::internalRamDescription()
{
  return
    "$0000 - $0FFF - 4K display data\n"
    "                indirectly accessible to 6507\n"
    "                via DPC+'s Data Fetcher registers\n"
    "$1000 - $13FF - 1K frequency table,\n"
    "                C variables and C stack\n"
    "                not accessible to 6507";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeDPCPlusWidget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  myRamOld.assign(myOldState.internalram.data() + start,
                  myOldState.internalram.data() + start + count);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeDPCPlusWidget::internalRamCurrent(int start, int count)
{
  myRamCurrent.clear();
  myRamCurrent.assign(myCart.myDisplayImage.begin() + start,
                      myCart.myDisplayImage.begin() + start + count);
  return myRamCurrent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPCPlusWidget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myDisplayImage[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeDPCPlusWidget::internalRamGetValue(int addr)
{
  return myCart.myDisplayImage[addr];
}
