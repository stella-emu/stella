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

#include "CartDPC.hxx"
#include "DataGridWidget.hxx"
#include "PopUpWidget.hxx"
#include "CartDPCWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDPCWidget::CartridgeDPCWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeDPC& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  uInt16 size = cart.mySize;

  ostringstream info;
  info << "DPC cartridge, two 4K banks + 2K display bank\n"
       << "DPC registers accessible @ $F000 - $F07F\n"
       << "  $F000 - $F03F (R), $F040 - $F07F (W)\n"

       << "Startup bank = " << cart.myStartBank << "\n";

  // Eventually, we should query this from the debugger/disassembler
  for(uInt32 i = 0, offset = 0xFFC, spot = 0xFF8; i < 2; ++i, offset += 0x1000)
  {
    uInt16 start = (cart.myImage[offset+1] << 8) | cart.myImage[offset];
    start -= start % 0x1000;
    info << "Bank " << i << " @ $" << Common::Base::HEX4 << (start + 0x80) << " - "
         << "$" << (start + 0xFFF) << " (hotspot = $" << (spot+i) << ")\n";
  }

  int xpos = 10,
      ypos = addBaseInformation(size, "Activision (Pitfall II)", info.str()) +
              myLineHeight;

  VariantList items;
  items.push_back("0 ($FF8)");
  items.push_back("1 ($FF9)");
  myBank =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("0 ($FFx) "),
                    myLineHeight, items, "Set bank: ",
                    _font.getStringWidth("Set bank: "), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
  ypos += myLineHeight + 8;

  // Data fetchers
  int lwidth = _font.getStringWidth("Data Fetchers: ");
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Data Fetchers: ", kTextAlignLeft);

  // Top registers
  lwidth = _font.getStringWidth("Counter Registers: ");
  xpos = 18;  ypos += myLineHeight + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Top Registers: ", kTextAlignLeft);
  xpos += lwidth;

  myTops = new DataGridWidget(boss, _nfont, xpos, ypos-2, 8, 1, 2, 8, Common::Base::F_16);
  myTops->setTarget(this);
  myTops->setEditable(false);

  // Bottom registers
  xpos = 18;  ypos += myLineHeight + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Bottom Registers: ", kTextAlignLeft);
  xpos += lwidth;

  myBottoms = new DataGridWidget(boss, _nfont, xpos, ypos-2, 8, 1, 2, 8, Common::Base::F_16);
  myBottoms->setTarget(this);
  myBottoms->setEditable(false);

  // Counter registers
  xpos = 18;  ypos += myLineHeight + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Counter Registers: ", kTextAlignLeft);
  xpos += lwidth;

  myCounters = new DataGridWidget(boss, _nfont, xpos, ypos-2, 8, 1, 4, 16, Common::Base::F_16_4);
  myCounters->setTarget(this);
  myCounters->setEditable(false);

  // Flag registers
  xpos = 18;  ypos += myLineHeight + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Flag Registers: ", kTextAlignLeft);
  xpos += lwidth;

  myFlags = new DataGridWidget(boss, _nfont, xpos, ypos-2, 8, 1, 2, 8, Common::Base::F_16);
  myFlags->setTarget(this);
  myFlags->setEditable(false);

  // Music mode
  xpos = 10;  ypos += myLineHeight + 12;
  lwidth = _font.getStringWidth("Music mode (DF5/DF6/DF7): ");
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Music mode (DF5/DF6/DF7): ", kTextAlignLeft);
  xpos += lwidth;

  myMusicMode = new DataGridWidget(boss, _nfont, xpos, ypos-2, 3, 1, 2, 8, Common::Base::F_16);
  myMusicMode->setTarget(this);
  myMusicMode->setEditable(false);

  // Current random number
  xpos = 10;  ypos += myLineHeight + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Current random number: ", kTextAlignLeft);
  xpos += lwidth;

  myRandom = new DataGridWidget(boss, _nfont, xpos, ypos-2, 1, 1, 2, 8, Common::Base::F_16);
  myRandom->setTarget(this);
  myRandom->setEditable(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPCWidget::saveOldState()
{
  myOldState.tops.clear();
  myOldState.bottoms.clear();
  myOldState.counters.clear();
  myOldState.flags.clear();
  myOldState.music.clear();

  for(int i = 0; i < 8; ++i)
  {
    myOldState.tops.push_back(myCart.myTops[i]);
    myOldState.bottoms.push_back(myCart.myBottoms[i]);
    myOldState.counters.push_back(myCart.myCounters[i]);
    myOldState.flags.push_back(myCart.myFlags[i]);
  }
  for(int i = 0; i < 3; ++i)
  {
    myOldState.music.push_back(myCart.myMusicMode[i]);
  }

  myOldState.random = myCart.myRandomNumber;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPCWidget::loadConfig()
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
    alist.push_back(0);  vlist.push_back(myCart.myFlags[i]);
    changed.push_back(myCart.myFlags[i] != myOldState.flags[i]);
  }
  myFlags->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
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
    myCart.unlockBank();
    myCart.bank(myBank->getSelected());
    myCart.lockBank();
    invalidate();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeDPCWidget::bankState()
{
  ostringstream& buf = buffer();

  static const char* spot[] = { "$FF8", "$FF9" };
  buf << "Bank = " << dec << myCart.myCurrentBank
      << ", hotspot = " << spot[myCart.myCurrentBank];

  return buf.str();
}
