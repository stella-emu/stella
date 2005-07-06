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
// $Id: TiaWidget.cxx,v 1.3 2005-07-06 15:09:16 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "GuiUtils.hxx"
#include "GuiObject.hxx"
#include "Debugger.hxx"
#include "Widget.hxx"
#include "EditTextWidget.hxx"
#include "DataGridWidget.hxx"

#include "TiaWidget.hxx"

// ID's for the various widgets
// We need ID's, since there are more than one of several types of widgets
enum {
  kRamID,
  kCOLUP0ID,
  kCOLUP1ID,
  kCOLUBKID,
  kCOLUPFID
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaWidget::TiaWidget(GuiObject* boss, int x, int y, int w, int h)
  : Widget(boss, x, y, w, h),
    CommandSender(boss)
{
  int xpos   = 10;
  int ypos   = 20;
  int lwidth = 25;
  const int vWidth = _w - kButtonWidth - 20, space = 6, buttonw = 24;
  const GUI::Font& font = instance()->consoleFont();

  // Create a 16x1 grid holding byte values with labels
  myRamGrid = new DataGridWidget(boss, xpos+lwidth, ypos, 16, 1, 2, 8, kBASE_16);
  myRamGrid->setTarget(this);
  myRamGrid->setID(kRamID);
  myActiveWidget = myRamGrid;

  StaticTextWidget* t = new StaticTextWidget(boss, xpos, ypos + 2,
                        lwidth, kLineHeight,
                        Debugger::to_hex_8(0) + string(":"),
                        kTextAlignLeft);
  t->setFont(font);
  for(int col = 0; col < 16; ++col)
  {
    StaticTextWidget* t = new StaticTextWidget(boss,
                          xpos + col*myRamGrid->colWidth() + lwidth + 7,
                          ypos - kLineHeight,
                          lwidth, kLineHeight,
                          Debugger::to_hex_4(col),
                          kTextAlignLeft);
    t->setFont(font);
  }
  
  xpos = 20;  ypos = 4 * kLineHeight;
  new StaticTextWidget(boss, xpos, ypos, 30, kLineHeight, "Label: ", kTextAlignLeft);
  xpos += 30;
  myLabel = new EditTextWidget(boss, xpos, ypos-2, 100, kLineHeight, "");
  myLabel->clearFlags(WIDGET_TAB_NAVIGATE);
  myLabel->setFont(font);
  myLabel->setEditable(false);

  xpos += 120;
  new StaticTextWidget(boss, xpos, ypos, 35, kLineHeight, "Decimal: ", kTextAlignLeft);
  xpos += 35;
  myDecValue = new EditTextWidget(boss, xpos, ypos-2, 30, kLineHeight, "");
  myDecValue->clearFlags(WIDGET_TAB_NAVIGATE);
  myDecValue->setFont(font);
  myDecValue->setEditable(false);

  xpos += 48;
  new StaticTextWidget(boss, xpos, ypos, 35, kLineHeight, "Binary: ", kTextAlignLeft);
  xpos += 35;
  myBinValue = new EditTextWidget(boss, xpos, ypos-2, 60, kLineHeight, "");
  myBinValue->clearFlags(WIDGET_TAB_NAVIGATE);
  myBinValue->setFont(font);
  myBinValue->setEditable(false);

/*
  // Add some buttons for common actions
  ButtonWidget* b;
  xpos = vWidth + 10;  ypos = 20;
  b = new ButtonWidget(boss, xpos, ypos, buttonw, 16, "0", kRZeroCmd, 0);
  b->setTarget(this);


  xpos = vWidth + 30 + 10;  ypos = 20;
//  b = new ButtonWidget(boss, xpos, ypos, buttonw, 16, "", kRCmd, 0);
//  b->setTarget(this);
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaWidget::~TiaWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  // We simply change the values in the ByteGridWidget
  // It will then send the 'kDGItemDataChangedCmd' signal to change the actual
  // memory location
  int addr, value;
  const char* buf;

  Debugger& dbg = instance()->debugger();

  switch(cmd)
  {
    case kDGItemDataChangedCmd:
      switch(id)
      {
        case kRamID:
          changeRam();
          break;

        default:
          cerr << "TiaWidget DG changed\n";
          break;
      }
      // FIXME -  maybe issue a full reload, since changing one item can affect
      //          others in this tab??
      break;

    case kDGSelectionChangedCmd:
      // FIXME - for now, only the RAM has multiple cells, so it's the only one
      // that can receive this signal
      addr  = myRamGrid->getSelectedAddr();
      value = myRamGrid->getSelectedValue();

      buf = instance()->debugger().equates()->getLabel(addr);
      if(buf) myLabel->setEditString(buf);
      else    myLabel->setEditString("");

      myDecValue->setEditString(instance()->debugger().valueToString(value, kBASE_10));
      myBinValue->setEditString(instance()->debugger().valueToString(value, kBASE_2));
      break;
  }

  // TODO - dirty rect, or is it necessary here?
  instance()->frameBuffer().refreshOverlay();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaWidget::loadConfig()
{
  fillGrid();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaWidget::fillGrid()
{
  AddrList alist;
  ValueList vlist;
  BoolArray changed;

// FIXME - have this actually talk to TIADebug
  Debugger& dbg = instance()->debugger();
  for(unsigned int i = 0; i < 16; i++)
  {
    alist.push_back(i);
    vlist.push_back(i);
    changed.push_back(false);
  }
  myRamGrid->setList(alist, vlist, changed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaWidget::changeRam()
{
  int addr  = myRamGrid->getSelectedAddr();
  int value = myRamGrid->getSelectedValue();

//FIXME  instance()->debugger().writeRAM(addr - kRamStart, value);
  myDecValue->setEditString(instance()->debugger().valueToString(value, kBASE_10));
  myBinValue->setEditString(instance()->debugger().valueToString(value, kBASE_2));
}
