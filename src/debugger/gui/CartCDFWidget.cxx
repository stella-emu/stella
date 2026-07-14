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

#include "DataGridWidget.hxx"
#include "PopUpWidget.hxx"
#include "Layout.hxx"
#include "CartCDFWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCDFWidget::CartridgeCDFWidget(
    GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
    int x, int y, int w, int h, CartridgeCDF& cart)
  : CartridgeARMWidget(boss, lfont, nfont, x, y, w, h, cart),
    myCart{cart}
{
  VariantList items;
  if(isCDFJplus())
  {
    VarList::push_back(items, "0 ($FFF4)");
    VarList::push_back(items, "1 ($FFF5)");
    VarList::push_back(items, "2 ($FFF6)");
    VarList::push_back(items, "3 ($FFF7)");
    VarList::push_back(items, "4 ($FFF8)");
    VarList::push_back(items, "5 ($FFF9)");
    VarList::push_back(items, "6 ($FFFA)");
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
  myBank = new PopUpWidget(boss, _font, 0, 0, items, "Set bank", 0, kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
  myLabelColumn.emplace_back(myBank);

  // Fast Fetch flag, and (CDFJ+) the offset it fetches from
  myFastFetch = new CheckboxWidget(boss, _font, 0, 0, "Fast Fetcher enabled");
  myFastFetch->setTarget(this);
  myFastFetch->setEditable(false);

  if(isCDFJplus())
  {
    myFastFetchOffsetLabel = new StaticTextWidget(_boss, _font, 0, 0, "Fast Fetch Offset:");

    myFastFetcherOffset = new DataGridWidget(boss, _nfont, 0, 0, 1, 1, 2, 8,
                                             Common::Base::Fmt::_16_2);
    myFastFetcherOffset->setTarget(this);
    myFastFetcherOffset->setEditable(false);
  }

  const auto addGrid = [&](DataGridWidget*& grid, int cols, int rows,
                           int colchars, int bits, Common::Base::Fmt fmt) {
    grid = new DataGridWidget(boss, _nfont, 0, 0, cols, rows, colchars, bits, fmt);
    grid->setTarget(this);
    grid->setEditable(false);
  };
  const bool jump2 = isCDFJ() || isCDFJplus();   // two jump streams, not one

  // The datastream table: pointers on the left, increments on the right, with the
  // command and jump streams on rows of their own beneath them
  myPointersLabel = new StaticTextWidget(boss, _font, 0, 0, "Datastream Pointers");
  addGrid(myDatastreamPointers,     4, 8, 6, 32, Common::Base::Fmt::_16_3_2);
  addGrid(myCommandStreamPointer,   1, 1, 6, 32, Common::Base::Fmt::_16_3_2);
  addGrid(myJumpStreamPointers, jump2 ? 2 : 1, 1, 6, 32, Common::Base::Fmt::_16_3_2);

  myIncrementsLabel = new StaticTextWidget(boss, _font, 0, 0, "Datastream Increments");
  addGrid(myDatastreamIncrements,   4, 8, 5, 32, Common::Base::Fmt::_16_2_2);
  addGrid(myCommandStreamIncrement, 1, 1, 5, 32, Common::Base::Fmt::_16_2_2);
  addGrid(myJumpStreamIncrements, jump2 ? 2 : 1, 1, 5, 32, Common::Base::Fmt::_16_2_2);

  // The stream each table row holds: the first eight by number, then the named ones
  for(uInt32 row = 0; row < 8; ++row)
    myDatastreamLabels[row] =
      new StaticTextWidget(_boss, _font, 0, 0,
                           Common::Base::toString(row * 4, Common::Base::Fmt::_16_2));

  myDatastreamLabels[8] = new StaticTextWidget(_boss, _font, 0, 0, "Write Data (20)");
  myDatastreamLabels[9] = new StaticTextWidget(_boss, _font, 0, 0,
                            jump2 ? "Jump Data (21|22)" : "Jump Data (21)");

  // Music states
  myMusicLabel = new StaticTextWidget(_boss, _font, 0, 0, "Music States:");

  myCountersLabel = new StaticTextWidget(boss, _font, 0, 0, "Counters");
  addGrid(myMusicCounters,      3, 1, 8, 32, Common::Base::Fmt::_16_8);

  myFrequenciesLabel = new StaticTextWidget(boss, _font, 0, 0, "Frequencies");
  addGrid(myMusicFrequencies,   3, 1, 8, 32, Common::Base::Fmt::_16_8);

  myWaveformsLabel = new StaticTextWidget(boss, _font, 0, 0, "Waveforms");
  addGrid(myMusicWaveforms,     3, 1, 8, 16, Common::Base::Fmt::_16_2);

  myWaveformSizesLabel = new StaticTextWidget(boss, _font, 0, 0, "Waveform Sizes");
  addGrid(myMusicWaveformSizes, 3, 1, 8, 16, Common::Base::Fmt::_16_2);

  // Digital Audio flag, and the sample it plays from
  myDigitalSample = new CheckboxWidget(boss, _font, 0, 0, "Digital Sample mode");
  myDigitalSample->setTarget(this);
  myDigitalSample->setEditable(false);

  mySamplePointerLabel = new StaticTextWidget(boss, _font, 0, 0, "Sample Pointer");
  addGrid(mySamplePointer,      1, 1, 8, 32, Common::Base::Fmt::_16_8);

  createCycleWidgets();

  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<GUI::Layout> CartridgeCDFWidget::layoutDatastreams()
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
  auto streams = std::make_unique<BoxLayout>(BoxLayout::Dir::Vertical);
  streams->addSpace(myDatastreamPointers->firstTextY()
                    - myDatastreamLabels[0]->firstTextY());
  for(uInt32 row = 0; row < 8; ++row)
    streams->addFixed(anchoredItem(myDatastreamLabels[row]), _lineHeight);

  // A grid: labels | pointers | increments.  The rows keep the two tables in step,
  // and the named streams' cells end flush with the column above them (they hold
  // the last of its four), which is what the old 3/4-of-the-width offset was for.
  // Their labels LIE ACROSS the label and pointer columns rather than sitting in
  // the first, which would widen it to their length and push the table off-window
  auto table = std::make_unique<GridLayout>(3, 4, _fontWidth, VGAP);
  table->columnAuto(0);
  table->columnAuto(1);
  table->columnAuto(2);
  for(int row = 0; row < 4; ++row)
    table->rowAuto(row);

  table->place(1, 0, anchoredItem(myPointersLabel));
  table->place(2, 0, anchoredItem(myIncrementsLabel));

  table->place(0, 1, std::move(streams));
  table->place(1, 1, alignedItem(myDatastreamPointers, HAlign::Left, VAlign::Top));
  table->place(2, 1, alignedItem(myDatastreamIncrements, HAlign::Left, VAlign::Top));

  table->place(0, 2, anchoredItem(myDatastreamLabels[8]), 2);
  table->place(1, 2, alignedItem(myCommandStreamPointer, HAlign::Right, VAlign::Center));
  table->place(2, 2, anchoredItem(myCommandStreamIncrement));

  table->place(0, 3, anchoredItem(myDatastreamLabels[9]), 2);
  table->place(1, 3, alignedItem(myJumpStreamPointers, HAlign::Right, VAlign::Center));
  table->place(2, 3, anchoredItem(myJumpStreamIncrements));

  return table;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDFWidget::layoutContent(GUI::BoxLayout& col)
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::indentedItem;
  using GUI::labeledRow;
  using Dir = BoxLayout::Dir;

  const int indent = _fontWidth * 2;

  // The music rows share a label column, as do the sample rows below them
  GUI::alignLabels({{myCountersLabel, indent}, {myFrequenciesLabel, indent},
                    {myWaveformsLabel, indent}, {myWaveformSizesLabel, indent}});
  GUI::alignLabels({{mySamplePointerLabel, indent}});

  // The bank selector, with the fast fetcher beside it
  auto top = std::make_unique<BoxLayout>(Dir::Horizontal, _fontWidth * 3);
  top->addAuto(anchoredItem(myBank));
  top->addAuto(anchoredItem(myFastFetch));
  col.addAuto(std::move(top));

  if(myFastFetcherOffset != nullptr)
    col.addAuto(labeledRow(myFastFetchOffsetLabel, myFastFetcherOffset, 0, indent));

  col.addSpace(_lineHeight / 2);
  col.addAuto(layoutDatastreams());

  col.addSpace(_lineHeight / 2);
  col.addAuto(anchoredItem(myMusicLabel));
  col.addAuto(labeledRow(myCountersLabel,      myMusicCounters,      0, indent));
  col.addAuto(labeledRow(myFrequenciesLabel,   myMusicFrequencies,   0, indent));
  col.addAuto(labeledRow(myWaveformsLabel,     myMusicWaveforms,     0, indent));
  col.addAuto(labeledRow(myWaveformSizesLabel, myMusicWaveformSizes, 0, indent));

  col.addSpace(_lineHeight / 2);
  col.addAuto(anchoredItem(myDigitalSample));
  col.addAuto(labeledRow(mySamplePointerLabel, mySamplePointer, 0, indent));

  // ...and the ARM cycle counters below everything
  CartridgeARMWidget::layoutContent(col);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDFWidget::saveOldState()
{
  myOldState.fastfetchoffset.clear();
  myOldState.datastreampointers.clear();
  myOldState.datastreamincrements.clear();
  myOldState.addressmaps.clear();
  myOldState.mcounters.clear();
  myOldState.mfreqs.clear();
  myOldState.mwaves.clear();
  myOldState.mwavesizes.clear();
  myOldState.internalram.clear();
  myOldState.samplepointer.clear();

  if(isCDFJplus())
    myOldState.fastfetchoffset.push_back(myCart.myRAM[myCart.myFastFetcherOffset]);

  for(uInt32 i = 0; i < (isCDFJ() || isCDFJplus() ? 35U : 34U); ++i)
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

    myOldState.datastreampointers.push_back(
      myCart.getDatastreamPointer(i) >> (isCDFJplus() ? 8 : 12));
    myOldState.datastreamincrements.push_back(myCart.getDatastreamIncrement(i));
  }

  std::ranges::copy(myCart.myMusicCounters,
    std::back_inserter(myOldState.mcounters));

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
void CartridgeCDFWidget::loadConfig()
{
  const Int32 ds_shift = isCDFJplus() ? 8 : 12;
  myBank->setSelectedIndex(myCart.getBank());

  // Get registers, using change tracking
  IntArray alist;
  IntArray vlist;
  BoolArray changed;

  const auto clearAll = [&]() {
    alist.clear(); vlist.clear(); changed.clear();
  };

  if(isCDFJplus())
  {
    clearAll();
    alist.push_back(0);  vlist.push_back(myCart.myRAM[myCart.myFastFetcherOffset]);
    changed.push_back((myCart.myRAM[myCart.myFastFetcherOffset]) !=
      static_cast<uInt32>(myOldState.fastfetchoffset[0]));
    myFastFetcherOffset->setList(alist, vlist, changed);
  }

  clearAll();
  for(int i = 0; i < 32; ++i)
  {
    // Pointers are stored as:
    // PPPFF---     CDFJ
    // PPPPFF--     CDFJ+
    //
    // Increments are stored as
    // ----IIFF
    //
    // P = Pointer
    // I = Increment
    // F = Fractional

    const Int32 pointervalue = myCart.getDatastreamPointer(i) >> ds_shift;
    alist.push_back(0);  vlist.push_back(pointervalue);
    changed.push_back(pointervalue != myOldState.datastreampointers[i]);
  }
  myDatastreamPointers->setList(alist, vlist, changed);

  clearAll();
  for(int i = 32; i < 34; ++i)
  {
    const Int32 pointervalue = myCart.getDatastreamPointer(i) >> ds_shift;
    alist.push_back(0);  vlist.push_back(pointervalue);
    changed.push_back(pointervalue != myOldState.datastreampointers[i]);
  }

  clearAll();
  alist.push_back(0);
  vlist.push_back(myCart.getDatastreamPointer(0x20) >> ds_shift);
  changed.push_back(std::cmp_not_equal(myCart.getDatastreamPointer(0x20), myOldState.datastreampointers[0x20]));
  myCommandStreamPointer->setList(alist, vlist, changed);

  clearAll();
  for(int i = 0; i < ((isCDFJ() || isCDFJplus()) ? 2 : 1); ++i)
  {
    const Int32 pointervalue = myCart.getDatastreamPointer(0x21 + i) >> ds_shift;
    alist.push_back(0);  vlist.push_back(pointervalue);
    changed.push_back(pointervalue != myOldState.datastreampointers[0x21 + i]);
  }
  myJumpStreamPointers->setList(alist, vlist, changed);

  clearAll();
  for(int i = 0; i < 32; ++i)
  {
    const Int32 incrementvalue = myCart.getDatastreamIncrement(i);
    alist.push_back(0);  vlist.push_back(incrementvalue);
    changed.push_back(incrementvalue != myOldState.datastreamincrements[i]);
  }
  myDatastreamIncrements->setList(alist, vlist, changed);

  clearAll();
  alist.push_back(0);
  vlist.push_back(myCart.getDatastreamIncrement(0x20));
  changed.push_back(std::cmp_not_equal(myCart.getDatastreamIncrement(0x20), myOldState.datastreamincrements[0x20]));
  myCommandStreamIncrement->setList(alist, vlist, changed);

  clearAll();
  for(int i = 0; i < ((isCDFJ() || isCDFJplus()) ? 2 : 1); ++i)
  {
    const Int32 pointervalue = myCart.getDatastreamIncrement(0x21 + i) >> ds_shift;
    alist.push_back(0);  vlist.push_back(pointervalue);
    changed.push_back(pointervalue != myOldState.datastreamincrements[0x21 + i]);
  }
  myJumpStreamIncrements->setList(alist, vlist, changed);

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

  clearAll();
  alist.push_back(0);  vlist.push_back(myCart.getSample());
  changed.push_back(std::cmp_not_equal(myCart.getSample(),
                                       myOldState.samplepointer[0]));
  mySamplePointer->setList(alist, vlist, changed);

  myFastFetch->setState((myCart.myMode & 0x0f) == 0);
  myDigitalSample->setState((myCart.myMode & 0xf0) == 0);

  if((myCart.myMode & 0xf0) == 0)
  {
    myMusicWaveforms->setCrossed(true);
    myMusicWaveformSizes->setCrossed(true);
    mySamplePointer->setCrossed(false);
  }
  else
  {
    myMusicWaveforms->setCrossed(false);
    myMusicWaveformSizes->setCrossed(false);
    mySamplePointer->setCrossed(true);
  }

  // ARM cycles
  CartridgeARMWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDFWidget::handleCommand(CommandSender* sender,
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
string CartridgeCDFWidget::bankState()
{
  static constexpr std::array<string_view, 8> spot = {
    "$FFF4", "$FFF5", "$FFF6", "$FFF7", "$FFF8", "$FFF9", "$FFFA", "$FFFB"
  };
  return std::format("Bank = {}, hotspot = {}",
    myCart.getBank(),
    spot[myCart.getBank() + (isCDFJplus() ? 0 : 1)]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeCDFWidget::internalRamSize()
{
  return myCart.ramSize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeCDFWidget::internalRamRPort(int start)
{
  return 0x0000 + start;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeCDFWidget::internalRamDescription()
{
  if(isCDFJplus())
    return
      "$0000 - $07FF - CDFJ+ driver\n"
      "                not accessible to 6507\n"
      "$0800 - $7FFF - 30K Data Stream storage\n"
      "                indirectly accessible to 6507\n"
      "                via fast fetchers\n";
  else
    return
      "$0000 - $07FF - CDF/CDFJ driver\n"
      "                not accessible to 6507\n"
      "$0800 - $17FF - 4K Data Stream storage\n"
      "                indirectly accessible to 6507\n"
      "                via fast fetchers\n"
      "$1800 - $1FFF - 2K C variable storage and stack\n"
      "                not accessible to 6507";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeCDFWidget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  myRamOld.assign(myOldState.internalram.data() + start,
                  myOldState.internalram.data() + start + count);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeCDFWidget::internalRamCurrent(int start, int count)
{
  myRamCurrent.clear();
  myRamCurrent.assign(myCart.myRAM.data() + start,
                      myCart.myRAM.data() + start + count);
  return myRamCurrent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDFWidget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myRAM[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeCDFWidget::internalRamGetValue(int addr)
{
  return myCart.myRAM[addr];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCDFWidget::isCDFJ() const
{
  return myCart.myCDFSubtype == CartridgeCDF::CDFSubtype::CDFJ;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCDFWidget::isCDFJplus() const
{
  return myCart.isCDFJplus();
}
