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
// $Id: RiotWidget.cxx,v 1.1 2008-04-29 19:11:42 stephena Exp $
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

#define CREATE_IO_REGS(desc, bits, bitsID, reg)                          \
  t = new StaticTextWidget(boss, font, xpos, ypos+2, lwidth, fontHeight, \
                           desc, kTextAlignLeft);                        \
  xpos += t->getWidth() + 5;                                             \
  bits = new ToggleBitWidget(boss, font, xpos, ypos, 8, 1);              \
  bits->setTarget(this);                                                 \
  bits->setID(bitsID);                                                   \
  addFocusWidget(bits);                                                  \
  xpos += bits->getWidth() + 5;                                          \
  reg = new DataGridWidget(boss, font, xpos, ypos, 1, 1, 2, 8, kBASE_16);\
  reg->setTarget(this);                                                  \
  reg->setEditable(false);                                               \
  addFocusWidget(reg);                                                   \
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
  int xpos = 10, ypos = 25, lwidth = 7 * fontWidth;
  StaticTextWidget* t;

  // Set the strings to be used in the various bit registers
  // We only do this once because it's the state that changes, not the strings
  StringList off, on;
  for(int i = 0; i < 8; ++i)
  {
    off.push_back("0");
    on.push_back("1");
  }

  // SWCHA bits and actual value
  CREATE_IO_REGS("SWCHA:", mySWCHABits, kSWCHABitsID, mySWCHA);

  // SWACNT bits and actual value
  xpos = 10;  ypos += lineHeight + 5;
  CREATE_IO_REGS("SWACNT:", mySWACNTBits, kSWACNTBitsID, mySWACNT);

  // SWCHB bits and actual value
  xpos = 10;  ypos += 2 * lineHeight;
  CREATE_IO_REGS("SWCHB:", mySWCHBBits, kSWCHBBitsID, mySWCHB);

  // SWBCNT bits and actual value
  xpos = 10;  ypos += lineHeight + 5;
  CREATE_IO_REGS("SWBCNT:", mySWBCNTBits, kSWBCNTBitsID, mySWBCNT);
//  mySWBCNTBits->setEnabled(false);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RiotWidget::~RiotWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RiotWidget::loadConfig()
{
#define IO_REGS_UPDATE(bits, reg, s_bits, s_reg)              \
  changed.clear();                                            \
  for(unsigned int i = 0; i < state.s_bits.size(); ++i)       \
    changed.push_back(state.s_bits[i] != oldstate.s_bits[i]); \
  bits->setState(state.s_bits, changed);                      \
  reg->setList(state.s_reg, oldstate.s_reg, state.s_reg != oldstate.s_reg);

  IntArray alist;
  IntArray vlist;
  BoolArray changed;

  // We push the enumerated items as addresses, and deal with the real
  // address in the callback (handleCommand)
  Debugger& dbg   = instance()->debugger();
  RiotDebug& riot = dbg.riotDebug();
  const RiotState& state    = (RiotState&) riot.getState();
  const RiotState& oldstate = (RiotState&) riot.getOldState();

  // Update the SWCHA register booleans
  IO_REGS_UPDATE(mySWCHABits, mySWCHA, swchaBits, SWCHA);

  // Update the SWACNT register booleans
  IO_REGS_UPDATE(mySWACNTBits, mySWACNT, swacntBits, SWACNT);

  // Update the SWCHB register booleans
  IO_REGS_UPDATE(mySWCHBBits, mySWCHB, swchbBits, SWCHB);

  // Update the SWBCNT register booleans
  IO_REGS_UPDATE(mySWBCNTBits, mySWBCNT, swbcntBits, SWBCNT);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RiotWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
}
