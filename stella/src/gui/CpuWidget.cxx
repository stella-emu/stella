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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: CpuWidget.cxx,v 1.10 2005-07-05 15:25:44 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <sstream>

#include "OSystem.hxx"
#include "GuiUtils.hxx"
#include "GuiObject.hxx"
#include "Debugger.hxx"
#include "Widget.hxx"
#include "DataGridWidget.hxx"
#include "EditTextWidget.hxx"
#include "ToggleBitWidget.hxx"

#include "CpuWidget.hxx"

enum {
  kPCRegAddr,
  kSPRegAddr,
  kARegAddr,
  kXRegAddr,
  kYRegAddr,
  kNumRegs
};

enum {
  kPSRegN = 0,
  kPSRegV = 1,
  kPSRegB = 3,
  kPSRegD = 4,
  kPSRegI = 5,
  kPSRegZ = 6,
  kPSRegC = 7
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CpuWidget::CpuWidget(GuiObject* boss, int x, int y, int w, int h)
  : Widget(boss, x, y, w, h),
    CommandSender(boss)
{
  int xpos   = 10;
  int ypos   = 10;
  int lwidth = 20;
  const GUI::Font& font = instance()->consoleFont();

  // Create a 1x5 grid with labels for the CPU registers
  myCpuGrid = new DataGridWidget(boss, xpos+lwidth + 5, ypos, 1, 5, 8, 0xffff);
  myCpuGrid->setTarget(this);
  myActiveWidget = myCpuGrid;

  string labels[5] = { "PC", "SP", "A", "X", "Y" };
  for(int row = 0; row < 5; ++row)
  {
    StaticTextWidget* t = new StaticTextWidget(boss, xpos, ypos + row*kLineHeight + 2,
                          lwidth, kLineHeight,
                          labels[row] + string(":"),
                          kTextAlignLeft);
    t->setFont(font);
  }

  // Create a read-only textbox containing the current PC label
  xpos += lwidth + myCpuGrid->colWidth() + 10;  ypos = 10;
  myPCLabel = new EditTextWidget(boss, xpos, ypos, 100, kLineHeight, "");
  myPCLabel->clearFlags(WIDGET_TAB_NAVIGATE);
  myPCLabel->setFont(font);
  myPCLabel->setEditable(false);

  // Create a bitfield widget for changing the processor status
  xpos = 10;  ypos = 2 + 6*kLineHeight;
  StaticTextWidget* t = new StaticTextWidget(boss, xpos, ypos, lwidth, kLineHeight,
                                             "PS:", kTextAlignLeft);
  t->setFont(font);
  myPSRegister = new ToggleBitWidget(boss, xpos+lwidth + 5, ypos-2, 8, 1);
  myPSRegister->setTarget(this);

  // And some status fields
  xpos = 10;  ypos = 10 + 8*kLineHeight;
  new StaticTextWidget(boss, xpos, ypos, 55, kLineHeight, "Current Ins:", kTextAlignLeft);
  xpos += 60;
  myCurrentIns = new EditTextWidget(boss, xpos, ypos-2, 300, kLineHeight, "");
  myCurrentIns->clearFlags(WIDGET_TAB_NAVIGATE);
  myCurrentIns->setFont(font);
  myCurrentIns->setEditable(false);

  xpos = 10;  ypos += kLineHeight + 5;
  new StaticTextWidget(boss, xpos, ypos, 55, kLineHeight, "Cycle Count:", kTextAlignLeft);
  xpos += 60;
  myCycleCount = new EditTextWidget(boss, xpos, ypos-2, 50, kLineHeight, "");
  myCycleCount->clearFlags(WIDGET_TAB_NAVIGATE);
  myCycleCount->setFont(font);
  myCycleCount->setEditable(false);

  xpos = 10;  ypos += kLineHeight + 5;
  new StaticTextWidget(boss, xpos, ypos, 55, kLineHeight, "BP/Trap:", kTextAlignLeft);
  xpos += 60;
  myStatus = new EditTextWidget(boss, xpos, ypos-2, 100, kLineHeight, "");
  myStatus->clearFlags(WIDGET_TAB_NAVIGATE);
  myStatus->setFont(font);
  myStatus->setEditable(false);

  // Set the strings to be used in the PSRegister
  // We only do this once because it's the state that changes, not the strings
  const char* offstr[] = { "n", "v", "-", "b", "d", "i", "z", "c" };
  const char* onstr[]  = { "N", "V", "-", "B", "D", "I", "Z", "C" };
  StringList off, on;
  for(int i = 0; i < 8; ++i)
  {
    off.push_back(offstr[i]);
    on.push_back(onstr[i]);
  }
  myPSRegister->setList(off, on);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CpuWidget::~CpuWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  int addr, value;
  Debugger& dbg = instance()->debugger();

  switch(cmd)
  {
    case kDGItemDataChangedCmd:
      addr  = myCpuGrid->getSelectedAddr();
      value = myCpuGrid->getSelectedValue();

      switch(addr)
      {
        case kPCRegAddr:
        dbg.setPC(value);
        break;

        case kSPRegAddr:
        dbg.setSP(value);
        break;

        case kARegAddr:
        dbg.setA(value);
        break;

        case kXRegAddr:
        dbg.setX(value);
        break;

        case kYRegAddr:
        dbg.setY(value);
        break;
      }
      break;

    case kTBItemDataChangedCmd:
    {
      bool state = myPSRegister->getSelectedState();

      switch(data)
      {
        case kPSRegN:
          dbg.setN(state);
          break;

        case kPSRegV:
          dbg.setV(state);
          break;

        case kPSRegB:
          dbg.setB(state);
          break;

        case kPSRegD:
          dbg.setD(state);
          break;

        case kPSRegI:
          dbg.setI(state);
          break;

        case kPSRegZ:
          dbg.setZ(state);
          break;

        case kPSRegC:
          dbg.setC(state);
          break;
      }
    }
  }

  // TODO - dirty rect, or is it necessary here?
  instance()->frameBuffer().refreshOverlay();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuWidget::loadConfig()
{
  fillGrid();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuWidget::fillGrid()
{
  AddrList alist;
  ValueList vlist;
  BoolArray changed;

  // We push the enumerated items as addresses, and deal with the real
  // address in the callback (handleCommand)
  alist.push_back(kPCRegAddr);
  alist.push_back(kSPRegAddr);
  alist.push_back(kARegAddr);
  alist.push_back(kXRegAddr);
  alist.push_back(kYRegAddr);

  // And now fill the values
  Debugger& dbg = instance()->debugger();
  vlist.push_back(dbg.getPC());
  vlist.push_back(dbg.getSP());
  vlist.push_back(dbg.getA());
  vlist.push_back(dbg.getX());
  vlist.push_back(dbg.getY());

  for(int i = 0; i < 6; ++i)  // FIXME - track changes in registers
    changed.push_back(false);
  myCpuGrid->setList(alist, vlist, changed);

  // Update the PS register booleans
  BoolArray b;
  int ps = dbg.getPS();
  for(int i = 0; i < 8; ++i)
  {
    if(ps & (1<<(7-i)))
      b.push_back(true);
    else
      b.push_back(false);
  }
  myPSRegister->setState(b);

  // Update the other status fields
  int pc = dbg.getPC();
  const char* buf;

  buf = dbg.equates()->getLabel(pc);
  if(buf)
    myPCLabel->setEditString(buf);
  else
    myPCLabel->setEditString("");

  myPCLabel->setEditString("");

  myCurrentIns->setEditString(dbg.disassemble(pc, 1));

  myCycleCount->setEditString(dbg.valueToString(dbg.cycles(), kBASE_10));

  string status;
  if(dbg.breakpoints()->isSet(pc))
    status = "BP set";

  // FIXME - add trap info
  myStatus->setEditString(status);
}
