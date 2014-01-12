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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "DataGridWidget.hxx"
#include "FrameBuffer.hxx"
#include "GuiObject.hxx"
#include "OSystem.hxx"
#include "TIADebug.hxx"
#include "Widget.hxx"

#include "AudioWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioWidget::AudioWidget(GuiObject* boss, const GUI::Font& lfont,
                         const GUI::Font& nfont,
                         int x, int y, int w, int h)
  : Widget(boss, lfont, x, y, w, h),
    CommandSender(boss)
{
  const int fontWidth  = lfont.getMaxCharWidth(),
            fontHeight = lfont.getFontHeight(),
            lineHeight = lfont.getLineHeight();
  int xpos = 10, ypos = 25, lwidth = lfont.getStringWidth("AUDW: ");

  // AudF registers
  new StaticTextWidget(boss, lfont, xpos, ypos+2,
                       lwidth, fontHeight,
                       "AUDF:", kTextAlignLeft);
  xpos += lwidth;
  myAudF = new DataGridWidget(boss, nfont, xpos, ypos,
                              2, 1, 2, 5, Common::Base::F_16);
  myAudF->setTarget(this);
  myAudF->setID(kAUDFID);
  myAudF->setEditable(false);
  addFocusWidget(myAudF);

  for(int col = 0; col < 2; ++col)
  {
    new StaticTextWidget(boss, lfont, xpos + col*myAudF->colWidth() + 7,
                         ypos - lineHeight, fontWidth, fontHeight,
                         Common::Base::toString(col, Common::Base::F_16_1),
                         kTextAlignLeft);
  }

  // AudC registers
  xpos = 10;  ypos += lineHeight + 5;
  new StaticTextWidget(boss, lfont, xpos, ypos+2, lwidth, fontHeight,
                       "AUDC:", kTextAlignLeft);
  xpos += lwidth;
  myAudC = new DataGridWidget(boss, nfont, xpos, ypos,
                              2, 1, 2, 4, Common::Base::F_16);
  myAudC->setTarget(this);
  myAudC->setID(kAUDCID);
  myAudC->setEditable(false);
  addFocusWidget(myAudC);

  // AudV registers
  xpos = 10;  ypos += lineHeight + 5;
  new StaticTextWidget(boss, lfont, xpos, ypos+2, lwidth, fontHeight,
                       "AUDV:", kTextAlignLeft);
  xpos += lwidth;
  myAudV = new DataGridWidget(boss, nfont, xpos, ypos,
                              2, 1, 2, 4, Common::Base::F_16);
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
  // TODO - implement this
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioWidget::loadConfig()
{
  IntArray alist;
  IntArray vlist;
  BoolArray blist, changed, grNew, grOld;

  Debugger& dbg = instance().debugger();
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
