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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "CartCDF.hxx"
#include "DataGridWidget.hxx"
#include "PopUpWidget.hxx"
#include "CartCDFWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCDFWidget::CartridgeCDFWidget(
    GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
    int x, int y, int w, int h, CartridgeCDF& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  uInt16 size = 8 * 4096;

  ostringstream info;
  info << "CDF Stuffing cartridge\n"
  << "32K ROM, seven 4K banks are accessible to 2600\n"
  << "8K CDF RAM\n"
  << "CDF registers accessible @ $F000 - $F03F\n"
  << "Banks accessible at hotspots $FF5 to $FFB\n"
  << "Startup bank = " << cart.myStartBank << "\n";

#if 0
  // Eventually, we should query this from the debugger/disassembler
  for(uInt32 i = 0, offset = 0xFFC, spot = 0xFF5; i < 7; ++i, offset += 0x1000)
  {
    uInt16 start = (cart.myImage[offset+1] << 8) | cart.myImage[offset];
    start -= start % 0x1000;
    info << "Bank " << i << " @ $" << HEX4 << (start + 0x80) << " - "
    << "$" << (start + 0xFFF) << " (hotspot = $" << (spot+i) << ")\n";
  }
#endif

  int xpos = 10,
  ypos = addBaseInformation(size, "AtariAge", info.str()) +
  myLineHeight;

  VariantList items;
  VarList::push_back(items, "0 ($FF5)");
  VarList::push_back(items, "1 ($FF6)");
  VarList::push_back(items, "2 ($FF7)");
  VarList::push_back(items, "3 ($FF8)");
  VarList::push_back(items, "4 ($FF9)");
  VarList::push_back(items, "5 ($FFA)");
  VarList::push_back(items, "6 ($FFB)");
  myBank =
  new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("0 ($FFx) "),
                  myLineHeight, items, "Set bank: ",
                  _font.getStringWidth("Set bank: "), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);

  int lwidth = _font.getStringWidth("Datastream Increments: "); // get width of the widest label

  // Datastream Pointers
  xpos = 0;  ypos += myLineHeight + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
                       myFontHeight, "Datastream Pointers: ", kTextAlignLeft);
  xpos += lwidth;

  myDatastreamPointers = new DataGridWidget(boss, _nfont, 0, ypos+myLineHeight-2, 4, 8, 6, 32, Common::Base::F_16_3_2);
  myDatastreamPointers->setTarget(this);
  myDatastreamPointers->setEditable(false);

  // Datastream Increments
  xpos = 0 + myDatastreamPointers->getWidth();
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
                       myFontHeight, "Datastream Increments: ", kTextAlignLeft);

  myDatastreamIncrements = new DataGridWidget(boss, _nfont, xpos, ypos+myLineHeight-2, 4, 8, 5, 32, Common::Base::F_16_2_2);
  myDatastreamIncrements->setTarget(this);
  myDatastreamIncrements->setEditable(false);

  // Music counters
  xpos = 10;  ypos += myLineHeight*10 + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
                       myFontHeight, "Music Counters: ", kTextAlignLeft);
  xpos += lwidth;

  myMusicCounters = new DataGridWidget(boss, _nfont, xpos, ypos-2, 3, 1, 8, 32, Common::Base::F_16_8);
  myMusicCounters->setTarget(this);
  myMusicCounters->setEditable(false);

  // Music frequencies
  xpos = 10;  ypos += myLineHeight + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
                       myFontHeight, "Music Frequencies: ", kTextAlignLeft);
  xpos += lwidth;

  myMusicFrequencies = new DataGridWidget(boss, _nfont, xpos, ypos-2, 3, 1, 8, 32, Common::Base::F_16_8);
  myMusicFrequencies->setTarget(this);
  myMusicFrequencies->setEditable(false);

  // Music waveforms
  xpos = 10;  ypos += myLineHeight + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
                       myFontHeight, "Music Waveforms: ", kTextAlignLeft);
  xpos += lwidth;

  myMusicWaveforms = new DataGridWidget(boss, _nfont, xpos, ypos-2, 3, 1, 4, 16, Common::Base::F_16_2);
  myMusicWaveforms->setTarget(this);
  myMusicWaveforms->setEditable(false);

  // Music waveform sizes
  xpos = 10;  ypos += myLineHeight + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
                       myFontHeight, "Music Waveform Sizes: ", kTextAlignLeft);
  xpos += lwidth;

  myMusicWaveformSizes = new DataGridWidget(boss, _nfont, xpos, ypos-2, 3, 1, 4, 16, Common::Base::F_16_2);
  myMusicWaveformSizes->setTarget(this);
  myMusicWaveformSizes->setEditable(false);

  // done differently than in DPC+, need to rethink debugger support
//  // Fast fetch and immediate mode LDA flags
//  xpos = 10;  ypos += myLineHeight + 4;
//  myFastFetch = new CheckboxWidget(boss, _font, xpos, ypos, "Fast Fetcher enabled");
//  myFastFetch->setTarget(this);
//  myFastFetch->setEditable(false);
//  ypos += myLineHeight + 4;
//  myIMLDA = new CheckboxWidget(boss, _font, xpos, ypos, "Immediate mode LDA");
//  myIMLDA->setTarget(this);
//  myIMLDA->setEditable(false);
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

  for(uInt32 i = 0; i < 32; i++)
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
  {
    myOldState.mcounters.push_back(myCart.myMusicCounters[i]);
  }

  for(uInt32 i = 0; i < 3; ++i)
  {
    myOldState.mfreqs.push_back(myCart.myMusicFrequencies[i]);
    myOldState.mwaves.push_back(myCart.getWaveform(i) >> 5);
    myOldState.mwavesizes.push_back(myCart.getWaveformSize((i)));
  }

  for(uInt32 i = 0; i < internalRamSize(); ++i)
    myOldState.internalram.push_back(myCart.myCDFRAM[i]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDFWidget::loadConfig()
{
  myBank->setSelectedIndex(myCart.myCurrentBank);

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
  for(int i = 0; i < 32; ++i)
  {
    Int32 incrementvalue = myCart.getDatastreamIncrement(i);
    alist.push_back(0);  vlist.push_back(incrementvalue);
    changed.push_back(incrementvalue != myOldState.datastreamincrements[i]);
  }
  myDatastreamIncrements->setList(alist, vlist, changed);

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

// done differently than in DPC+, need to rethink debugger support
//  myFastFetch->setState(myCart.myFastFetch);
//  myIMLDA->setState(myCart.myLDAimmediate);

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

  static const char* spot[] = {
    "$FF5", "$FF6", "$FF7", "$FF8", "$FF9", "$FFA", "$FFB"
  };
  buf << "Bank = " << std::dec << myCart.myCurrentBank
  << ", hotspot = " << spot[myCart.myCurrentBank];

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeCDFWidget::internalRamSize()
{
  return 8*1024;
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
  desc << "$0000 - $07FF - CDF driver\n"
  << "                not accessible to 6507\n"
  << "$0800 - $17FF - 4K Data Stream storage\n"
  << "                indirectly accessible to 6507\n"
  << "                via CDF's Data Stream registers\n"
  << "$1800 - $1FFF - 2K C variable storage and stack\n"
  << "                not accessible to 6507";

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
    myRamCurrent.push_back(myCart.myCDFRAM[start + i]);
  return myRamCurrent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDFWidget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myCDFRAM[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeCDFWidget::internalRamGetValue(int addr)
{
  return myCart.myCDFRAM[addr];
}
