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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "CartDPCPlus.hxx"
#include "DataGridWidget.hxx"
#include "PopUpWidget.hxx"
#include "CartDPCPlusWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDPCPlusWidget::CartridgeDPCPlusWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeDPCPlus& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  uInt16 size = cart.mySize;

  ostringstream info;
  info << "Extended DPC cartridge, six 4K banks, 4K display bank, 1K frequency table, "
       << "8K DPC RAM\n"
       << "DPC registers accessible @ $F000 - $F07F\n"
       << "  $F000 - $F03F (R), $F040 - $F07F (W)\n"
       << "Banks accessible at hotspots $FF6 to $FFB\n"
       << "Startup bank = " << cart.myStartBank << "\n";

#if 0
  // Eventually, we should query this from the debugger/disassembler
  for(uInt32 i = 0, offset = 0xFFC, spot = 0xFF6; i < 6; ++i, offset += 0x1000)
  {
    uInt16 start = (cart.myImage[offset+1] << 8) | cart.myImage[offset];
    start -= start % 0x1000;
    info << "Bank " << i << " @ $" << HEX4 << (start + 0x80) << " - "
         << "$" << (start + 0xFFF) << " (hotspot = $" << (spot+i) << ")\n";
  }
#endif

  int xpos = 10,
      ypos = addBaseInformation(size, "Activision (Pitfall II)", info.str()) +
              myLineHeight;

  VariantList items;
  items.push_back("0 ($FF6)");
  items.push_back("1 ($FF7)");
  items.push_back("2 ($FF8)");
  items.push_back("3 ($FF9)");
  items.push_back("4 ($FFA)");
  items.push_back("5 ($FFB)");
  myBank =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("0 ($FFx) "),
                    myLineHeight, items, "Set bank: ",
                    _font.getStringWidth("Set bank: "), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);

  // Top registers
  int lwidth = _font.getStringWidth("Counter Registers: ");
  xpos = 10;  ypos += myLineHeight + 8;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Top Registers: ", kTextAlignLeft);
  xpos += lwidth;

  myTops = new DataGridWidget(boss, _nfont, xpos, ypos-2, 8, 1, 2, 8, Common::Base::F_16);
  myTops->setTarget(this);
  myTops->setEditable(false);

  // Bottom registers
  xpos = 10;  ypos += myLineHeight + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Bottom Registers: ", kTextAlignLeft);
  xpos += lwidth;

  myBottoms = new DataGridWidget(boss, _nfont, xpos, ypos-2, 8, 1, 2, 8, Common::Base::F_16);
  myBottoms->setTarget(this);
  myBottoms->setEditable(false);

  // Counter registers
  xpos = 10;  ypos += myLineHeight + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Counter Registers: ", kTextAlignLeft);
  xpos += lwidth;

  myCounters = new DataGridWidget(boss, _nfont, xpos, ypos-2, 8, 1, 4, 16, Common::Base::F_16_4);
  myCounters->setTarget(this);
  myCounters->setEditable(false);

  // Fractional counter registers
  xpos = 10;  ypos += myLineHeight + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Frac Counters: ", kTextAlignLeft);
  xpos += lwidth;

  myFracCounters = new DataGridWidget(boss, _nfont, xpos, ypos-2, 4, 2, 8, 32, Common::Base::F_16_8);
  myFracCounters->setTarget(this);
  myFracCounters->setEditable(false);

  // Fractional increment registers
  xpos = 10;  ypos += myFracCounters->getHeight() + 8;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Frac Increments: ", kTextAlignLeft);
  xpos += lwidth;

  myFracIncrements = new DataGridWidget(boss, _nfont, xpos, ypos-2, 8, 1, 2, 8, Common::Base::F_16);
  myFracIncrements->setTarget(this);
  myFracIncrements->setEditable(false);

  // Special function parameters
  xpos = 10;  ypos += myLineHeight + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Function Params: ", kTextAlignLeft);
  xpos += lwidth;

  myParameter = new DataGridWidget(boss, _nfont, xpos, ypos-2, 8, 1, 2, 8, Common::Base::F_16);
  myParameter->setTarget(this);
  myParameter->setEditable(false);

  // Music counters
  xpos = 10;  ypos += myLineHeight + 4;
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

  myMusicWaveforms = new DataGridWidget(boss, _nfont, xpos, ypos-2, 3, 1, 4, 16, Common::Base::F_16_4);
  myMusicWaveforms->setTarget(this);
  myMusicWaveforms->setEditable(false);

  // Current random number
  lwidth = _font.getStringWidth("Current random number: ");
  xpos = 10;  ypos += myLineHeight + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Current random number: ", kTextAlignLeft);
  xpos += lwidth;

  myRandom = new DataGridWidget(boss, _nfont, xpos, ypos-2, 1, 1, 8, 32, Common::Base::F_16_8);
  myRandom->setTarget(this);
  myRandom->setEditable(false);

  // Fast fetch and immediate mode LDA flags
  xpos += myRandom->getWidth() + 30;
  myFastFetch = new CheckboxWidget(boss, _font, xpos, ypos, "Fast Fetcher enabled");
  myFastFetch->setTarget(this);
  myFastFetch->setEditable(false);
  ypos += myLineHeight + 4;
  myIMLDA = new CheckboxWidget(boss, _font, xpos, ypos, "Immediate mode LDA");
  myIMLDA->setTarget(this);
  myIMLDA->setEditable(false);
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

  for(int i = 0; i < 8; ++i)
  {
    myOldState.tops.push_back(myCart.myTops[i]);
    myOldState.bottoms.push_back(myCart.myBottoms[i]);
    myOldState.counters.push_back(myCart.myCounters[i]);
    myOldState.fraccounters.push_back(myCart.myFractionalCounters[i]);
    myOldState.fracinc.push_back(myCart.myFractionalIncrements[i]);
    myOldState.param.push_back(myCart.myParameter[i]);
  }
  for(int i = 0; i < 3; ++i)
  {
    myOldState.mcounters.push_back(myCart.myMusicCounters[i]);
    myOldState.mfreqs.push_back(myCart.myMusicFrequencies[i]);
    myOldState.mwaves.push_back(myCart.myMusicWaveforms[i]);
  }

  myOldState.random = myCart.myRandomNumber;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPCPlusWidget::loadConfig()
{
  myBank->setSelectedIndex(myCart.myCurrentBank);

  // Get registers, using change tracking
  IntArray alist;
  IntArray vlist;
  BoolArray changed;

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 8; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myTops[i]);
    changed.push_back(myCart.myTops[i] != myOldState.tops[i]);
  }
  myTops->setList(alist, vlist, changed);
  
  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 8; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myBottoms[i]);
    changed.push_back(myCart.myBottoms[i] != myOldState.bottoms[i]);
  }
  myBottoms->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 8; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myCounters[i]);
    changed.push_back(myCart.myCounters[i] != myOldState.counters[i]);
  }
  myCounters->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 8; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myFractionalCounters[i]);
    changed.push_back(myCart.myFractionalCounters[i] != (uInt32)myOldState.fraccounters[i]);
  }
  myFracCounters->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 8; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myFractionalIncrements[i]);
    changed.push_back(myCart.myFractionalIncrements[i] != myOldState.fracinc[i]);
  }
  myFracIncrements->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 8; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myParameter[i]);
    changed.push_back(myCart.myParameter[i] != myOldState.param[i]);
  }
  myParameter->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 3; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myMusicCounters[i]);
    changed.push_back(myCart.myMusicCounters[i] != (uInt32)myOldState.mcounters[i]);
  }
  myMusicCounters->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 3; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myMusicFrequencies[i]);
    changed.push_back(myCart.myMusicFrequencies[i] != (uInt32)myOldState.mfreqs[i]);
  }
  myMusicFrequencies->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 3; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myMusicWaveforms[i]);
    changed.push_back(myCart.myMusicWaveforms[i] != myOldState.mwaves[i]);
  }
  myMusicWaveforms->setList(alist, vlist, changed);

  myRandom->setList(0, myCart.myRandomNumber,
      myCart.myRandomNumber != myOldState.random);

  myFastFetch->setState(myCart.myFastFetch);
  myIMLDA->setState(myCart.myLDAimmediate);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPCPlusWidget::handleCommand(CommandSender* sender,
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
string CartridgeDPCPlusWidget::bankState()
{
  ostringstream& buf = buffer();

  static const char* spot[] = {
    "$FF6", "$FF7", "$FF8", "$FF9", "$FFA", "$FFB"
  };
  buf << "Bank = " << dec << myCart.myCurrentBank
      << ", hotspot = " << spot[myCart.myCurrentBank];

  return buf.str();
}
