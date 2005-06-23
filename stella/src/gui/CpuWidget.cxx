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
// $Id: CpuWidget.cxx,v 1.4 2005-06-23 14:33:11 stephena Exp $
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

#include "CpuWidget.hxx"

enum {
  kPCRegAddr,
  kSPRegAddr,
  kARegAddr,
  kXRegAddr,
  kYRegAddr,
  kNumRegs
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CpuWidget::CpuWidget(GuiObject* boss, int x, int y, int w, int h)
  : Widget(boss, x, y, w, h),
    CommandSender(boss)
{
  int xpos   = 10;
  int ypos   = 10;
  int lwidth = 20;
//  const int vWidth = _w - kButtonWidth - 20, space = 6, buttonw = 24;
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



/*
  // Add some buttons for common actions
  ButtonWidget* b;
  xpos = vWidth + 10;  ypos = 20;
  b = new ButtonWidget(boss, xpos, ypos, buttonw, 16, "0", kRZeroCmd, 0);
  b->setTarget(this);

  ypos += 16 + space;
  b = new ButtonWidget(boss, xpos, ypos, buttonw, 16, "Inv", kRInvertCmd, 0);
  b->setTarget(this);

  ypos += 16 + space;
  b = new ButtonWidget(boss, xpos, ypos, buttonw, 16, "++", kRIncCmd, 0);
  b->setTarget(this);

  ypos += 16 + space;
  b = new ButtonWidget(boss, xpos, ypos, buttonw, 16, "<<", kRShiftLCmd, 0);
  b->setTarget(this);

  xpos = vWidth + 30 + 10;  ypos = 20;
//  b = new ButtonWidget(boss, xpos, ypos, buttonw, 16, "", kRCmd, 0);
//  b->setTarget(this);

  ypos += 16 + space;
  b = new ButtonWidget(boss, xpos, ypos, buttonw, 16, "Neg", kRNegateCmd, 0);
  b->setTarget(this);

  ypos += 16 + space;
  b = new ButtonWidget(boss, xpos, ypos, buttonw, 16, "--", kRDecCmd, 0);
  b->setTarget(this);

  ypos += 16 + space;
  b = new ButtonWidget(boss, xpos, ypos, buttonw, 16, ">>", kRShiftRCmd, 0);
  b->setTarget(this);
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CpuWidget::~CpuWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuWidget::handleCommand(CommandSender* sender, int cmd, int data)
{
  int addr, value;
//  unsigned char byte;

  switch(cmd)
  {
    case kDGItemDataChangedCmd:
      addr  = myCpuGrid->getSelectedAddr();
      value = myCpuGrid->getSelectedValue();

      switch(addr)
      {
        case kPCRegAddr:
        instance()->debugger().setPC(value);
        break;

        case kSPRegAddr:
        instance()->debugger().setS(value);
        break;

        case kARegAddr:
        instance()->debugger().setA(value);
        break;

        case kXRegAddr:
        instance()->debugger().setX(value);
        break;

        case kYRegAddr:
        instance()->debugger().setY(value);
        break;
      }
      break;

/*
    case kRZeroCmd:
      myRamGrid->setSelectedValue(0);
      break;

    case kRInvertCmd:
      byte = (unsigned char) myRamGrid->getSelectedValue();
      byte = ~byte;
      myRamGrid->setSelectedValue((int)byte);
      break;

    case kRNegateCmd:
      byte = (unsigned char) myRamGrid->getSelectedValue();
      byte = (~byte) + 1;
      myRamGrid->setSelectedValue((int)byte);
      break;

    case kRIncCmd:
      byte = (unsigned char) myRamGrid->getSelectedValue();
      byte += 1;
      myRamGrid->setSelectedValue((int)byte);
      break;

    case kRDecCmd:
      byte = (unsigned char) myRamGrid->getSelectedValue();
      byte -= 1;
      myRamGrid->setSelectedValue((int)byte);
      break;

    case kRShiftLCmd:
      byte = (unsigned char) myRamGrid->getSelectedValue();
      byte <<= 1;
      myRamGrid->setSelectedValue((int)byte);
      break;

    case kRShiftRCmd:
      byte = (unsigned char) myRamGrid->getSelectedValue();
      byte >>= 1;
      myRamGrid->setSelectedValue((int)byte);
      break;
*/
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
  vlist.push_back(dbg.getS());
  vlist.push_back(dbg.getA());
  vlist.push_back(dbg.getX());
  vlist.push_back(dbg.getY());

  myCpuGrid->setList(alist, vlist);

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
