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
// $Id: TiaWidget.cxx,v 1.13 2008-05-15 15:07:29 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "ColorWidget.hxx"
#include "DataGridWidget.hxx"
#include "EditTextWidget.hxx"
#include "FrameBuffer.hxx"
#include "GuiObject.hxx"
#include "OSystem.hxx"
#include "TIADebug.hxx"
#include "ToggleBitWidget.hxx"
#include "TogglePixelWidget.hxx"
#include "Widget.hxx"

#include "TiaWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaWidget::TiaWidget(GuiObject* boss, const GUI::Font& font,
                     int x, int y, int w, int h)
  : Widget(boss, font, x, y, w, h),
    CommandSender(boss)
{
  _type = kTiaWidget;

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

  t = new StaticTextWidget(boss, font, xpos, ypos + 2,
                           lwidth-2, fontHeight,
                           "00:", kTextAlignLeft);
  for(int col = 0; col < 16; ++col)
  {
    t = new StaticTextWidget(boss, font, xpos + col*myRamGrid->colWidth() + lwidth + 7,
                             ypos - lineHeight,
                             fontWidth, fontHeight,
                             Debugger::to_hex_4(col),
                             kTextAlignLeft);
  }
  
  xpos = 20;  ypos += 2 * lineHeight;
  t = new StaticTextWidget(boss, font, xpos, ypos, 6*fontWidth, fontHeight,
                           "Label:", kTextAlignLeft);
  xpos += 6*fontWidth + 5;
  myLabel = new EditTextWidget(boss, font, xpos, ypos-2, 15*fontWidth,
                               lineHeight, "");
  myLabel->setEditable(false);

  xpos += 15*fontWidth + 20;
  t = new StaticTextWidget(boss, font, xpos, ypos, 4*fontWidth, fontHeight,
                           "Dec:", kTextAlignLeft);
  xpos += 4*fontWidth + 5;
  myDecValue = new EditTextWidget(boss, font, xpos, ypos-2, 4*fontWidth,
                                  lineHeight, "");
  myDecValue->setEditable(false);

  xpos += 4*fontWidth + 20;
  t = new StaticTextWidget(boss, font, xpos, ypos, 4*fontWidth, fontHeight,
                           "Bin:", kTextAlignLeft);
  xpos += 4*fontWidth + 5;
  myBinValue = new EditTextWidget(boss, font, xpos, ypos-2, 9*fontWidth,
                                  lineHeight, "");
  myBinValue->setEditable(false);

  // Color registers
  const char* regNames[] = { "COLUP0:", "COLUP1:", "COLUPF:", "COLUBK:" };
  xpos = 10;  ypos += 3*lineHeight;
  for(int row = 0; row < 4; ++row)
  {
    t = new StaticTextWidget(boss, font, xpos, ypos + row*lineHeight + 2,
                             7*fontWidth, fontHeight,
                             regNames[row],
                             kTextAlignLeft);
  }
  xpos += 7*fontWidth + 5;
  myColorRegs = new DataGridWidget(boss, font, xpos, ypos,
                                   1, 4, 2, 8, kBASE_16);
  myColorRegs->setTarget(this);
  myColorRegs->setID(kColorRegsID);
  addFocusWidget(myColorRegs);

  xpos += myColorRegs->colWidth() + 5;
  myCOLUP0Color = new ColorWidget(boss, font, xpos, ypos+2, 20, lineHeight - 4);
  myCOLUP0Color->setTarget(this);

  ypos += lineHeight;
  myCOLUP1Color = new ColorWidget(boss, font, xpos, ypos+2, 20, lineHeight - 4);
  myCOLUP1Color->setTarget(this);

  ypos += lineHeight;
  myCOLUPFColor = new ColorWidget(boss, font, xpos, ypos+2, 20, lineHeight - 4);
  myCOLUPFColor->setTarget(this);

  ypos += lineHeight;
  myCOLUBKColor = new ColorWidget(boss, font, xpos, ypos+2, 20, lineHeight - 4);
  myCOLUBKColor->setTarget(this);

  ////////////////////////////
  // Collision register bits
  ////////////////////////////
  // Add horizontal labels
  xpos += myCOLUBKColor->getWidth() + 2*fontWidth + 30;  ypos -= 4*lineHeight + 5;
  t = new StaticTextWidget(boss, font, xpos, ypos, 14*fontWidth, fontHeight,
                           "PF BL M1 M0 P1", kTextAlignLeft);

  // Add label for Strobes; buttons will be added later
  t = new StaticTextWidget(boss, font, xpos + t->getWidth() + 9*fontWidth, ypos,
                           8*fontWidth, fontHeight,
                           "Strobes:", kTextAlignLeft);

  // Add vertical labels
  xpos -= 2*fontWidth + 5;  ypos += lineHeight;
  const char* collLabel[] = { "P0", "P1", "M0", "M1", "BL" };
  for(int row = 0; row < 5; ++row)
  {
    t = new StaticTextWidget(boss, font, xpos, ypos + row*(lineHeight+3),
                             2*fontWidth, fontHeight,
                             collLabel[row], kTextAlignLeft);
  }

  // Finally, add all 15 collision bits
  xpos += 2 * fontWidth + 5;
  unsigned int collX = xpos, collY = ypos, idx = 0;
  for(unsigned int row = 0; row < 5; ++row)
  {
    for(unsigned int col = 0; col < 5 - row; ++col)
    {
      myCollision[idx] = new CheckboxWidget(boss, font, collX, collY,
                                            "", kCheckActionCmd);
      myCollision[idx]->setTarget(this);
      myCollision[idx]->setID(idx);
// TODO - make collisions editable in TIA //
      myCollision[idx]->setEditable(false);
//      addFocusWidget(myCollision[idx]);
////////////////////////////////////////////

      collX += myCollision[idx]->getWidth() + 10;
      idx++;
    }
    collX = xpos;
    collY += lineHeight+3;
  }

  ////////////////////////////
  // Strobe buttons
  ////////////////////////////
  ButtonWidget* b;
  unsigned int buttonX, buttonY;
  buttonX = collX + 20*fontWidth;  buttonY = ypos;
  b = new ButtonWidget(boss, font, buttonX, buttonY, 50, lineHeight,
                       "WSync", kWsyncCmd);
  b->setTarget(this);

  buttonY += lineHeight + 3;
  b = new ButtonWidget(boss, font, buttonX, buttonY, 50, lineHeight,
                       "ResP0", kResP0Cmd);
  b->setTarget(this);

  buttonY += lineHeight + 3;
  b = new ButtonWidget(boss, font, buttonX, buttonY, 50, lineHeight,
                       "ResM0", kResM0Cmd);
  b->setTarget(this);

  buttonY += lineHeight + 3;
  b = new ButtonWidget(boss, font, buttonX, buttonY, 50, lineHeight,
                       "ResBL", kResBLCmd);
  b->setTarget(this);

  buttonY += lineHeight + 3;
  b = new ButtonWidget(boss, font, buttonX, buttonY, 50, lineHeight,
                       "HmClr", kHmclrCmd);
  b->setTarget(this);

  buttonX += 50 + 10;  buttonY = ypos;
  b = new ButtonWidget(boss, font, buttonX, buttonY, 50, lineHeight,
                       "RSync", kRsyncCmd);
  b->setTarget(this);

  buttonY += lineHeight + 3;
  b = new ButtonWidget(boss, font, buttonX, buttonY, 50, lineHeight,
                       "ResP1", kResP1Cmd);
  b->setTarget(this);

  buttonY += lineHeight + 3;
  b = new ButtonWidget(boss, font, buttonX, buttonY, 50, lineHeight,
                       "ResM1", kResM1Cmd);
  b->setTarget(this);

  buttonY += lineHeight + 3;
  b = new ButtonWidget(boss, font, buttonX, buttonY, 50, lineHeight,
                       "HMove", kHmoveCmd);
  b->setTarget(this);

  buttonY += lineHeight + 3;
  b = new ButtonWidget(boss, font, buttonX, buttonY, 50, lineHeight,
                       "CxClr", kCxclrCmd);
  b->setTarget(this);

  ////////////////////////////
  // P0 register info
  ////////////////////////////
  // grP0
  xpos = 10;  ypos = 13*lineHeight;
  t = new StaticTextWidget(boss, font, xpos, ypos+2,
                           7*fontWidth, fontHeight,
                           "P0: GR:", kTextAlignLeft);
  xpos += 7*fontWidth + 5;
  myGRP0 = new TogglePixelWidget(boss, font, xpos, ypos+2, 8, 1);
  myGRP0->setTarget(this);
  myGRP0->setID(kGRP0ID);
  addFocusWidget(myGRP0);

  // posP0
  xpos += myGRP0->getWidth() + 8;
  t = new StaticTextWidget(boss, font, xpos, ypos+2,
                           4*fontWidth, fontHeight,
                           "Pos:", kTextAlignLeft);
  xpos += 4*fontWidth + 5;
  myPosP0 = new DataGridWidget(boss, font, xpos, ypos,
                               1, 1, 2, 8, kBASE_16);
  myPosP0->setTarget(this);
  myPosP0->setID(kPosP0ID);
  addFocusWidget(myPosP0);

  // hmP0
  xpos += myPosP0->getWidth() + 8;
  t = new StaticTextWidget(boss, font, xpos, ypos+2,
                           3*fontWidth, fontHeight,
                           "HM:", kTextAlignLeft);
  xpos += 3*fontWidth + 5;
  myHMP0 = new DataGridWidget(boss, font, xpos, ypos,
                              1, 1, 1, 4, kBASE_16_4);
  myHMP0->setTarget(this);
  myHMP0->setID(kHMP0ID);
  addFocusWidget(myHMP0);

  // P0 reflect and delay
  xpos += myHMP0->getWidth() + 15;
  myRefP0 = new CheckboxWidget(boss, font, xpos, ypos+1,
                               "Reflect", kCheckActionCmd);
  myRefP0->setTarget(this);
  myRefP0->setID(kRefP0ID);
  addFocusWidget(myRefP0);

  xpos += myRefP0->getWidth() + 15;
  myDelP0 = new CheckboxWidget(boss, font, xpos, ypos+1,
                               "Delay", kCheckActionCmd);
  myDelP0->setTarget(this);
  myDelP0->setID(kDelP0ID);
  addFocusWidget(myDelP0);

  // NUSIZ0 (player portion)
  xpos = 10 + lwidth;  ypos += myGRP0->getHeight() + 5;
  t = new StaticTextWidget(boss, font, xpos, ypos+2,
                           8*fontWidth, fontHeight,
                           "NusizP0:", kTextAlignLeft);
  xpos += 8*fontWidth + 5;
  myNusizP0 = new DataGridWidget(boss, font, xpos, ypos,
                                 1, 1, 1, 3, kBASE_16_4);
  myNusizP0->setTarget(this);
  myNusizP0->setID(kNusizP0ID);
  addFocusWidget(myNusizP0);

  xpos += myNusizP0->getWidth() + 5;
  myNusizP0Text = new EditTextWidget(boss, font, xpos, ypos+1, 23*fontWidth,
                                     lineHeight, "");
  myNusizP0Text->setEditable(false);

  ////////////////////////////
  // P1 register info
  ////////////////////////////
  // grP1
  xpos = 10;  ypos += 2*lineHeight;
  t = new StaticTextWidget(boss, font, xpos, ypos+2, 7*fontWidth, fontHeight,
                           "P1: GR:", kTextAlignLeft);
  xpos += 7*fontWidth + 5;
  myGRP1 = new TogglePixelWidget(boss, font, xpos, ypos+2, 8, 1);
  myGRP1->setTarget(this);
  myGRP1->setID(kGRP1ID);
  addFocusWidget(myGRP1);

  // posP1
  xpos += myGRP1->getWidth() + 8;
  t = new StaticTextWidget(boss, font, xpos, ypos+2, 4*fontWidth, fontHeight,
                           "Pos:", kTextAlignLeft);
  xpos += 4*fontWidth + 5;
  myPosP1 = new DataGridWidget(boss, font, xpos, ypos,
                               1, 1, 2, 8, kBASE_16);
  myPosP1->setTarget(this);
  myPosP1->setID(kPosP1ID);
  addFocusWidget(myPosP1);

  // hmP1
  xpos += myPosP1->getWidth() + 8;
  t = new StaticTextWidget(boss, font, xpos, ypos+2, 3*fontWidth, fontHeight,
                           "HM:", kTextAlignLeft);
  xpos += 3*fontWidth + 5;
  myHMP1 = new DataGridWidget(boss, font, xpos, ypos,
                              1, 1, 1, 4, kBASE_16_4);
  myHMP1->setTarget(this);
  myHMP1->setID(kHMP1ID);
  addFocusWidget(myHMP1);

  // P1 reflect and delay
  xpos += myHMP1->getWidth() + 15;
  myRefP1 = new CheckboxWidget(boss, font, xpos, ypos+1,
                               "Reflect", kCheckActionCmd);
  myRefP1->setTarget(this);
  myRefP1->setID(kRefP1ID);
  addFocusWidget(myRefP1);

  xpos += myRefP1->getWidth() + 15;
  myDelP1 = new CheckboxWidget(boss, font, xpos, ypos+1,
                               "Delay", kCheckActionCmd);
  myDelP1->setTarget(this);
  myDelP1->setID(kDelP1ID);
  addFocusWidget(myDelP1);

  // NUSIZ1 (player portion)
  xpos = 10 + lwidth;  ypos += myGRP1->getHeight() + 5;
  t = new StaticTextWidget(boss, font, xpos, ypos+2, 8*fontWidth, fontHeight,
                           "NusizP1:", kTextAlignLeft);
  xpos += 8*fontWidth + 5;
  myNusizP1 = new DataGridWidget(boss, font, xpos, ypos,
                                 1, 1, 1, 3, kBASE_16_4);
  myNusizP1->setTarget(this);
  myNusizP1->setID(kNusizP1ID);
  addFocusWidget(myNusizP1);

  xpos += myNusizP1->getWidth() + 5;
  myNusizP1Text = new EditTextWidget(boss, font, xpos, ypos+1, 23*fontWidth,
                                     lineHeight, "");
  myNusizP1Text->setEditable(false);

  ////////////////////////////
  // M0 register info
  ////////////////////////////
  // enaM0
  xpos = 10;  ypos += 2*lineHeight;
  t = new StaticTextWidget(boss, font, xpos, ypos+2, 3*fontWidth, fontHeight,
                           "M0:", kTextAlignLeft);
  xpos += 3*fontWidth + 8;
  myEnaM0 = new CheckboxWidget(boss, font, xpos, ypos+2,
                               "Enable", kCheckActionCmd);
  myEnaM0->setTarget(this);
  myEnaM0->setID(kEnaM0ID);
  addFocusWidget(myEnaM0);
  
  // posM0
  xpos += myEnaM0->getWidth() + 12;
  t = new StaticTextWidget(boss, font, xpos, ypos+2, 4*fontWidth, fontHeight,
                           "Pos:", kTextAlignLeft);
  xpos += 4*fontWidth + 5;
  myPosM0 = new DataGridWidget(boss, font, xpos, ypos,
                               1, 1, 2, 8, kBASE_16);
  myPosM0->setTarget(this);
  myPosM0->setID(kPosM0ID);
  addFocusWidget(myPosM0);

  // hmM0
  xpos += myPosM0->getWidth() + 8;
  t = new StaticTextWidget(boss, font, xpos, ypos+2, 3*fontWidth, fontHeight,
                           "HM:", kTextAlignLeft);
  xpos += 3*fontWidth + 5;
  myHMM0 = new DataGridWidget(boss, font, xpos, ypos,
                              1, 1, 1, 4, kBASE_16_4);
  myHMM0->setTarget(this);
  myHMM0->setID(kHMM0ID);
  addFocusWidget(myHMM0);

  // NUSIZ0 (missile portion)
  xpos += myHMM0->getWidth() + 8;
  t = new StaticTextWidget(boss, font, xpos, ypos+2, 5*fontWidth, fontHeight,
                           "Size:", kTextAlignLeft);
  xpos += 5*fontWidth + 5;
  myNusizM0 = new DataGridWidget(boss, font, xpos, ypos,
                                 1, 1, 1, 2, kBASE_16_4);
  myNusizM0->setTarget(this);
  myNusizM0->setID(kNusizM0ID);
  addFocusWidget(myNusizM0);

  // M0 reset
  xpos += myNusizM0->getWidth() + 15;
  myResMP0 = new CheckboxWidget(boss, font, xpos, ypos+1,
                                "Reset", kCheckActionCmd);
  myResMP0->setTarget(this);
  myResMP0->setID(kResMP0ID);
  addFocusWidget(myResMP0);

  ////////////////////////////
  // M1 register info
  ////////////////////////////
  // enaM1
  xpos = 10;  ypos += lineHeight + 6;
  t = new StaticTextWidget(boss, font, xpos, ypos+2, 3*fontWidth, fontHeight,
                           "M1:", kTextAlignLeft);
  xpos += 3*fontWidth + 8;
  myEnaM1 = new CheckboxWidget(boss, font, xpos, ypos+2,
                               "Enable", kCheckActionCmd);
  myEnaM1->setTarget(this);
  myEnaM1->setID(kEnaM1ID);
  addFocusWidget(myEnaM1);
  
  // posM0
  xpos += myEnaM1->getWidth() + 12;
  t = new StaticTextWidget(boss, font, xpos, ypos+2, 4*fontWidth, fontHeight,
                           "Pos:", kTextAlignLeft);
  xpos += 4*fontWidth + 5;
  myPosM1 = new DataGridWidget(boss, font, xpos, ypos,
                               1, 1, 2, 8, kBASE_16);
  myPosM1->setTarget(this);
  myPosM1->setID(kPosM1ID);
  addFocusWidget(myPosM1);

  // hmM0
  xpos += myPosM1->getWidth() + 8;
  t = new StaticTextWidget(boss, font, xpos, ypos+2, 3*fontWidth, fontHeight,
                           "HM:", kTextAlignLeft);
  xpos += 3*fontWidth + 5;
  myHMM1 = new DataGridWidget(boss, font, xpos, ypos,
                              1, 1, 1, 4, kBASE_16_4);
  myHMM1->setTarget(this);
  myHMM1->setID(kHMM1ID);
  addFocusWidget(myHMM1);

  // NUSIZ1 (missile portion)
  xpos += myHMM1->getWidth() + 8;
  t = new StaticTextWidget(boss, font, xpos, ypos+2, 5*fontWidth, fontHeight,
                           "Size:", kTextAlignLeft);
  xpos += 5*fontWidth + 5;
  myNusizM1 = new DataGridWidget(boss, font, xpos, ypos,
                                 1, 1, 1, 2, kBASE_16_4);
  myNusizM1->setTarget(this);
  myNusizM1->setID(kNusizM1ID);
  addFocusWidget(myNusizM1);

  // M1 reset
  xpos += myNusizM1->getWidth() + 15;
  myResMP1 = new CheckboxWidget(boss, font, xpos, ypos+1,
                                "Reset", kCheckActionCmd);
  myResMP1->setTarget(this);
  myResMP1->setID(kResMP1ID);
  addFocusWidget(myResMP1);

  ////////////////////////////
  // BL register info
  ////////////////////////////
  // enaBL
  xpos = 10;  ypos += lineHeight + 6;
  t = new StaticTextWidget(boss, font, xpos, ypos+2, 3*fontWidth, fontHeight,
                           "BL:", kTextAlignLeft);
  xpos += 3*fontWidth + 8;
  myEnaBL = new CheckboxWidget(boss, font, xpos, ypos+2,
                               "Enable", kCheckActionCmd);
  myEnaBL->setTarget(this);
  myEnaBL->setID(kEnaBLID);
  addFocusWidget(myEnaBL);
  
  // posBL
  xpos += myEnaBL->getWidth() + 12;
  t = new StaticTextWidget(boss, font, xpos, ypos+2, 4*fontWidth, fontHeight,
                           "Pos:", kTextAlignLeft);
  xpos += 4*fontWidth + 5;
  myPosBL = new DataGridWidget(boss, font, xpos, ypos,
                               1, 1, 2, 8, kBASE_16);
  myPosBL->setTarget(this);
  myPosBL->setID(kPosBLID);
  addFocusWidget(myPosBL);

  // hmBL
  xpos += myPosBL->getWidth() + 8;
  t = new StaticTextWidget(boss, font, xpos, ypos+2, 3*fontWidth, fontHeight,
                           "HM:", kTextAlignLeft);
  xpos += 3*fontWidth + 5;
  myHMBL = new DataGridWidget(boss, font, xpos, ypos,
                              1, 1, 1, 4, kBASE_16_4);
  myHMBL->setTarget(this);
  myHMBL->setID(kHMBLID);
  addFocusWidget(myHMBL);

  // CTRLPF (size portion)
  xpos += myHMBL->getWidth() + 8;
  t = new StaticTextWidget(boss, font, xpos, ypos+2, 5*fontWidth, fontHeight,
                           "Size:", kTextAlignLeft);
  xpos += 5*fontWidth + 5;
  mySizeBL = new DataGridWidget(boss, font, xpos, ypos,
                                1, 1, 1, 2, kBASE_16_4);
  mySizeBL->setTarget(this);
  mySizeBL->setID(kSizeBLID);
  addFocusWidget(mySizeBL);

  // BL delay
  xpos += mySizeBL->getWidth() + 15;
  myDelBL = new CheckboxWidget(boss, font, xpos, ypos+1,
                               "Delay", kCheckActionCmd);
  myDelBL->setTarget(this);
  myDelBL->setID(kDelBLID);
  addFocusWidget(myDelBL);

  ////////////////////////////
  // PF 0/1/2 registers
  ////////////////////////////
  // PF0
  xpos = 10;  ypos += lineHeight + 6;
  t = new StaticTextWidget(boss, font, xpos, ypos+2, 4*fontWidth, fontHeight,
                           "PF:", kTextAlignLeft);
  xpos += 4*fontWidth;
  myPF[0] = new TogglePixelWidget(boss, font, xpos, ypos+2, 4, 1);
  myPF[0]->setTarget(this);
  myPF[0]->setID(kPF0ID);
  addFocusWidget(myPF[0]);

  // PF1
  xpos += myPF[0]->getWidth() + 2;
  myPF[1] = new TogglePixelWidget(boss, font, xpos, ypos+2, 8, 1);
  myPF[1]->setTarget(this);
  myPF[1]->setID(kPF1ID);
  addFocusWidget(myPF[1]);

  // PF2
  xpos += myPF[1]->getWidth() + 2;
  myPF[2] = new TogglePixelWidget(boss, font, xpos, ypos+2, 8, 1);
  myPF[2]->setTarget(this);
  myPF[2]->setID(kPF2ID);
  addFocusWidget(myPF[2]);

  // PF reflect, score, priority
  xpos = 10 + 4*fontWidth;  ypos += lineHeight + 6;
  myRefPF = new CheckboxWidget(boss, font, xpos, ypos+1,
                               "Reflect", kCheckActionCmd);
  myRefPF->setTarget(this);
  myRefPF->setID(kRefPFID);
  addFocusWidget(myRefPF);

  xpos += myRefPF->getWidth() + 15;
  myScorePF = new CheckboxWidget(boss, font, xpos, ypos+1,
                                 "Score", kCheckActionCmd);
  myScorePF->setTarget(this);
  myScorePF->setID(kScorePFID);
  addFocusWidget(myScorePF);

  xpos += myScorePF->getWidth() + 15;
  myPriorityPF = new CheckboxWidget(boss, font, xpos, ypos+1,
                                    "Priority", kCheckActionCmd);
  myPriorityPF->setTarget(this);
  myPriorityPF->setID(kPriorityPFID);
  addFocusWidget(myPriorityPF);
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
    case kWsyncCmd:
      tia.strobeWsync();
      break;

    case kRsyncCmd:
      tia.strobeRsync();
      break;

    case kResP0Cmd:
      tia.strobeResP0();
      break;

    case kResP1Cmd:
      tia.strobeResP1();
      break;

    case kResM0Cmd:
      tia.strobeResM0();
      break;

    case kResM1Cmd:
      tia.strobeResM1();
      break;

    case kResBLCmd:
      tia.strobeResBL();
      break;

    case kHmoveCmd:
      tia.strobeHmove();
      break;

    case kHmclrCmd:
      tia.strobeHmclr();
      break;

    case kCxclrCmd:
      tia.strobeCxclr();
      break;

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

        case kPosM0ID:
          tia.posM0(myPosM0->getSelectedValue());
          break;

        case kPosM1ID:
          tia.posM1(myPosM1->getSelectedValue());
          break;

        case kPosBLID:
          tia.posBL(myPosBL->getSelectedValue());
          break;

        case kHMP0ID:
          tia.hmP0(myHMP0->getSelectedValue());
          break;

        case kHMP1ID:
          tia.hmP1(myHMP1->getSelectedValue());
          break;

        case kHMM0ID:
          tia.hmM0(myHMM0->getSelectedValue());
          break;

        case kHMM1ID:
          tia.hmM1(myHMM1->getSelectedValue());
          break;

        case kHMBLID:
          tia.hmBL(myHMBL->getSelectedValue());
          break;

        case kNusizP0ID:
          tia.nusizP0(myNusizP0->getSelectedValue());
          myNusizP0Text->setEditString(tia.nusizP0String());
          break;

        case kNusizP1ID:
          tia.nusizP1(myNusizP1->getSelectedValue());
          myNusizP1Text->setEditString(tia.nusizP1String());
          break;

        case kNusizM0ID:
          tia.nusizM0(myNusizM0->getSelectedValue());
          break;

        case kNusizM1ID:
          tia.nusizM1(myNusizM1->getSelectedValue());
          break;

        case kSizeBLID:
          tia.sizeBL(mySizeBL->getSelectedValue());
          break;

        default:
          cerr << "TiaWidget DG changed\n";
          break;
      }
      break;

    case kTWItemDataChangedCmd:
      switch(id)
      {
        case kGRP0ID:
          tia.grP0(myGRP0->getIntState());
          break;

        case kGRP1ID:
          tia.grP1(myGRP1->getIntState());
          break;

        case kPF0ID:
          tia.pf0(myPF[0]->getIntState());
          break;

        case kPF1ID:
          tia.pf1(myPF[1]->getIntState());
          break;

        case kPF2ID:
          tia.pf2(myPF[2]->getIntState());
          break;
      }
      break;

    case kDGSelectionChangedCmd:
      switch(id)
      {
        case kRamID:
          addr  = myRamGrid->getSelectedAddr();
          value = myRamGrid->getSelectedValue();

          // We're using the read-addresses here
          // Should we also add write-addresses, or remove this entirely?
          myLabel->setEditString(dbg.equates().getLabel(addr, true));

          myDecValue->setEditString(dbg.valueToString(value, kBASE_10));
          myBinValue->setEditString(dbg.valueToString(value, kBASE_2));
          break;
      }
      break;

    case kCheckActionCmd:
      switch(id)
      {
        case kP0_PFID:
          tia.collP0_PF(myCollision[kP0_PFID]->getState() ? 1 : 0);
          break;

        case kP0_BLID:
          tia.collP0_BL(myCollision[kP0_BLID]->getState() ? 1 : 0);
          break;

        case kP0_M1ID:
          tia.collM1_P0(myCollision[kP0_M1ID]->getState() ? 1 : 0);
          break;

        case kP0_M0ID:
          tia.collM0_P0(myCollision[kP0_M0ID]->getState() ? 1 : 0);
          break;

        case kP0_P1ID:
          tia.collP0_P1(myCollision[kP0_P1ID]->getState() ? 1 : 0);
          break;

        case kP1_PFID:
          tia.collP1_PF(myCollision[kP1_PFID]->getState() ? 1 : 0);
          break;

        case kP1_BLID:
          tia.collP1_BL(myCollision[kP1_BLID]->getState() ? 1 : 0);
          break;

        case kP1_M1ID:
          tia.collM1_P1(myCollision[kP1_M1ID]->getState() ? 1 : 0);
          break;

        case kP1_M0ID:
          tia.collM0_P1(myCollision[kP1_M0ID]->getState() ? 1 : 0);
          break;

        case kM0_PFID:
          tia.collM0_PF(myCollision[kM0_PFID]->getState() ? 1 : 0);
          break;

        case kM0_BLID:
          tia.collM0_BL(myCollision[kM0_BLID]->getState() ? 1 : 0);
          break;

        case kM0_M1ID:
          tia.collM0_M1(myCollision[kM0_M1ID]->getState() ? 1 : 0);
          break;

        case kM1_PFID:
          tia.collM1_PF(myCollision[kM1_PFID]->getState() ? 1 : 0);
          break;

        case kM1_BLID:
          tia.collM1_BL(myCollision[kM1_BLID]->getState() ? 1 : 0);
          break;

        case kBL_PFID:
          tia.collBL_PF(myCollision[kBL_PFID]->getState() ? 1 : 0);
          break;

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

        case kDelBLID:
          tia.vdelBL(myDelBL->getState() ? 1 : 0);
          break;

        case kResMP0ID:
          tia.resMP0(myResMP0->getState() ? 1 : 0);
          break;

        case kResMP1ID:
          tia.resMP1(myResMP1->getState() ? 1 : 0);
          break;

        case kRefPFID:
          tia.refPF(myRefPF->getState() ? 1 : 0);
          break;

        case kScorePFID:
          tia.scorePF(myScorePF->getState() ? 1 : 0);
          break;

        case kPriorityPFID:
          tia.priorityPF(myPriorityPF->getState() ? 1 : 0);
          break;
      }
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaWidget::loadConfig()
{
//cerr << "TiaWidget::loadConfig()\n";
  fillGrid();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaWidget::fillGrid()
{
  IntArray alist;
  IntArray vlist;
  BoolArray blist, changed, grNew, grOld;

  Debugger& dbg = instance()->debugger();
  TIADebug& tia = dbg.tiaDebug();
  TiaState& state    = (TiaState&) tia.getState();
  TiaState& oldstate = (TiaState&) tia.getOldState();

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
  // Collision register bits
  ////////////////////////////
  myCollision[kP0_PFID]->setState(tia.collP0_PF());
  myCollision[kP0_BLID]->setState(tia.collP0_BL());
  myCollision[kP0_M1ID]->setState(tia.collM1_P0());
  myCollision[kP0_M0ID]->setState(tia.collM0_P0());
  myCollision[kP0_P1ID]->setState(tia.collP0_P1());
  myCollision[kP1_PFID]->setState(tia.collP1_PF());
  myCollision[kP1_BLID]->setState(tia.collP1_BL());
  myCollision[kP1_M1ID]->setState(tia.collM1_P1());
  myCollision[kP1_M0ID]->setState(tia.collM0_P1());
  myCollision[kM0_PFID]->setState(tia.collM0_PF());
  myCollision[kM0_BLID]->setState(tia.collM0_BL());
  myCollision[kM0_M1ID]->setState(tia.collM0_M1());
  myCollision[kM1_PFID]->setState(tia.collM1_PF());
  myCollision[kM1_BLID]->setState(tia.collM1_BL());
  myCollision[kBL_PFID]->setState(tia.collBL_PF());

  ////////////////////////////
  // P0 register info
  ////////////////////////////
  // grP0
  myGRP0->setColor(state.coluRegs[0]);
  myGRP0->setIntState(state.gr[P0], false);

  // posP0
  myPosP0->setList(0, state.pos[P0], state.pos[P0] != oldstate.pos[P0]);

  // hmP0
  myHMP0->setList(0, state.hm[P0], state.hm[P0] != oldstate.hm[P0]);

  // refP0 & vdelP0
  myRefP0->setState(tia.refP0());
  myDelP0->setState(tia.vdelP0());

  // NUSIZ0 (player portion)
  myNusizP0->setList(0, state.size[P0], state.size[P0] != oldstate.size[P0]);
  myNusizP0Text->setEditString(tia.nusizP0String());

  ////////////////////////////
  // P1 register info
  ////////////////////////////
  // grP1
  myGRP1->setColor(state.coluRegs[1]);
  myGRP1->setIntState(state.gr[P1], false);

  // posP1
  myPosP1->setList(0, state.pos[P1], state.pos[P1] != oldstate.pos[P1]);

  // hmP1
  myHMP1->setList(0, state.hm[P1], state.hm[P1] != oldstate.hm[P1]);

  // refP1 & vdelP1
  myRefP1->setState(tia.refP1());
  myDelP1->setState(tia.vdelP1());

  // NUSIZ1 (player portion)
  myNusizP1->setList(0, state.size[P1], state.size[P1] != oldstate.size[P1]);
  myNusizP1Text->setEditString(tia.nusizP1String());

  ////////////////////////////
  // M0 register info
  ////////////////////////////
  // enaM0
  myEnaM0->setState(tia.enaM0());

  // posM0
  myPosM0->setList(0, state.pos[M0], state.pos[M0] != oldstate.pos[M0]);

  // hmM0
  myHMM0->setList(0, state.hm[M0], state.hm[M0] != oldstate.hm[M0]);

  // NUSIZ0 (missile portion)
  myNusizM0->setList(0, state.size[M0], state.size[M0] != oldstate.size[M0]);

  // resMP0
  myResMP0->setState(tia.resMP0());

  ////////////////////////////
  // M1 register info
  ////////////////////////////
  // enaM1
  myEnaM1->setState(tia.enaM1());

  // posM1
  myPosM1->setList(0, state.pos[M1], state.pos[M1] != oldstate.pos[M1]);

  // hmM1
  myHMM1->setList(0, state.hm[M1], state.hm[M1] != oldstate.hm[M1]);

  // NUSIZ1 (missile portion)
  myNusizM1->setList(0, state.size[M1], state.size[M1] != oldstate.size[M1]);

  // resMP1
  myResMP1->setState(tia.resMP1());

  ////////////////////////////
  // BL register info
  ////////////////////////////
  // enaBL
  myEnaBL->setState(tia.enaBL());

  // posBL
  myPosBL->setList(0, state.pos[BL], state.pos[BL] != oldstate.pos[BL]);

  // hmBL
  myHMBL->setList(0, state.hm[BL], state.hm[BL] != oldstate.hm[BL]);

  // CTRLPF (size portion)
  mySizeBL->setList(0, state.size[BL], state.size[BL] != oldstate.size[BL]);

  // vdelBL
  myDelBL->setState(tia.vdelBL());

  ////////////////////////////
  // PF register info
  ////////////////////////////
  // PF0
  myPF[0]->setColor(state.coluRegs[2]);
  myPF[0]->setIntState(state.pf[0], true);  // reverse bit order

  // PF1
  myPF[1]->setColor(state.coluRegs[2]);
  myPF[1]->setIntState(state.pf[1], false);

  // PF2
  myPF[2]->setColor(state.coluRegs[2]);
  myPF[2]->setIntState(state.pf[2], true);  // reverse bit order

  // Reflect
  myRefPF->setState(tia.refPF());

  // Score
  myScorePF->setState(tia.scorePF());

  // Priority
  myPriorityPF->setState(tia.priorityPF());
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
