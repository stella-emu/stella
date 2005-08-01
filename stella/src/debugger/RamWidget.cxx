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
// $Id: RamWidget.cxx,v 1.1 2005-08-01 22:33:12 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <sstream>

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "GuiUtils.hxx"
#include "GuiObject.hxx"
#include "Widget.hxx"
#include "EditTextWidget.hxx"
#include "DataGridWidget.hxx"
#include "RamDebug.hxx"

#include "RamWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RamWidget::RamWidget(GuiObject* boss, int x, int y, int w, int h)
  : Widget(boss, x, y, w, h),
    CommandSender(boss)
{
  int xpos = 10, ypos = 20, lwidth = 4 * kCFontWidth;
  const int vWidth = _w - kButtonWidth - 20, space = 6, buttonw = 24;
  const GUI::Font& font = instance()->consoleFont();
  StaticTextWidget* t;

  // Create a 16x8 grid holding byte values (16 x 8 = 128 RAM bytes) with labels
  myRamGrid = new DataGridWidget(boss, xpos + lwidth, ypos, 16, 8, 2, 8, kBASE_16);
  myRamGrid->setTarget(this);
  myRamGrid->clearFlags(WIDGET_TAB_NAVIGATE);
  myActiveWidget = myRamGrid;

  for(int row = 0; row < 8; ++row)
  {
    t = new StaticTextWidget(boss, xpos, ypos + row*kLineHeight + 2,
                             lwidth, kCFontHeight,
                             Debugger::to_hex_8(row*16 + kRamStart) + string(":"),
                             kTextAlignLeft);
    t->setFont(font);
  }
  for(int col = 0; col < 16; ++col)
  {
    t = new StaticTextWidget(boss, xpos + col*myRamGrid->colWidth() + lwidth + 7,
                             ypos - kLineHeight,
                             kCFontWidth, kCFontHeight,
                             Debugger::to_hex_4(col),
                             kTextAlignLeft);
    t->setFont(font);
  }
  
  xpos = 20;  ypos = 11 * kLineHeight;
  t = new StaticTextWidget(boss, xpos, ypos,
                           6*kCFontWidth, kCFontHeight,
                           "Label:", kTextAlignLeft);
  t->setFont(font);
  xpos += 6*kCFontWidth + 5;
  myLabel = new EditTextWidget(boss, xpos, ypos-2, 15*kCFontWidth, kLineHeight, "");
  myLabel->clearFlags(WIDGET_TAB_NAVIGATE);
  myLabel->setFont(font);
  myLabel->setEditable(false);

  xpos += 15*kCFontWidth + 20;
  t = new StaticTextWidget(boss, xpos, ypos,
                           4*kCFontWidth, kCFontHeight,
                           "Dec:", kTextAlignLeft);
  t->setFont(font);
  xpos += 4*kCFontWidth + 5;
  myDecValue = new EditTextWidget(boss, xpos, ypos-2, 4*kCFontWidth, kLineHeight, "");
  myDecValue->clearFlags(WIDGET_TAB_NAVIGATE);
  myDecValue->setFont(font);
  myDecValue->setEditable(false);

  xpos += 4*kCFontWidth + 20;
  t = new StaticTextWidget(boss, xpos, ypos,
                           4*kCFontWidth, kCFontHeight,
                           "Bin:", kTextAlignLeft);
  t->setFont(font);
  xpos += 4*kCFontWidth + 5;
  myBinValue = new EditTextWidget(boss, xpos, ypos-2, 9*kCFontWidth, kLineHeight, "");
  myBinValue->clearFlags(WIDGET_TAB_NAVIGATE);
  myBinValue->setFont(font);
  myBinValue->setEditable(false);

  // Add some buttons for common actions
  ButtonWidget* b;
  xpos = vWidth + 10;  ypos = 20;
  b = new ButtonWidget(boss, xpos, ypos, buttonw, 16, "0", kDGZeroCmd, 0);
  b->setTarget(myRamGrid);

  ypos += 16 + space;
  b = new ButtonWidget(boss, xpos, ypos, buttonw, 16, "Inv", kDGInvertCmd, 0);
  b->setTarget(myRamGrid);

  ypos += 16 + space;
  b = new ButtonWidget(boss, xpos, ypos, buttonw, 16, "++", kDGIncCmd, 0);
  b->setTarget(myRamGrid);

  ypos += 16 + space;
  b = new ButtonWidget(boss, xpos, ypos, buttonw, 16, "<<", kDGShiftLCmd, 0);
  b->setTarget(myRamGrid);

  ypos += 16 + space;
  // keep a pointer to this one, it gets disabled/enabled
  myUndoButton = b = new ButtonWidget(boss, xpos, ypos, buttonw*2+10, 16, "Undo", kUndoCmd, 0);
  b->setTarget(this);

  ypos += 16 + space;
  // keep a pointer to this one, it gets disabled/enabled
  myRevertButton = b = new ButtonWidget(boss, xpos, ypos, buttonw*2+10, 16, "Revert", kRevertCmd, 0);
  b->setTarget(this);

  xpos = vWidth + 30 + 10;  ypos = 20;
//  b = new ButtonWidget(boss, xpos, ypos, buttonw, 16, "", kRCmd, 0);
//  b->setTarget(this);

  ypos += 16 + space;
  b = new ButtonWidget(boss, xpos, ypos, buttonw, 16, "Neg", kDGNegateCmd, 0);
  b->setTarget(myRamGrid);

  ypos += 16 + space;
  b = new ButtonWidget(boss, xpos, ypos, buttonw, 16, "--", kDGDecCmd, 0);
  b->setTarget(myRamGrid);

  ypos += 16 + space;
  b = new ButtonWidget(boss, xpos, ypos, buttonw, 16, ">>", kDGShiftRCmd, 0);
  b->setTarget(myRamGrid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RamWidget::~RamWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  // We simply change the values in the ByteGridWidget
  // It will then send the 'kDGItemDataChangedCmd' signal to change the actual
  // memory location
  int addr, value;
  const char* buf;

  RamDebug& dbg = instance()->debugger().ramDebug();
  switch(cmd)
  {
    case kDGItemDataChangedCmd:
      addr  = myRamGrid->getSelectedAddr();
      value = myRamGrid->getSelectedValue();

      myUndoAddress = addr;
      myUndoValue = dbg.read(addr);

      dbg.write(addr, value);
      myDecValue->setEditString(instance()->debugger().valueToString(value, kBASE_10));
      myBinValue->setEditString(instance()->debugger().valueToString(value, kBASE_2));
      myRevertButton->setEnabled(true);
      myUndoButton->setEnabled(true);
      break;

    case kDGSelectionChangedCmd:
      addr  = myRamGrid->getSelectedAddr();
      value = myRamGrid->getSelectedValue();

      buf = instance()->debugger().equates()->getLabel(addr+kRamStart).c_str();
      if(*buf) myLabel->setEditString(buf);
      else    myLabel->setEditString("");

      myDecValue->setEditString(instance()->debugger().valueToString(value, kBASE_10));
      myBinValue->setEditString(instance()->debugger().valueToString(value, kBASE_2));
      break;

    case kRevertCmd:
      for(unsigned int i = 0; i < kRamSize; i++)
        dbg.write(i, _oldValueList[i]);
      fillGrid(true);
      break;

    case kUndoCmd:
      dbg.write(myUndoAddress, myUndoValue);
      myUndoButton->setEnabled(false);
      fillGrid(false);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::loadConfig()
{
cerr << "RamWidget::loadConfig()\n";
  fillGrid(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::fillGrid(bool updateOld)
{
  IntArray alist;
  IntArray vlist;
  BoolArray changed;

  if(updateOld) _oldValueList.clear();

  RamDebug& dbg = instance()->debugger().ramDebug();

  RamState state    = (RamState&) dbg.getState();
  RamState oldstate = (RamState&) dbg.getOldState();

  vlist = state.ram;
  if(updateOld) _oldValueList = state.ram;

  for(unsigned int i = 0; i < 16*8; i++)
  {
    alist.push_back(i);
    changed.push_back(state.ram[i] != oldstate.ram[i]);
  }

  myRamGrid->setList(alist, vlist, changed);
  if(updateOld)
  {
    myRevertButton->setEnabled(false);
    myUndoButton->setEnabled(false);
  }
}
