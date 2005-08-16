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
// $Id: TiaWidget.cxx,v 1.6 2005-08-16 18:34:12 stephena Exp $
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
#include "ToggleBitWidget.hxx"
#include "TiaWidget.hxx"

// ID's for the various widgets
// We need ID's, since there are more than one of several types of widgets
enum {
  kRamID,
  kColorRegsID,
  kGRP0ID,
  kGRP1ID,
  kPosP0ID,
  kPosP1ID,
  kHMP0ID,
  kHMP1ID,
  kRefP0ID,
  kRefP1ID,
  kDelP0ID,
  kDelP1ID,
  kNusizP0ID,
  kNusizP1ID
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
  const GUI::Font& font = instance()->consoleFont();
  const int fontWidth  = font.getMaxCharWidth(),
            fontHeight = font.getFontHeight(),
            lineHeight = font.getLineHeight();
  int xpos = 10, ypos = 25, lwidth = 4 * font.getMaxCharWidth();
  StaticTextWidget* t;

  // Create a 16x1 grid holding byte values with labels
  myRamGrid = new DataGridWidget(boss, font, xpos + lwidth, ypos,
                                 16, 1, 2, 8, kBASE_16);
  myRamGrid->setEditable(false);
  myRamGrid->setTarget(this);
  myRamGrid->setID(kRamID);
  addFocusWidget(myRamGrid);

  t = new StaticTextWidget(boss, xpos, ypos + 2,
                           lwidth-2, fontHeight,
                           "00:", kTextAlignLeft);
  t->setFont(font);
  for(int col = 0; col < 16; ++col)
  {
    t = new StaticTextWidget(boss, xpos + col*myRamGrid->colWidth() + lwidth + 7,
                             ypos - lineHeight,
                             fontWidth, fontHeight,
                             Debugger::to_hex_4(col),
                             kTextAlignLeft);
    t->setFont(font);
  }
  
  xpos = 20;  ypos += 2 * lineHeight;
  t = new StaticTextWidget(boss, xpos, ypos,
                           6*fontWidth, fontHeight,
                           "Label:", kTextAlignLeft);
  t->setFont(font);
  xpos += 6*fontWidth + 5;
  myLabel = new EditTextWidget(boss, xpos, ypos-2, 15*fontWidth, lineHeight, "");
  myLabel->setFont(font);
  myLabel->setEditable(false);

  xpos += 15*fontWidth + 20;
  t = new StaticTextWidget(boss, xpos, ypos,
                           4*fontWidth, fontHeight,
                           "Dec:", kTextAlignLeft);
  t->setFont(font);
  xpos += 4*fontWidth + 5;
  myDecValue = new EditTextWidget(boss, xpos, ypos-2, 4*fontWidth, lineHeight, "");
  myDecValue->setFont(font);
  myDecValue->setEditable(false);

  xpos += 4*fontWidth + 20;
  t = new StaticTextWidget(boss, xpos, ypos,
                           4*fontWidth, fontHeight,
                           "Bin:", kTextAlignLeft);
  t->setFont(font);
  xpos += 4*fontWidth + 5;
  myBinValue = new EditTextWidget(boss, xpos, ypos-2, 9*fontWidth, lineHeight, "");
  myBinValue->setFont(font);
  myBinValue->setEditable(false);

  // Color registers
  const char* regNames[] = { "COLUP0:", "COLUP1:", "COLUPF:", "COLUBK:" };
  xpos = 10;  ypos += 2*lineHeight;
  for(int row = 0; row < 4; ++row)
  {
    t = new StaticTextWidget(boss, xpos, ypos + row*lineHeight + 2,
                             7*fontWidth, fontHeight,
                             regNames[row],
                             kTextAlignLeft);
    t->setFont(font);
  }
  xpos += 7*fontWidth + 5;
  myColorRegs = new DataGridWidget(boss, font, xpos, ypos,
                                   1, 4, 2, 8, kBASE_16);
  myColorRegs->setTarget(this);
  myColorRegs->setID(kColorRegsID);
  addFocusWidget(myColorRegs);

  xpos += myColorRegs->colWidth() + 5;
  myCOLUP0Color = new ColorWidget(boss, xpos, ypos+2, 20, lineHeight - 4);
  myCOLUP0Color->setTarget(this);

  ypos += lineHeight;
  myCOLUP1Color = new ColorWidget(boss, xpos, ypos+2, 20, lineHeight - 4);
  myCOLUP1Color->setTarget(this);

  ypos += lineHeight;
  myCOLUPFColor = new ColorWidget(boss, xpos, ypos+2, 20, lineHeight - 4);
  myCOLUPFColor->setTarget(this);

  ypos += lineHeight;
  myCOLUBKColor = new ColorWidget(boss, xpos, ypos+2, 20, lineHeight - 4);
  myCOLUBKColor->setTarget(this);

  // Set the strings to be used in the grPx registers
  // We only do this once because it's the state that changes, not the strings
  const char* offstr[] = { "0", "0", "0", "0", "0", "0", "0", "0" };
  const char* onstr[]  = { "1", "1", "1", "1", "1", "1", "1", "1" };
  StringList off, on;
  for(int i = 0; i < 8; ++i)
  {
    off.push_back(offstr[i]);
    on.push_back(onstr[i]);
  }

  ////////////////////////////
  // P0 register info
  ////////////////////////////
  // grP0
  xpos = 10;  ypos += 2*lineHeight;
  t = new StaticTextWidget(boss, xpos, ypos+2,
                           7*fontWidth, fontHeight,
                           "P0/ GR:", kTextAlignLeft);
  t->setFont(font);
  xpos += 7*fontWidth + 5;
  myGRP0 = new ToggleBitWidget(boss, font, xpos, ypos, 8, 1);
  myGRP0->setList(off, on);
  myGRP0->setTarget(this);
  myGRP0->setID(kGRP0ID);
  addFocusWidget(myGRP0);

  // posP0
  xpos += myGRP0->getWidth() + 8;
  t = new StaticTextWidget(boss, xpos, ypos+2,
                           4*fontWidth, fontHeight,
                           "Pos:", kTextAlignLeft);
  t->setFont(font);
  xpos += 4*fontWidth + 5;
  myPosP0 = new DataGridWidget(boss, font, xpos, ypos,
                               1, 1, 2, 8, kBASE_16);
  myPosP0->setTarget(this);
  myPosP0->setID(kPosP0ID);
  addFocusWidget(myPosP0);

  // hmP0
  xpos += myPosP0->getWidth() + 8;
  t = new StaticTextWidget(boss, xpos, ypos+2,
                           3*fontWidth, fontHeight,
                           "HM:", kTextAlignLeft);
  t->setFont(font);
  xpos += 3*fontWidth + 5;
  myHMP0 = new DataGridWidget(boss, font, xpos, ypos,
                              1, 1, 1, 4, kBASE_16_4);
  myHMP0->setTarget(this);
  myHMP0->setID(kHMP0ID);
  addFocusWidget(myHMP0);

  // P0 reflect and delay
  xpos += myHMP0->getWidth() + 10;
  myRefP0 = new CheckboxWidget(boss, font, xpos, ypos+1, "Reflect", kCheckActionCmd);
  myRefP0->setFont(font);
  myRefP0->setTarget(this);
  myRefP0->setID(kRefP0ID);
  addFocusWidget(myRefP0);

  xpos += myRefP0->getWidth() + 10;
  myDelP0 = new CheckboxWidget(boss, font, xpos, ypos+1, "Delay", kCheckActionCmd);
  myDelP0->setFont(font);
  myDelP0->setTarget(this);
  myDelP0->setID(kDelP0ID);
  addFocusWidget(myDelP0);

  // NUSIZ0
  xpos = 10 + lwidth;  ypos += myGRP0->getHeight() + 2;
  t = new StaticTextWidget(boss, xpos, ypos+2,
                           7*fontWidth, fontHeight,
                           "NUSIZ0:", kTextAlignLeft);
  t->setFont(font);
  xpos += 7*fontWidth + 5;
  myNusiz0 = new DataGridWidget(boss, font, xpos, ypos,
                                1, 1, 1, 3, kBASE_16_4);
  myNusiz0->setTarget(this);
  myNusiz0->setID(kNusizP0ID);
  addFocusWidget(myNusiz0);

  xpos += myNusiz0->getWidth() + 5;
  myNusiz0Text = new EditTextWidget(boss, xpos, ypos+1, 23*fontWidth, lineHeight, "");
  myNusiz0Text->setFont(font);
  myNusiz0Text->setEditable(false);

  ////////////////////////////
  // P1 register info
  ////////////////////////////
  // grP1
  xpos = 10;  ypos += 2*lineHeight;
  t = new StaticTextWidget(boss, xpos, ypos+2,
                           7*fontWidth, fontHeight,
                           "P1/ GR:", kTextAlignLeft);
  t->setFont(font);
  xpos += 7*fontWidth + 5;
  myGRP1 = new ToggleBitWidget(boss, font, xpos, ypos, 8, 1);
  myGRP1->setList(off, on);
  myGRP1->setTarget(this);
  myGRP1->setID(kGRP1ID);
  addFocusWidget(myGRP1);

  // posP1
  xpos += myGRP1->getWidth() + 8;
  t = new StaticTextWidget(boss, xpos, ypos+2,
                           4*fontWidth, fontHeight,
                           "Pos:", kTextAlignLeft);
  t->setFont(font);
  xpos += 4*fontWidth + 5;
  myPosP1 = new DataGridWidget(boss, font, xpos, ypos,
                               1, 1, 2, 8, kBASE_16);
  myPosP1->setTarget(this);
  myPosP1->setID(kPosP1ID);
  addFocusWidget(myPosP1);

  // hmP1
  xpos += myPosP1->getWidth() + 8;
  t = new StaticTextWidget(boss, xpos, ypos+2,
                           3*fontWidth, fontHeight,
                           "HM:", kTextAlignLeft);
  t->setFont(font);
  xpos += 3*fontWidth + 5;
  myHMP1 = new DataGridWidget(boss, font, xpos, ypos,
                              1, 1, 1, 4, kBASE_16_4);
  myHMP1->setTarget(this);
  myHMP1->setID(kHMP1ID);
  addFocusWidget(myHMP1);

  // P1 reflect and delay
  xpos += myHMP1->getWidth() + 10;
  myRefP1 = new CheckboxWidget(boss, font, xpos, ypos+1, "Reflect", kCheckActionCmd);
  myRefP1->setFont(font);
  myRefP1->setTarget(this);
  myRefP1->setID(kRefP1ID);
  addFocusWidget(myRefP1);

  xpos += myRefP1->getWidth() + 10;
  myDelP1 = new CheckboxWidget(boss, font, xpos, ypos+1, "Delay", kCheckActionCmd);
  myDelP1->setFont(font);
  myDelP1->setTarget(this);
  myDelP1->setID(kDelP1ID);
  addFocusWidget(myDelP1);

  // NUSIZ1
  xpos = 10 + lwidth;  ypos += myGRP1->getHeight() + 2;
  t = new StaticTextWidget(boss, xpos, ypos+2,
                           7*fontWidth, fontHeight,
                           "NUSIZ1:", kTextAlignLeft);
  t->setFont(font);
  xpos += 7*fontWidth + 5;
  myNusiz1 = new DataGridWidget(boss, font, xpos, ypos,
                                1, 1, 1, 3, kBASE_16_4);
  myNusiz1->setTarget(this);
  myNusiz1->setID(kNusizP1ID);
  addFocusWidget(myNusiz1);

  xpos += myNusiz1->getWidth() + 5;
  myNusiz1Text = new EditTextWidget(boss, xpos, ypos+1, 23*fontWidth, lineHeight, "");
  myNusiz1Text->setFont(font);
  myNusiz1Text->setEditable(false);


}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaWidget::~TiaWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  // We simply change the values in the DataGridWidget
  // It will then send the 'kDGItemDataChangedCmd' signal to change the actual
  // memory location
  int addr, value;
  string buf;

  Debugger& dbg = instance()->debugger();
  TIADebug& tia = dbg.tiaDebug();

  switch(cmd)
  {
    case kDGItemDataChangedCmd:
      switch(id)
      {
        case kColorRegsID:
          changeColorRegs();
          break;

        case kPosP0ID:
          tia.posP0(myPosP0->getSelectedValue());
          break;

        case kPosP1ID:
          tia.posP1(myPosP1->getSelectedValue());
          break;

        case kHMP0ID:
          tia.hmP0(myHMP0->getSelectedValue());
          break;

        case kHMP1ID:
          tia.hmP1(myHMP1->getSelectedValue());
          break;

        case kNusizP0ID:
          tia.nusiz0(myNusiz0->getSelectedValue());
          myNusiz0Text->setEditString(tia.nusiz0String());
          break;

        case kNusizP1ID:
          tia.nusiz1(myNusiz1->getSelectedValue());
          myNusiz1Text->setEditString(tia.nusiz1String());
          break;

        default:
          cerr << "TiaWidget DG changed\n";
          break;
      }
      break;

    case kTBItemDataChangedCmd:
      switch(id)
      {
        case kGRP0ID:
          value = convertBoolToInt(myGRP0->getState());
          tia.grP0(value);
          break;

        case kGRP1ID:
          value = convertBoolToInt(myGRP1->getState());
          tia.grP1(value);
          break;
      }
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
        case kRefP0ID:
          tia.refP0(myRefP0->getState() ? 1 : 0);
          break;

        case kRefP1ID:
          tia.refP1(myRefP1->getState() ? 1 : 0);
          break;

        case kDelP0ID:
          tia.vdelP0(myDelP0->getState() ? 1 : 0);
          break;

        case kDelP1ID:
          tia.vdelP1(myDelP1->getState() ? 1 : 0);
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
  BoolArray orig, changed, grNew, grOld;

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

  ////////////////////////////
  // P0 register info
  ////////////////////////////
  // grP0
  grNew.clear(); grOld.clear();
  convertCharToBool(grNew, state.gr[P0]);
  convertCharToBool(grOld, oldstate.gr[P0]);

  changed.clear();
  for(unsigned int i = 0; i < 8; ++i)
    changed.push_back(grNew[i] != grOld[i]);

  myGRP0->setState(grNew, changed);

  // posP0
  myPosP0->setList(0, state.pos[P0], state.pos[P0] != oldstate.pos[P0]);

  // hmP0 register
  myHMP0->setList(0, state.hm[P0], state.hm[P0] != oldstate.hm[P0]);

  // refP0 & vdelP0
  myRefP0->setState(state.refP0);
  myDelP0->setState(state.vdelP0);

  // NUSIZ0
  myNusiz0->setList(0, state.nusiz[0], state.nusiz[0] != oldstate.nusiz[0]);
  myNusiz0Text->setEditString(tia.nusiz0String());

  ////////////////////////////
  // P1 register info
  ////////////////////////////
  // grP1
  grNew.clear(); grOld.clear();
  convertCharToBool(grNew, state.gr[P1]);
  convertCharToBool(grOld, oldstate.gr[P1]);

  changed.clear();
  for(unsigned int i = 0; i < 8; ++i)
    changed.push_back(grNew[i] != grOld[i]);

  myGRP1->setState(grNew, changed);

  // posP1
  myPosP1->setList(0, state.pos[P1], state.pos[P1] != oldstate.pos[P1]);

  // hmP1 register
  myHMP1->setList(0, state.hm[P1], state.hm[P1] != oldstate.hm[P1]);

  // refP1 & vdelP1
  myRefP1->setState(state.refP1);
  myDelP1->setState(state.vdelP1);

  // NUSIZ1
  myNusiz1->setList(0, state.nusiz[1], state.nusiz[1] != oldstate.nusiz[1]);
  myNusiz1Text->setEditString(tia.nusiz1String());

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaWidget::convertCharToBool(BoolArray& b, unsigned char value)
{
  for(unsigned int i = 0; i < 8; ++i)
  {
    if(value & (1<<(7-i)))
      b.push_back(true);
    else
      b.push_back(false);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TiaWidget::convertBoolToInt(const BoolArray& b)
{
  unsigned int value = 0, size = b.size();

  for(unsigned int i = 0; i < size; ++i)
  {
    if(b[i])
      value |= 1<<(size-i-1);
  }

  return value;
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
