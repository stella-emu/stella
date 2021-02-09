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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "DataGridWidget.hxx"
#include "PopUpWidget.hxx"
#include "EditTextWidget.hxx"
#include "CartCDFWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCDFWidget::CartridgeCDFWidget(
    GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
    int x, int y, int w, int h, CartridgeCDF& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart{cart}
{
  const int VBORDER = 8;
  const int HBORDER = 2;
  const int INDENT = 20;
  const int VGAP = 4;

  int xpos = HBORDER, ypos = VBORDER;

  VariantList items;
  if (isCDFJplus()) {
    VarList::push_back(items, "0 ($FFF4)");
    VarList::push_back(items, "1 ($FFF5)");
    VarList::push_back(items, "2 ($FFF6)");
    VarList::push_back(items, "3 ($FFF7)");
    VarList::push_back(items, "4 ($FFF8)");
    VarList::push_back(items, "5 ($FFF9)");
    VarList::push_back(items, "6 ($FFFA)");
  } else {
    VarList::push_back(items, "0 ($FFF5)");
    VarList::push_back(items, "1 ($FFF6)");
    VarList::push_back(items, "2 ($FFF7)");
    VarList::push_back(items, "3 ($FFF8)");
    VarList::push_back(items, "4 ($FFF9)");
    VarList::push_back(items, "5 ($FFFA)");
    VarList::push_back(items, "6 ($FFFB)");
  }
  myBank = new PopUpWidget(boss, _font, xpos, ypos, _font.getStringWidth("0 ($FFFx)"),
                           myLineHeight, items,
                           "Set bank ", 0, kBankChanged);
  myBank->setTarget(this);
  //addFocusWidget(myBank);

  // Fast Fetch flag
  myFastFetch = new CheckboxWidget(boss, _font, myBank->getRight() + 24, ypos + 1,
                                   "Fast Fetcher enabled");
  myFastFetch->setTarget(this);
  myFastFetch->setEditable(false);

  int lwidth;

  // Datastream Pointers
#define DS_X (HBORDER + _font.getStringWidth("xx "))
  xpos = DS_X;
  ypos += myLineHeight + VGAP * 3;
  new StaticTextWidget(boss, _font, xpos, ypos, "Datastream Pointers");

  myDatastreamPointers = new DataGridWidget(boss, _nfont, DS_X,
                                            ypos+myLineHeight, 4, 8, 6, 32,
                                            Common::Base::Fmt::_16_3_2);
  myDatastreamPointers->setTarget(this);
  myDatastreamPointers->setEditable(false);

  myCommandStreamPointer = new DataGridWidget(boss, _nfont, DS_X  + myDatastreamPointers->getWidth() * 3 / 4,
                                              ypos+myLineHeight + 8*myLineHeight, 1, 1, 6, 32,
                                              Common::Base::Fmt::_16_3_2);
  myCommandStreamPointer->setTarget(this);
  myCommandStreamPointer->setEditable(false);

  if (isCDFJ() || isCDFJplus())
    myJumpStreamPointers = new DataGridWidget(boss, _nfont, DS_X  + myDatastreamPointers->getWidth() * 2 / 4,
                                              ypos+myLineHeight + 9*myLineHeight, 2, 1, 6, 32,
                                              Common::Base::Fmt::_16_3_2);
  else
    myJumpStreamPointers = new DataGridWidget(boss, _nfont, DS_X  + myDatastreamPointers->getWidth() * 3 / 4,
                                              ypos+myLineHeight + 9*myLineHeight, 1, 1, 6, 32,
                                              Common::Base::Fmt::_16_3_2);
  myJumpStreamPointers->setTarget(this);
  myJumpStreamPointers->setEditable(false);

  uInt32 row;
  for(row = 0; row < 8; ++row)
  {
    myDatastreamLabels[row] =
    new StaticTextWidget(_boss, _font, DS_X - _font.getStringWidth("xx "),
                         ypos+myLineHeight + row*myLineHeight + 2, "   ");
    myDatastreamLabels[row]->setLabel(Common::Base::toString(row * 4,
                                      Common::Base::Fmt::_16_2));
  }
  lwidth = _font.getStringWidth("Jump Data (21|22)");
  myDatastreamLabels[8] =
  new StaticTextWidget(_boss, _font, DS_X - _font.getStringWidth("xx "),
                       ypos+myLineHeight + 8*myLineHeight + 2,
                       lwidth, myFontHeight, "Write Data (20)");
  myDatastreamLabels[9] =
  new StaticTextWidget(_boss, _font, DS_X - _font.getStringWidth("xx "),
                       ypos+myLineHeight + 9*myLineHeight + 2,
                       lwidth, myFontHeight,
                       (isCDFJ() || isCDFJplus()) ? "Jump Data (21|22)" : "Jump Data (21)");

  // Datastream Increments
  xpos = DS_X + myDatastreamPointers->getWidth() + 16;
  new StaticTextWidget(boss, _font, xpos, ypos, "Datastream Increments");

  myDatastreamIncrements = new DataGridWidget(boss, _nfont, xpos,
                                              ypos+myLineHeight, 4, 8, 5, 32,
                                              Common::Base::Fmt::_16_2_2);
  myDatastreamIncrements->setTarget(this);
  myDatastreamIncrements->setEditable(false);

  myCommandStreamIncrement = new DataGridWidget(boss, _nfont, xpos,
                                                ypos+myLineHeight + 8*myLineHeight, 1, 1, 5, 32,
                                                Common::Base::Fmt::_16_2_2);
  myCommandStreamIncrement->setTarget(this);
  myCommandStreamIncrement->setEditable(false);

  myJumpStreamIncrements = new DataGridWidget(boss, _nfont, xpos,
                                              ypos+myLineHeight + 9*myLineHeight, (isCDFJ() || isCDFJplus()) ? 2 : 1, 1, 5, 32,
                                              Common::Base::Fmt::_16_2_2);
  myJumpStreamIncrements->setTarget(this);
  myJumpStreamIncrements->setEditable(false);
  xpos = HBORDER;  ypos += myLineHeight * 11 + VGAP * 3;

  lwidth = _font.getStringWidth("Waveform Sizes ");

  // Music counters
  new StaticTextWidget(_boss, _font, xpos, ypos, "Music States:");
  xpos += INDENT;
  ypos += myLineHeight + VGAP;

  new StaticTextWidget(boss, _font, xpos, ypos, "Counters");
  xpos += lwidth;

  myMusicCounters = new DataGridWidget(boss, _nfont, xpos, ypos-2, 3, 1, 8, 32,
                                       Common::Base::Fmt::_16_8);
  myMusicCounters->setTarget(this);
  myMusicCounters->setEditable(false);

  // Music frequencies
  xpos = HBORDER + INDENT;  ypos += myLineHeight + VGAP;
  new StaticTextWidget(boss, _font, xpos, ypos, "Frequencies");
  xpos += lwidth;

  myMusicFrequencies = new DataGridWidget(boss, _nfont, xpos, ypos-2, 3, 1, 8, 32,
                                          Common::Base::Fmt::_16_8);
  myMusicFrequencies->setTarget(this);
  myMusicFrequencies->setEditable(false);

  // Music waveforms
  xpos = HBORDER + INDENT;  ypos += myLineHeight + VGAP;
  new StaticTextWidget(boss, _font, xpos, ypos, "Waveforms");
  xpos += lwidth;

  myMusicWaveforms = new DataGridWidget(boss, _nfont, xpos, ypos-2, 3, 1, 8, 16,
                                        Common::Base::Fmt::_16_2);
  myMusicWaveforms->setTarget(this);
  myMusicWaveforms->setEditable(false);

  // Music waveform sizes
  xpos = HBORDER + INDENT;  ypos += myLineHeight + VGAP;
  new StaticTextWidget(boss, _font, xpos, ypos, "Waveform Sizes");
  xpos += lwidth;

  myMusicWaveformSizes = new DataGridWidget(boss, _nfont, xpos, ypos-2, 3, 1, 8, 16,
                                            Common::Base::Fmt::_16_2);
  myMusicWaveformSizes->setTarget(this);
  myMusicWaveformSizes->setEditable(false);

  // Digital Audio flag
  xpos = HBORDER;  ypos += myLineHeight + VGAP;

  myDigitalSample = new CheckboxWidget(boss, _font, xpos, ypos, "Digital Sample mode");
  myDigitalSample->setTarget(this);
  myDigitalSample->setEditable(false);

  xpos = HBORDER + INDENT;  ypos += myLineHeight + VGAP;

  int lwidth2 = _font.getStringWidth("Sample Pointer ");
  new StaticTextWidget(boss, _font, xpos, ypos, "Sample Pointer");

  mySamplePointer = new DataGridWidget(boss, _nfont, xpos + lwidth2, ypos - 2, 1, 1, 8, 32,
                                       Common::Base::Fmt::_16_8);
  mySamplePointer->setTarget(this);
  mySamplePointer->setEditable(false);

  xpos = HBORDER;  ypos += myLineHeight + VGAP * 2;
  new StaticTextWidget(boss, _font, xpos, ypos + 1, "Last ARM run stats:");
  xpos = HBORDER + INDENT; ypos += myLineHeight + VGAP;
  StaticTextWidget* s = new StaticTextWidget(boss, _font, xpos, ypos + 1, "Mem. cycles ");
  myThumbMemCycles = new EditTextWidget(boss, _font, s->getRight(), ypos - 1,
                                           EditTextWidget::calcWidth(_font, 6), myLineHeight, "");
  myThumbMemCycles->setEditable(false);
  myThumbMemCycles->setToolTip("Number of memory cycles of last ARM run.");

  s = new StaticTextWidget(boss, _font, myThumbMemCycles->getRight() + _fontWidth * 2, ypos + 1, "Fetches ");
  myThumbFetches = new EditTextWidget(boss, _font, s->getRight(), ypos - 1,
                                           EditTextWidget::calcWidth(_font, 6), myLineHeight, "");
  myThumbFetches->setEditable(false);
  myThumbFetches->setToolTip("Number of fetches/instructions of last ARM run.");

  ypos += myLineHeight + VGAP;
  s = new StaticTextWidget(boss, _font, xpos, ypos + 1, "Reads ");
  myThumbReads = new EditTextWidget(boss, _font, myThumbMemCycles->getLeft(), ypos - 1,
                                    EditTextWidget::calcWidth(_font, 6), myLineHeight, "");
  myThumbReads->setEditable(false);
  myThumbReads->setToolTip("Number of reads of last ARM run.");

  s = new StaticTextWidget(boss, _font, myThumbReads->getRight() + _fontWidth * 2, ypos + 1, "Writes ");
  myThumbWrites = new EditTextWidget(boss, _font, myThumbFetches->getLeft(), ypos - 1,
                                    EditTextWidget::calcWidth(_font, 6), myLineHeight, "");
  myThumbWrites->setEditable(false);
  myThumbWrites->setToolTip("Number of write of last ARM run.");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDFWidget::saveOldState()
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

  for(uInt32 i = 0; i < static_cast<uInt32>((isCDFJ() || isCDFJplus()) ? 35 : 34); ++i)
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
    myOldState.datastreamincrements.push_back(myCart.getDatastreamIncrement(i));
  }

  for(uInt32 i = 0; i < 3; ++i)
    myOldState.mcounters.push_back(myCart.myMusicCounters[i]);

  for(uInt32 i = 0; i < 3; ++i)
  {
    myOldState.mfreqs.push_back(myCart.myMusicFrequencies[i]);
    myOldState.mwaves.push_back(myCart.getWaveform(i) >> 5);
    myOldState.mwavesizes.push_back(myCart.getWaveformSize((i)));
  }

  for(uInt32 i = 0; i < internalRamSize(); ++i)
    myOldState.internalram.push_back(myCart.myRAM[i]);

  myOldState.samplepointer.push_back(myCart.getSample());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDFWidget::loadConfig()
{
  myBank->setSelectedIndex(myCart.getBank());

  // Get registers, using change tracking
  IntArray alist;
  IntArray vlist;
  BoolArray changed;

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 32; ++i)
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

    Int32 pointervalue = myCart.getDatastreamPointer(i) >> 12;
    alist.push_back(0);  vlist.push_back(pointervalue);
    changed.push_back(pointervalue != myOldState.datastreampointers[i]);
  }
  myDatastreamPointers->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 32; i < 34; ++i)
  {
    Int32 pointervalue = myCart.getDatastreamPointer(i) >> 12;
    alist.push_back(0);  vlist.push_back(pointervalue);
    changed.push_back(pointervalue != myOldState.datastreampointers[i]);
  }

  alist.clear();  vlist.clear();  changed.clear();
  alist.push_back(0);
  vlist.push_back(myCart.getDatastreamPointer(0x20) >> 12);
  changed.push_back(static_cast<Int32>(myCart.getDatastreamPointer(0x20)) != myOldState.datastreampointers[0x20]);
  myCommandStreamPointer->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < ((isCDFJ() || isCDFJplus()) ? 2 : 1); ++i)
  {
    Int32 pointervalue = myCart.getDatastreamPointer(0x21 + i) >> 12;
    alist.push_back(0);  vlist.push_back(pointervalue);
    changed.push_back(pointervalue != myOldState.datastreampointers[0x21 + i]);
  }
  myJumpStreamPointers->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 32; ++i)
  {
    Int32 incrementvalue = myCart.getDatastreamIncrement(i);
    alist.push_back(0);  vlist.push_back(incrementvalue);
    changed.push_back(incrementvalue != myOldState.datastreamincrements[i]);
  }
  myDatastreamIncrements->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  alist.push_back(0);
  vlist.push_back(myCart.getDatastreamIncrement(0x20));
  changed.push_back(static_cast<Int32>(myCart.getDatastreamIncrement(0x20)) != myOldState.datastreamincrements[0x20]);
  myCommandStreamIncrement->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < ((isCDFJ() || isCDFJplus()) ? 2 : 1); ++i)
  {
    Int32 pointervalue = myCart.getDatastreamIncrement(0x21 + i) >> 12;
    alist.push_back(0);  vlist.push_back(pointervalue);
    changed.push_back(pointervalue != myOldState.datastreamincrements[0x21 + i]);
  }
  myJumpStreamIncrements->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 3; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myMusicCounters[i]);
    changed.push_back(myCart.myMusicCounters[i] != uInt32(myOldState.mcounters[i]));
  }
  myMusicCounters->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 3; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myMusicFrequencies[i]);
    changed.push_back(myCart.myMusicFrequencies[i] != uInt32(myOldState.mfreqs[i]));
  }
  myMusicFrequencies->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 3; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.getWaveform(i) >> 5);
    changed.push_back((myCart.getWaveform(i) >> 5) != uInt32(myOldState.mwaves[i]));
  }
  myMusicWaveforms->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 3; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.getWaveformSize(i));
    changed.push_back((myCart.getWaveformSize(i)) != uInt32(myOldState.mwavesizes[i]));
  }
  myMusicWaveformSizes->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  alist.push_back(0);  vlist.push_back(myCart.getSample());
  changed.push_back((myCart.getSample()) != uInt32(myOldState.samplepointer[0]));
  mySamplePointer->setList(alist, vlist, changed);

  myFastFetch->setState((myCart.myMode & 0x0f) == 0);
  myDigitalSample->setState((myCart.myMode & 0xf0) == 0);

  if ((myCart.myMode & 0xf0) == 0)
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

  myThumbMemCycles->setText(Common::Base::toString(myCart.stats().fetches
                            + myCart.stats().reads + myCart.stats().writes,
                            Common::Base::Fmt::_10_6));
  myThumbFetches->setText(Common::Base::toString(myCart.stats().fetches,
                          Common::Base::Fmt::_10_6));
  myThumbReads->setText(Common::Base::toString(myCart.stats().reads,
                        Common::Base::Fmt::_10_6));
  myThumbWrites->setText(Common::Base::toString(myCart.stats().writes,
                         Common::Base::Fmt::_10_6));

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDFWidget::handleCommand(CommandSender* sender,
                                       int cmd, int data, int id)
{
  if(cmd == kBankChanged)
  {
    myCart.unlockBank();
    myCart.bank(myBank->getSelected());
    myCart.lockBank();
    invalidate();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeCDFWidget::bankState()
{
  ostringstream& buf = buffer();

  static constexpr std::array<const char*, 8> spot = {
    "$FFF4", "$FFF5", "$FFF6", "$FFF7", "$FFF8", "$FFF9", "$FFFA", "$FFFB"
  };

  buf << "Bank = " << std::dec << myCart.getBank()
  << ", hotspot = " << spot[myCart.getBank() + (isCDFJplus() ? 0 : 1)];

  return buf.str();
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
  ostringstream desc;
  if (isCDFJplus()) {
    desc << "$0000 - $07FF - CDFJ+ driver\n"
    << "                not accessible to 6507\n"
    << "$0800 - $7FFF - 30K Data Stream storage\n"
    << "                indirectly accessible to 6507\n"
    << "                via fast fecthers\n";
  } else {
    desc << "$0000 - $07FF - CDF/CDFJ driver\n"
    << "                not accessible to 6507\n"
    << "$0800 - $17FF - 4K Data Stream storage\n"
    << "                indirectly accessible to 6507\n"
    << "                via fast fetchers\n"
    << "$1800 - $1FFF - 2K C variable storage and stack\n"
    << "                not accessible to 6507";
  }

  return desc.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeCDFWidget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  for(int i = 0; i < count; i++)
    myRamOld.push_back(myOldState.internalram[start + i]);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeCDFWidget::internalRamCurrent(int start, int count)
{
  myRamCurrent.clear();
  for(int i = 0; i < count; i++)
    myRamCurrent.push_back(myCart.myRAM[start + i]);
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
string CartridgeCDFWidget::describeCDFVersion(CartridgeCDF::CDFSubtype subtype)
{
  switch(subtype)
  {
    case CartridgeCDF::CDFSubtype::CDF0:
      return "CDF (v0)";

    case CartridgeCDF::CDFSubtype::CDF1:
      return "CDF (v1)";

    case CartridgeCDF::CDFSubtype::CDFJ:
      return "CDFJ";

    case CartridgeCDF::CDFSubtype::CDFJplus:
      return "CDFJ+";

    default:
      throw runtime_error("unreachable");
  }
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
