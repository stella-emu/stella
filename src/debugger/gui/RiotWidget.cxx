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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
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
#include "PopUpWidget.hxx"
#include "ToggleBitWidget.hxx"
#include "Widget.hxx"

#include "NullControlWidget.hxx"
#include "JoystickWidget.hxx"
#include "PaddleWidget.hxx"
#include "BoosterWidget.hxx"

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
  int xpos = 10, ypos = 25, lwidth = 9 * fontWidth, col = 0;
  StaticTextWidget* t;
  StringMap items;

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
  col = xpos + 20;  // remember this for adding widgets to the second column

  // SWACNT bits
  xpos = 10;  ypos += lineHeight + 5;
  CREATE_IO_REGS("SWACNT:", mySWACNTBits, kSWACNTBitsID);

  // SWCHA bits in 'peek' mode
  xpos = 10;  ypos += lineHeight + 5;
  CREATE_IO_REGS("SWCHA(R):", mySWCHAReadBits, 0);
  mySWCHAReadBits->setEditable(false);

  // SWCHB bits in 'poke' mode
  xpos = 10;  ypos += 2 * lineHeight;
  CREATE_IO_REGS("SWCHB(W):", mySWCHBWriteBits, kSWCHBBitsID);

  // SWBCNT bits
  xpos = 10;  ypos += lineHeight + 5;
  CREATE_IO_REGS("SWBCNT:", mySWBCNTBits, kSWBCNTBitsID);

  // SWCHB bits in 'peek' mode
  xpos = 10;  ypos += lineHeight + 5;
  CREATE_IO_REGS("SWCHB(R):", mySWCHBReadBits, 0);
  mySWCHBReadBits->setEditable(false);

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

  // Controller ports
  const RiotDebug& riot = instance().debugger().riotDebug();
  xpos = col;  ypos = 10;
  myLeftControl = addControlWidget(boss, font, xpos, ypos,
      riot.controller(Controller::Left));
  xpos += myLeftControl->getWidth() + 15;
  myRightControl = addControlWidget(boss, font, xpos, ypos,
      riot.controller(Controller::Right));

  // PO & P1 difficulty switches
  int pwidth = font.getStringWidth("B/easy");
  lwidth = font.getStringWidth("P0 Diff: ");
  xpos = col;  ypos += myLeftControl->getHeight() + 2 * lineHeight;
  items.clear();
  items.push_back("B/easy", "b");
  items.push_back("A/hard", "a");
  myP0Diff = new PopUpWidget(boss, font, xpos, ypos, pwidth, lineHeight, items,
                             "P0 Diff: ", lwidth, kP0DiffChanged);
  myP0Diff->setTarget(this);
  addFocusWidget(myP0Diff);
  ypos += myP0Diff->getHeight() + 5;
  myP1Diff = new PopUpWidget(boss, font, xpos, ypos, pwidth, lineHeight, items,
                             "P1 Diff: ", lwidth, kP1DiffChanged);
  myP1Diff->setTarget(this);
  addFocusWidget(myP1Diff);

  // TV Type
  ypos += myP1Diff->getHeight() + 5;
  items.clear();
  items.push_back("B&W", "bw");
  items.push_back("Color", "color");
  myTVType = new PopUpWidget(boss, font, xpos, ypos, pwidth, lineHeight, items,
                             "TV Type: ", lwidth, kTVTypeChanged);
  myTVType->setTarget(this);
  addFocusWidget(myTVType);

  // Select and Reset
  xpos += 20;  ypos += myTVType->getHeight() + 5;
  mySelect = new CheckboxWidget(boss, font, xpos, ypos, "Select",
                                kCheckActionCmd);
  mySelect->setID(kSelectID);
  mySelect->setTarget(this);
  addFocusWidget(mySelect);
  ypos += myTVType->getHeight() + 5;
  myReset = new CheckboxWidget(boss, font, xpos, ypos, "Reset",
                               kCheckActionCmd);
  myReset->setID(kResetID);
  myReset->setTarget(this);
  addFocusWidget(myReset);
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
  Debugger& dbg   = instance().debugger();
  RiotDebug& riot = dbg.riotDebug();
  const RiotState& state    = (RiotState&) riot.getState();
  const RiotState& oldstate = (RiotState&) riot.getOldState();

  // Update the SWCHA register booleans (poke mode)
  IO_REGS_UPDATE(mySWCHAWriteBits, swchaWriteBits);

  // Update the SWACNT register booleans
  IO_REGS_UPDATE(mySWACNTBits, swacntBits);

  // Update the SWCHA register booleans (peek mode)
  IO_REGS_UPDATE(mySWCHAReadBits, swchaReadBits);

  // Update the SWCHB register booleans (poke mode)
  IO_REGS_UPDATE(mySWCHBWriteBits, swchbWriteBits);

  // Update the SWBCNT register booleans
  IO_REGS_UPDATE(mySWBCNTBits, swbcntBits);

  // Update the SWCHB register booleans (peek mode)
  IO_REGS_UPDATE(mySWCHBReadBits, swchbReadBits);

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

  // Console switches (inverted, since 'selected' in the UI
  // means 'grounded' in the system)
  myP0Diff->setSelected((int)riot.diffP0());
  myP1Diff->setSelected((int)riot.diffP1());
  myTVType->setSelected((int)riot.tvType());
  mySelect->setState(!riot.select());
  myReset->setState(!riot.reset());

  myLeftControl->loadConfig();
  myRightControl->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RiotWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  int value = -1;
  RiotDebug& riot = instance().debugger().riotDebug();

  switch(cmd)
  {
    case kDGItemDataChangedCmd:
      switch(id)
      {
        case kTimWriteID:
          switch(myTimWrite->getSelectedAddr())
          {
            case kTim1TID:
              riot.tim1T(myTimWrite->getSelectedValue());
              break;
            case kTim8TID:
              riot.tim8T(myTimWrite->getSelectedValue());
              break;
            case kTim64TID:
              riot.tim64T(myTimWrite->getSelectedValue());
              break;
            case kTim1024TID:
              riot.tim1024T(myTimWrite->getSelectedValue());
              break;
          }
          break;
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
        case kSWCHBBitsID:
          value = Debugger::get_bits((BoolArray&)mySWCHBWriteBits->getState());
          riot.swchb(value & 0xff);
          break;
        case kSWBCNTBitsID:
          value = Debugger::get_bits((BoolArray&)mySWBCNTBits->getState());
          riot.swbcnt(value & 0xff);
          break;
      }
      break;

    case kCheckActionCmd:
      switch(id)
      {
        case kSelectID:
          riot.select(!mySelect->getState());
          break;
        case kResetID:
          riot.reset(!myReset->getState());
          break;
      }
      break;

    case kP0DiffChanged:
      riot.diffP0(myP0Diff->getSelectedTag() != "b");
      break;

    case kP1DiffChanged:
      riot.diffP1(myP1Diff->getSelectedTag() != "b");
      break;

    case kTVTypeChanged:
      riot.tvType((bool)myTVType->getSelected());
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ControllerWidget* RiotWidget::addControlWidget(GuiObject* boss, const GUI::Font& font,
        int x, int y, Controller& controller)
{
  switch(controller.type())
  {
    case Controller::Joystick:
      return new JoystickWidget(boss, font, x, y, controller);
    case Controller::Paddles:
      return new PaddleWidget(boss, font, x, y, controller);
    case Controller::BoosterGrip:
      return new BoosterWidget(boss, font, x, y, controller);
    default:
      return new NullControlWidget(boss, font, x, y, controller);
  }
}
