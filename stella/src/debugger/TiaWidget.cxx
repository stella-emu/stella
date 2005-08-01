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
// $Id: TiaWidget.cxx,v 1.1 2005-08-01 22:33:12 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "GuiUtils.hxx"
#include "GuiObject.hxx"
#include "TIADebug.hxx"
#include "Widget.hxx"
#include "EditTextWidget.hxx"
#include "DataGridWidget.hxx"
#include "ColorWidget.hxx"
#include "TiaWidget.hxx"

// ID's for the various widgets
// We need ID's, since there are more than one of several types of widgets
enum {
  kRamID       = 'TWra',
  kColorRegsID = 'TWcr',
  kVSyncID     = 'TWvs',
  kVBlankID    = 'TWvb'
};

// Color registers
enum {
  kCOLUP0Addr,
  kCOLUP1Addr,
  kCOLUPFAddr,
  kCOLUBKAddr
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaWidget::TiaWidget(GuiObject* boss, int x, int y, int w, int h)
  : Widget(boss, x, y, w, h),
    CommandSender(boss)
{
  int xpos = 10, ypos = 20, lwidth = 4 * kCFontWidth;
//  const int vWidth = _w - kButtonWidth - 20, space = 6, buttonw = 24;
  const GUI::Font& font = instance()->consoleFont();

  // Create a 16x1 grid holding byte values with labels
  myRamGrid = new DataGridWidget(boss, xpos + lwidth, ypos, 16, 1, 2, 8, kBASE_16);
  myRamGrid->setEditable(false);
  myRamGrid->setTarget(this);
  myRamGrid->setID(kRamID);
  myActiveWidget = myRamGrid;

  StaticTextWidget* t = new StaticTextWidget(boss, xpos, ypos + 2,
                        lwidth, kCFontHeight,
                        Debugger::to_hex_8(0) + string(":"),
                        kTextAlignLeft);
  t->setFont(font);
  for(int col = 0; col < 16; ++col)
  {
    t = new StaticTextWidget(boss, xpos + col*myRamGrid->colWidth() + lwidth + 7,
                             ypos - kLineHeight,
                             kCFontWidth, kCFontHeight,
                             Debugger::to_hex_4(col),
                             kTextAlignLeft);
    t->setFont(font);
  }
  
  xpos = 20;  ypos = 4 * kLineHeight;
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

  // Color registers
  const char* regNames[] = { "COLUP0:", "COLUP1:", "COLUPF:", "COLUBK:" };
  xpos = 10;  ypos += 2* kLineHeight;
  for(int row = 0; row < 4; ++row)
  {
    t = new StaticTextWidget(boss, xpos, ypos + row*kLineHeight + 2,
                             7*kCFontWidth, kCFontHeight,
                             regNames[row],
                             kTextAlignLeft);
    t->setFont(font);
  }
  xpos += 7*kCFontWidth + 5;
  myColorRegs = new DataGridWidget(boss, xpos, ypos, 1, 4, 2, 8, kBASE_16);
  myColorRegs->setTarget(this);
  myColorRegs->setID(kColorRegsID);

  xpos += myColorRegs->colWidth() + 5;
  myCOLUP0Color = new ColorWidget(boss, xpos, ypos+2, 20, kLineHeight - 4);
  myCOLUP0Color->setTarget(this);

  ypos += kLineHeight;
  myCOLUP1Color = new ColorWidget(boss, xpos, ypos+2, 20, kLineHeight - 4);
  myCOLUP1Color->setTarget(this);

  ypos += kLineHeight;
  myCOLUPFColor = new ColorWidget(boss, xpos, ypos+2, 20, kLineHeight - 4);
  myCOLUPFColor->setTarget(this);

  ypos += kLineHeight;
  myCOLUBKColor = new ColorWidget(boss, xpos, ypos+2, 20, kLineHeight - 4);
  myCOLUBKColor->setTarget(this);

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
  string buf;

  Debugger& dbg = instance()->debugger();

  switch(cmd)
  {
    case kDGItemDataChangedCmd:
      switch(id)
      {
        case kColorRegsID:
          changeColorRegs();
          break;

        default:
          cerr << "TiaWidget DG changed\n";
          break;
      }
      // FIXME -  maybe issue a full reload, since changing one item can affect
      //          others in this tab??
      // loadConfig();
      break;

    case kDGSelectionChangedCmd:
      switch(id)
      {
        case kRamID:
          addr  = myRamGrid->getSelectedAddr();
          value = myRamGrid->getSelectedValue();

          myLabel->setEditString(dbg.equates()->getLabel(addr));

          myDecValue->setEditString(dbg.valueToString(value, kBASE_10));
          myBinValue->setEditString(dbg.valueToString(value, kBASE_2));
          break;
      }
      break;

    case kCheckActionCmd:
      switch(id)
      {
        case kVSyncID:
          cerr << "vsync toggled\n";
          break;

        case kVBlankID:
          cerr << "vblank toggled\n";
          break;
      }
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaWidget::loadConfig()
{
cerr << "TiaWidget::loadConfig()\n";
  fillGrid();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaWidget::fillGrid()
{
  IntArray alist;
  IntArray vlist;
  BoolArray changed;

  Debugger& dbg = instance()->debugger();
  TIADebug& tia = dbg.tiaDebug();
  TiaState state    = (TiaState&) tia.getState();
  TiaState oldstate = (TiaState&) tia.getOldState();

  // TIA RAM
  alist.clear();  vlist.clear();  changed.clear();
  for(unsigned int i = 0; i < 16; i++)
  {
    alist.push_back(i);
    vlist.push_back(state.ram[i]);
    changed.push_back(state.ram[i] != oldstate.ram[i]);
  }
  myRamGrid->setList(alist, vlist, changed);

  // Color registers
  alist.clear();  vlist.clear();  changed.clear();
  for(unsigned int i = 0; i < 4; i++)
  {
    alist.push_back(i);
    vlist.push_back(state.coluRegs[i]);
    changed.push_back(state.coluRegs[i] != oldstate.coluRegs[i]);
  }
  myColorRegs->setList(alist, vlist, changed);

  myCOLUP0Color->setColor(state.coluRegs[0]);
  myCOLUP1Color->setColor(state.coluRegs[1]);
  myCOLUPFColor->setColor(state.coluRegs[2]);
  myCOLUBKColor->setColor(state.coluRegs[3]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaWidget::changeColorRegs()
{
  int addr  = myColorRegs->getSelectedAddr();
  int value = myColorRegs->getSelectedValue();

  switch(addr)
  {
    case kCOLUP0Addr:
      instance()->debugger().tiaDebug().coluP0(value);
      myCOLUP0Color->setColor(value);
      break;

    case kCOLUP1Addr:
      instance()->debugger().tiaDebug().coluP1(value);
      myCOLUP1Color->setColor(value);
      break;

    case kCOLUPFAddr:
      instance()->debugger().tiaDebug().coluPF(value);
      myCOLUPFColor->setColor(value);
      break;

    case kCOLUBKAddr:
      instance()->debugger().tiaDebug().coluBK(value);
      myCOLUBKColor->setColor(value);
      break;
  }
}
