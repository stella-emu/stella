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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: AudioWidget.cxx,v 1.5 2007-01-01 18:04:42 stephena Exp $
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
#include "DataGridWidget.hxx"
#include "AudioWidget.hxx"

// ID's for the various widgets
// We need ID's, since there are more than one of several types of widgets
enum {
  kAUDFID,
  kAUDCID,
  kAUDVID
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioWidget::AudioWidget(GuiObject* boss, const GUI::Font& font,
                         int x, int y, int w, int h)
  : Widget(boss, font, x, y, w, h),
    CommandSender(boss)
{
  _type = kAudioWidget;

  const int fontWidth  = font.getMaxCharWidth(),
            fontHeight = font.getFontHeight(),
            lineHeight = font.getLineHeight();
  int xpos = 10, ypos = 25, lwidth = font.getStringWidth("AUDW: ");

  // AudF registers
  new StaticTextWidget(boss, font, xpos, ypos+2,
                       lwidth, fontHeight,
                       "AUDF:", kTextAlignLeft);
  xpos += lwidth;
  myAudF = new DataGridWidget(boss, font, xpos, ypos,
                              2, 1, 2, 5, kBASE_16);
  myAudF->setTarget(this);
  myAudF->setID(kAUDFID);
  myAudF->setEditable(false);
  addFocusWidget(myAudF);

  for(int col = 0; col < 2; ++col)
  {
    new StaticTextWidget(boss, font, xpos + col*myAudF->colWidth() + 7,
                         ypos - lineHeight, fontWidth, fontHeight,
                         Debugger::to_hex_4(col), kTextAlignLeft);
  }

  // AudC registers
  xpos = 10;  ypos += lineHeight + 5;
  new StaticTextWidget(boss, font, xpos, ypos+2, lwidth, fontHeight,
                       "AUDC:", kTextAlignLeft);
  xpos += lwidth;
  myAudC = new DataGridWidget(boss, font, xpos, ypos,
                              2, 1, 2, 4, kBASE_16);
  myAudC->setTarget(this);
  myAudC->setID(kAUDCID);
  myAudC->setEditable(false);
  addFocusWidget(myAudC);

  // AudV registers
  xpos = 10;  ypos += lineHeight + 5;
  new StaticTextWidget(boss, font, xpos, ypos+2, lwidth, fontHeight,
                       "AUDV:", kTextAlignLeft);
  xpos += lwidth;
  myAudV = new DataGridWidget(boss, font, xpos, ypos,
                              2, 1, 2, 4, kBASE_16);
  myAudV->setTarget(this);
  myAudV->setID(kAUDVID);
  myAudV->setEditable(false);
  addFocusWidget(myAudV);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioWidget::~AudioWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
/* FIXME - implement this
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
        case kNusizP0ID:
          tia.nusizP0(myNusizP0->getSelectedValue());
          myNusizP0Text->setEditString(tia.nusizP0String());
          break;
      }
      break;
  }
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioWidget::loadConfig()
{
//cerr << "AudioWidget::loadConfig()\n";
  fillGrid();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioWidget::fillGrid()
{
  IntArray alist;
  IntArray vlist;
  BoolArray blist, changed, grNew, grOld;

  Debugger& dbg = instance()->debugger();
  TIADebug& tia = dbg.tiaDebug();
  TiaState state    = (TiaState&) tia.getState();
  TiaState oldstate = (TiaState&) tia.getOldState();

  // AUDF0/1
  alist.clear();  vlist.clear();  changed.clear();
  for(unsigned int i = 0; i < 2; i++)
  {
    alist.push_back(i);
    vlist.push_back(state.aud[i]);
    changed.push_back(state.aud[i] != oldstate.aud[i]);
  }
  myAudF->setList(alist, vlist, changed);

  // AUDC0/1
  alist.clear();  vlist.clear();  changed.clear();
  for(unsigned int i = 2; i < 4; i++)
  {
    alist.push_back(i-2);
    vlist.push_back(state.aud[i]);
    changed.push_back(state.aud[i] != oldstate.aud[i]);
  }
  myAudC->setList(alist, vlist, changed);

  // AUDV0/1
  alist.clear();  vlist.clear();  changed.clear();
  for(unsigned int i = 4; i < 6; i++)
  {
    alist.push_back(i-4);
    vlist.push_back(state.aud[i]);
    changed.push_back(state.aud[i] != oldstate.aud[i]);
  }
  myAudV->setList(alist, vlist, changed);
}
