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

#include "CartBUS.hxx"
#include "DataGridWidget.hxx"
#include "PopUpWidget.hxx"
#include "Layout.hxx"
#include "CartBUSWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeBUSWidget::CartridgeBUSWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      CartridgeBUS& cart)
  : CartridgeARMWidget(boss, lfont, nfont, cart),
    myCart{cart}
{
  const bool isBUS3 = cart.myBUSSubtype == CartridgeBUS::BUSSubtype::BUS3;

  if(isBUS3)
  {
    myDatastream2Rows = 2;
    myDatastreamCount = 18;
  }
  else
  {
    myDatastream2Rows = 4;
    myDatastreamCount = 20;
  }

  VariantList items;
  if(cart.myBUSSubtype == CartridgeBUS::BUSSubtype::BUS0)
  {
    VarList::push_back(items, "0 ($FFF6)");
    VarList::push_back(items, "1 ($FFF7)");
    VarList::push_back(items, "2 ($FFF8)");
    VarList::push_back(items, "3 ($FFF9)");
    VarList::push_back(items, "4 ($FFFA)");
    VarList::push_back(items, "5 ($FFFB)");
  }
  else
  {
    VarList::push_back(items, "0 ($FFF5)");
    VarList::push_back(items, "1 ($FFF6)");
    VarList::push_back(items, "2 ($FFF7)");
    VarList::push_back(items, "3 ($FFF8)");
    VarList::push_back(items, "4 ($FFF9)");
    VarList::push_back(items, "5 ($FFFA)");
    VarList::push_back(items, "6 ($FFFB)");
  }
  // Every widget is created at a placeholder position; reflow() positions them
  myBank = new PopUpWidget(boss, _font, items, "Set bank", 0, kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
  myLabelColumn.emplace_back(myBank);

  const auto addGrid = [&](DataGridWidget*& grid, int cols, int rows,
                           int colchars, int bits, Common::Base::Fmt fmt) {
    grid = new DataGridWidget(boss, _nfont, cols, rows, colchars, bits, fmt);
    grid->setTarget(this);
    grid->setEditable(false);
  };

  // The datastream table: pointers on the left, increments on the right, with the
  // named streams on rows of their own beneath them
  myPointersLabel = new StaticTextWidget(boss, _font, "Datastream Pointers");
  addGrid(myDatastreamPointers,   4, 4, 6, 32, Common::Base::Fmt::_16_3_2);
  addGrid(myDatastreamPointers2,  1, myDatastream2Rows, 6, 32, Common::Base::Fmt::_16_3_2);

  myIncrementsLabel = new StaticTextWidget(boss, _font, "Datastream Increments");
  addGrid(myDatastreamIncrements,  4, 4, 5, 32, Common::Base::Fmt::_16_2_2);
  addGrid(myDatastreamIncrements2, 1, myDatastream2Rows, 5, 32, Common::Base::Fmt::_16_2_2);

  // The stream each table row holds: the first four by number, then the named ones
  for(uInt32 row = 0; row < 4; ++row)
    myDatastreamLabels[row] =
      new StaticTextWidget(_boss, _font,
                           Common::Base::toString(row * 4, Common::Base::Fmt::_16_2));

  static constexpr std::array<string_view, 4> named{
    "Write Data 0 (stream 16)", "Write Data 1 (stream 17)",
    "Write Data 2 (stream 18)", "Write Data 3 (stream 19)"
  };
  if(isBUS3)
  {
    myDatastreamLabels[4] = new StaticTextWidget(_boss, _font, "Write Data (stream 16)");
    myDatastreamLabels[5] = new StaticTextWidget(_boss, _font, "Jump Data (stream 17)");
  }
  else
    for(uInt32 row = 0; row < 4; ++row)
      myDatastreamLabels[4 + row] =
        new StaticTextWidget(_boss, _font, named[row]);

  // Address maps
  myAddressMapsLabel = new StaticTextWidget(boss, _font, "Address Maps");
  addGrid(myAddressMaps, 8, 5, 8, 32, Common::Base::Fmt::_16_8);

  // Music states
  myCountersLabel = new StaticTextWidget(boss, _font, "Music Counters");
  addGrid(myMusicCounters,      3, 1, 8, 32, Common::Base::Fmt::_16_8);

  myFrequenciesLabel = new StaticTextWidget(boss, _font, "Music Frequencies");
  addGrid(myMusicFrequencies,   3, 1, 8, 32, Common::Base::Fmt::_16_8);

  myWaveformsLabel = new StaticTextWidget(boss, _font, "Music Waveforms");
  addGrid(myMusicWaveforms,     3, 1, 4, 16, Common::Base::Fmt::_16_2);

  myWaveformSizesLabel = new StaticTextWidget(boss, _font, "Music Waveform Sizes");
  addGrid(myMusicWaveformSizes, 3, 1, 4, 16, Common::Base::Fmt::_16_2);

  // BUS stuff and Digital Audio flags; only BUS3 plays digital samples
  myBusOverdrive = new CheckboxWidget(boss, _font, "BUS Overdrive enabled");
  myBusOverdrive->setTarget(this);
  myBusOverdrive->setEditable(false);

  if(isBUS3)
  {
    mySamplePointerLabel = new StaticTextWidget(boss, _font, "Sample Pointer");
    addGrid(mySamplePointer, 1, 1, 8, 32, Common::Base::Fmt::_16_8);

    myDigitalSample = new CheckboxWidget(boss, _font, "Digital Sample mode");
    myDigitalSample->setTarget(this);
    myDigitalSample->setEditable(false);
  }

  createCycleWidgets();

  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<GUI::Layout> CartridgeBUSWidget::layoutDatastreams() const
{
  using GUI::BoxLayout;
  using GUI::GridLayout;
  using GUI::anchoredItem;
  using GUI::alignedItem;
  using GUI::HAlign;
  using GUI::VAlign;

  // The stream labels down the left of the pointer grid: one per GRID row, which
  // no cell can say -- so the column is built to the grid's own row pitch, and
  // starts where the grid insets its first row's text (as RamWidget's do)
  const auto streamLabels = [&](int first, int count, const DataGridWidget* grid) {
    auto labels = std::make_unique<BoxLayout>(BoxLayout::Dir::Vertical);
    labels->addSpace(grid->firstTextY() - myDatastreamLabels[first]->firstTextY());
    for(int row = 0; row < count; ++row)
      labels->addFixed(anchoredItem(myDatastreamLabels[first + row]), _lineHeight);
    return labels;
  };

  // A grid: labels | pointers | increments.  The rows keep the two tables in step,
  // and the named streams' cells end flush with the column above them (they hold
  // the last of its four), which is what the old 3/4-of-the-width offset was for.
  // Their labels LIE ACROSS the label and pointer columns rather than sitting in
  // the first, which would widen it to their length and push the table off-window
  auto table = std::make_unique<GridLayout>(3, 3, _fontWidth, VGAP);
  for(int c = 0; c < 3; ++c)
    table->columnAuto(c);
  for(int r = 0; r < 3; ++r)
    table->rowAuto(r);

  table->place(1, 0, anchoredItem(myPointersLabel));
  table->place(2, 0, anchoredItem(myIncrementsLabel));

  table->place(0, 1, streamLabels(0, 4, myDatastreamPointers));
  table->place(1, 1, alignedItem(myDatastreamPointers, HAlign::Left, VAlign::Top));
  table->place(2, 1, alignedItem(myDatastreamIncrements, HAlign::Left, VAlign::Top));

  table->place(0, 2, streamLabels(4, myDatastream2Rows, myDatastreamPointers2), 2);
  table->place(1, 2, alignedItem(myDatastreamPointers2, HAlign::Right, VAlign::Top));
  table->place(2, 2, alignedItem(myDatastreamIncrements2, HAlign::Left, VAlign::Top));

  return table;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUSWidget::layoutContent(GUI::BoxLayout& col) const
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::labeledRow;
  using Dir = BoxLayout::Dir;

  // The music rows share a label column
  GUI::alignLabels({{myCountersLabel}, {myFrequenciesLabel},
                    {myWaveformsLabel}, {myWaveformSizesLabel}});

  col.addAuto(anchoredItem(myBank));

  col.addSpace(_lineHeight / 2);
  col.addAuto(layoutDatastreams());

  col.addSpace(_lineHeight / 2);
  col.addAuto(anchoredItem(myAddressMapsLabel));
  col.addAuto(anchoredItem(myAddressMaps));

  col.addSpace(_lineHeight / 2);
  col.addAuto(labeledRow(myCountersLabel,      myMusicCounters));
  col.addAuto(labeledRow(myFrequenciesLabel,   myMusicFrequencies));

  // Only BUS3 has a sample pointer, and it sits beside the waveforms
  auto waveforms = std::make_unique<BoxLayout>(Dir::Horizontal, _fontWidth * 3);
  waveforms->addAuto(labeledRow(myWaveformsLabel, myMusicWaveforms));
  if(mySamplePointer != nullptr)
  {
    GUI::alignLabels({{mySamplePointerLabel}});
    waveforms->addAuto(labeledRow(mySamplePointerLabel, mySamplePointer));
  }
  col.addAuto(std::move(waveforms));

  col.addAuto(labeledRow(myWaveformSizesLabel, myMusicWaveformSizes));

  // ...the two flags, then the ARM cycle counters below everything
  auto flags = std::make_unique<BoxLayout>(Dir::Horizontal, _fontWidth * 3);
  flags->addAuto(anchoredItem(myBusOverdrive));
  if(myDigitalSample != nullptr)
    flags->addAuto(anchoredItem(myDigitalSample));

  col.addSpace(_lineHeight / 2);
  col.addAuto(std::move(flags));

  CartridgeARMWidget::layoutContent(col);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUSWidget::saveOldState()
{
  myOldState.tops.clear();
  myOldState.bottoms.clear();
  myOldState.datastreampointers.clear();
  myOldState.datastreamincrements.clear();
  myOldState.addressmaps.clear();
  myOldState.mcounters.clear();
  myOldState.mfreqs.clear();
  myOldState.mwaves.clear();
  myOldState.mwavesizes.clear();
  myOldState.internalram.clear();
  myOldState.samplepointer.clear();

  for(int i = 0; i < myDatastreamCount; ++i)
  {
    // Pointers are stored as:
    // PPPFF---
    //
    // Increments are stored as
    // ----IIFF
    //
    // P = Pointer
    // I = Increment
    // F = Fractional

    myOldState.datastreampointers.push_back(myCart.getDatastreamPointer(i)>>12);
    if(i < 16)
      myOldState.datastreamincrements.push_back(myCart.getDatastreamIncrement(i));
    else
      myOldState.datastreamincrements.push_back(0x100);
  }

  for(uInt32 i = 0; i < 37; ++i) // only 37 map values
    myOldState.addressmaps.push_back(myCart.getAddressMap(i));

  for(uInt32 i = 37; i < 40; ++i) // but need 40 for the grid
    myOldState.addressmaps.push_back(0);

  for(uInt32 i = 0; i < 3; ++i)
    myOldState.mcounters.push_back(myCart.myMusicCounters[i]);

  for(uInt32 i = 0; i < 3; ++i)
  {
    myOldState.mfreqs.push_back(myCart.myMusicFrequencies[i]);
    myOldState.mwaves.push_back(myCart.getWaveform(i) >> 5);
    myOldState.mwavesizes.push_back(myCart.getWaveformSize(i));
  }

  myOldState.internalram.assign(myCart.myRAM.data(),
                                myCart.myRAM.data() + internalRamSize());

  myOldState.samplepointer.push_back(myCart.getSample());

  CartridgeARMWidget::saveOldState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUSWidget::loadConfig()
{
  myBank->setSelectedIndex(myCart.getBank());

  // Get registers, using change tracking
  IntArray alist;
  IntArray vlist;
  BoolArray changed;

  const auto clearAll = [&]() {
    alist.clear(); vlist.clear(); changed.clear();
  };

  clearAll();
  for(int i = 0; i < 16; ++i)
  {
    // Pointers are stored as:
    // PPPFF---
    //
    // Increments are stored as
    // ----IIFF
    //
    // P = Pointer
    // I = Increment
    // F = Fractional

    const Int32 pointervalue = myCart.getDatastreamPointer(i) >> 12;
    alist.push_back(0);  vlist.push_back(pointervalue);
    changed.push_back(pointervalue != myOldState.datastreampointers[i]);
  }
  myDatastreamPointers->setList(alist, vlist, changed);

  clearAll();
  for(int i = 16; i < myDatastreamCount; ++i)
  {
    const Int32 pointervalue = myCart.getDatastreamPointer(i) >> 12;
    alist.push_back(0);  vlist.push_back(pointervalue);
    changed.push_back(pointervalue != myOldState.datastreampointers[i]);
  }
  myDatastreamPointers2->setList(alist, vlist, changed);

  clearAll();
  for(int i = 0; i < 16; ++i)
  {
    const Int32 incrementvalue = myCart.getDatastreamIncrement(i);
    alist.push_back(0);  vlist.push_back(incrementvalue);
    changed.push_back(incrementvalue != myOldState.datastreamincrements[i]);
  }
  myDatastreamIncrements->setList(alist, vlist, changed);

  clearAll();
  for(int i = 16; i < myDatastreamCount; ++i)
  {
    constexpr Int32 incrementvalue = 0x100;
    alist.push_back(0);  vlist.push_back(incrementvalue);
    changed.push_back(incrementvalue != myOldState.datastreamincrements[i]);
  }
  myDatastreamIncrements2->setList(alist, vlist, changed);

  clearAll();
  for(int i = 0; i < 37; ++i) // only 37 map values
  {
    const Int32 mapvalue = myCart.getAddressMap(i);
    alist.push_back(0);  vlist.push_back(mapvalue);
    changed.push_back(mapvalue != myOldState.addressmaps[i]);
  }
  for(int i = 37; i < 40; ++i) // but need 40 for the grid
  {
    constexpr Int32 mapvalue = 0;
    alist.push_back(0);  vlist.push_back(mapvalue);
    changed.push_back(mapvalue != myOldState.addressmaps[i]);
  }
  myAddressMaps->setList(alist, vlist, changed);

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
    alist.push_back(0);  vlist.push_back(myCart.getWaveform(i) >> 5);
    changed.push_back(std::cmp_not_equal(myCart.getWaveform(i) >> 5,
                                         myOldState.mwaves[i]));
  }
  myMusicWaveforms->setList(alist, vlist, changed);

  clearAll();
  for(int i = 0; i < 3; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.getWaveformSize(i));
    changed.push_back(std::cmp_not_equal(myCart.getWaveformSize(i),
                                         myOldState.mwavesizes[i]));
  }
  myMusicWaveformSizes->setList(alist, vlist, changed);

  if(myCart.myBUSSubtype == CartridgeBUS::BUSSubtype::BUS3)
  {
    clearAll();
    alist.push_back(0);  vlist.push_back(myCart.getSample());
    changed.push_back(std::cmp_not_equal(myCart.getSample(),
                                         myOldState.samplepointer[0]));
    mySamplePointer->setList(alist, vlist, changed);
  }

  myBusOverdrive->setState((myCart.myMode & 0x0f) == 0);
  if(myCart.myBUSSubtype == CartridgeBUS::BUSSubtype::BUS3)
    myDigitalSample->setState((myCart.myMode & 0xf0) == 0);

  if((myCart.myMode & 0xf0) == 0)
  {
    myMusicWaveforms->setCrossed(true);
    myMusicWaveformSizes->setCrossed(true);
    if(myCart.myBUSSubtype == CartridgeBUS::BUSSubtype::BUS3)
      mySamplePointer->setCrossed(false);
  }
  else
  {
    myMusicWaveforms->setCrossed(false);
    myMusicWaveformSizes->setCrossed(false);
    if(myCart.myBUSSubtype == CartridgeBUS::BUSSubtype::BUS3)
      mySamplePointer->setCrossed(true);
  }

  CartridgeARMWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUSWidget::handleCommand(CommandSender* sender,
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
string CartridgeBUSWidget::bankState()
{
  const uInt16 bank = myCart.getBank();
  const bool isBUS0 = myCart.myBUSSubtype == CartridgeBUS::BUSSubtype::BUS0;
  static constexpr std::array<string_view, 7> allSpots = {
    "$FFF5", "$FFF6", "$FFF7", "$FFF8", "$FFF9", "$FFFA", "$FFFB"
  };
  // BUS0 starts at index 1 ($FFF6), others start at index 0 ($FFF5)
  return std::format("Bank = {}, hotspot = {}",
    bank, allSpots[bank + (isBUS0 ? 1 : 0)]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeBUSWidget::internalRamSize()
{
  return 8*1024;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeBUSWidget::internalRamRPort(int start)
{
  return 0x0000 + start;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeBUSWidget::internalRamDescription()
{
  return
    "$0000 - $07FF - BUS driver\n"
    "                not accessible to 6507\n"
    "$0800 - $17FF - 4K Data Stream storage\n"
    "                indirectly accessible to 6507\n"
    "                via BUS's Data Stream registers\n"
    "$1800 - $1FFF - 2K C variable storage and stack\n"
    "                not accessible to 6507";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeBUSWidget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  for(int i = 0; i < count; i++)
    myRamOld.push_back(myOldState.internalram[start + i]);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeBUSWidget::internalRamCurrent(int start, int count)
{
  myRamCurrent.clear();
  for(int i = 0; i < count; i++)
    myRamCurrent.push_back(myCart.myRAM[start + i]);
  return myRamCurrent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUSWidget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myRAM[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeBUSWidget::internalRamGetValue(int addr)
{
  return myCart.myRAM[addr];
}
