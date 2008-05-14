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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: RiotWidget.cxx,v 1.3 2008-05-14 18:04:57 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "DataGridWidget.hxx"
#include "EditTextWidget.hxx"
#include "FrameBuffer.hxx"
#include "GuiObject.hxx"
#include "OSystem.hxx"
#include "RiotDebug.hxx"
#include "ToggleBitWidget.hxx"
#include "Widget.hxx"

#include "RiotWidget.hxx"

#define CREATE_IO_REGS(desc, bits, bitsID)                               \
  t = new StaticTextWidget(boss, font, xpos, ypos+2, lwidth, fontHeight, \
                           desc, kTextAlignLeft);                        \
  xpos += t->getWidth() + 5;                                             \
  bits = new ToggleBitWidget(boss, font, xpos, ypos, 8, 1);              \
  bits->setTarget(this);                                                 \
  bits->setID(bitsID);                                                   \
  addFocusWidget(bits);                                                  \
  xpos += bits->getWidth() + 5;                                          \
  bits->setList(off, on);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RiotWidget::RiotWidget(GuiObject* boss, const GUI::Font& font,
                     int x, int y, int w, int h)
  : Widget(boss, font, x, y, w, h),
    CommandSender(boss)
{
  _type = kRiotWidget;

  const int fontWidth  = font.getMaxCharWidth(),
            fontHeight = font.getFontHeight(),
            lineHeight = font.getLineHeight();
  int xpos = 10, ypos = 25, lwidth = 9 * fontWidth;
  StaticTextWidget* t;

  // Set the strings to be used in the various bit registers
  // We only do this once because it's the state that changes, not the strings
  StringList off, on;
  for(int i = 0; i < 8; ++i)
  {
    off.push_back("0");
    on.push_back("1");
  }

  // SWCHA bits in 'poke' mode
  CREATE_IO_REGS("SWCHA(W):", mySWCHAWriteBits, kSWCHABitsID);

  // SWACNT bits
  xpos = 10;  ypos += lineHeight + 5;
  CREATE_IO_REGS("SWACNT:", mySWACNTBits, kSWACNTBitsID);

  // SWCHA bits in 'peek' mode
  xpos = 10;  ypos += lineHeight + 5;
  CREATE_IO_REGS("SWCHA(R):", mySWCHAReadBits, 0);
  mySWCHAReadBits->setEditable(false);

  // SWCHB bits in 'peek' mode
  xpos = 10;  ypos += 2 * lineHeight;
  CREATE_IO_REGS("SWCHB:", mySWCHBBits, 0);
  mySWCHBBits->setEditable(false);

  // Timer registers (R/W)
  const char* writeNames[] = { "TIM1T:", "TIM8T:", "TIM64T:", "TIM1024T:" };
  xpos = 10;  ypos += 2*lineHeight;
  for(int row = 0; row < 4; ++row)
  {
    t = new StaticTextWidget(boss, font, xpos, ypos + row*lineHeight + 2,
                             9*fontWidth, fontHeight, writeNames[row], kTextAlignLeft);
  }
  xpos += 9*fontWidth + 5;
  myTimWrite = new DataGridWidget(boss, font, xpos, ypos, 1, 4, 2, 8, kBASE_16);
  myTimWrite->setTarget(this);
  myTimWrite->setID(kTimWriteID);
  addFocusWidget(myTimWrite);

  // Timer registers (RO)
  const char* readNames[] = { "INTIM:", "TIMINT:", "Tim Clks:" };
  xpos = 10;  ypos += myTimWrite->getHeight() + lineHeight;
  for(int row = 0; row < 3; ++row)
  {
    t = new StaticTextWidget(boss, font, xpos, ypos + row*lineHeight + 2,
                             9*fontWidth, fontHeight, readNames[row], kTextAlignLeft);
  }
  xpos += 9*fontWidth + 5;
  myTimRead = new DataGridWidget(boss, font, xpos, ypos, 1, 3, 8, 32, kBASE_16);
  myTimRead->setTarget(this);
  myTimRead->setEditable(false);
  addFocusWidget(myTimRead);


}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RiotWidget::~RiotWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RiotWidget::loadConfig()
{
#define IO_REGS_UPDATE(bits, s_bits)                          \
  changed.clear();                                            \
  for(unsigned int i = 0; i < state.s_bits.size(); ++i)       \
    changed.push_back(state.s_bits[i] != oldstate.s_bits[i]); \
  bits->setState(state.s_bits, changed);

  IntArray alist;
  IntArray vlist;
  BoolArray changed;

  // We push the enumerated items as addresses, and deal with the real
  // address in the callback (handleCommand)
  Debugger& dbg   = instance()->debugger();
  RiotDebug& riot = dbg.riotDebug();
  const RiotState& state    = (RiotState&) riot.getState();
  const RiotState& oldstate = (RiotState&) riot.getOldState();

  // Update the SWCHA register booleans (poke mode)
  IO_REGS_UPDATE(mySWCHAWriteBits, swchaWriteBits);

  // Update the SWACNT register booleans
  IO_REGS_UPDATE(mySWACNTBits, swacntBits);

  // Update the SWCHA register booleans (peek mode)
  IO_REGS_UPDATE(mySWCHAReadBits, swchaReadBits);

  // Update the SWCHB register booleans
  IO_REGS_UPDATE(mySWCHBBits, swchbBits);

  // Update timer write registers
  alist.clear();  vlist.clear();  changed.clear();
  alist.push_back(kTim1TID);  vlist.push_back(state.TIM1T);
    changed.push_back(state.TIM1T != oldstate.TIM1T);
  alist.push_back(kTim8TID);  vlist.push_back(state.TIM8T);
    changed.push_back(state.TIM8T != oldstate.TIM8T);
  alist.push_back(kTim64TID);  vlist.push_back(state.TIM64T);
    changed.push_back(state.TIM64T != oldstate.TIM64T);
  alist.push_back(kTim1024TID);  vlist.push_back(state.TIM1024T);
    changed.push_back(state.TIM1024T != oldstate.TIM1024T);
  myTimWrite->setList(alist, vlist, changed);

  // Update timer read registers
  alist.clear();  vlist.clear();  changed.clear();
  alist.push_back(0);  vlist.push_back(state.INTIM);
    changed.push_back(state.INTIM != oldstate.INTIM);
  alist.push_back(0);  vlist.push_back(state.TIMINT);
    changed.push_back(state.TIMINT != oldstate.TIMINT);
  alist.push_back(0);  vlist.push_back(state.TIMCLKS);
    changed.push_back(state.TIMCLKS != oldstate.TIMCLKS);
  myTimRead->setList(alist, vlist, changed);


}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RiotWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  int addr = -1, value = -1;
  RiotDebug& riot = instance()->debugger().riotDebug();

  switch(cmd)
  {
    case kDGItemDataChangedCmd:
      switch(id)
      {
        case kTimWriteID:
        {
          addr  = myTimWrite->getSelectedAddr();
          value = myTimWrite->getSelectedValue();
          switch(addr)
          {
            case kTim1TID:
              riot.tim1T(value);
              break;
            case kTim8TID:
              riot.tim8T(value);
              break;
            case kTim64TID:
              riot.tim64T(value);
              break;
            case kTim1024TID:
              riot.tim1024T(value);
              break;
          }
        }
      }
      break;

    case kTWItemDataChangedCmd:
      switch(id)
      {
        case kSWCHABitsID:
          value = Debugger::get_bits((BoolArray&)mySWCHAWriteBits->getState());
          riot.swcha(value & 0xff);
          break;
        case kSWACNTBitsID:
          value = Debugger::get_bits((BoolArray&)mySWACNTBits->getState());
          riot.swacnt(value & 0xff);
          break;
      }
      break;
  }
}
