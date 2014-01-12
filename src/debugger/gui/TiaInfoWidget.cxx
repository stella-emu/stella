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

#include "Base.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "Debugger.hxx"
#include "TIADebug.hxx"
#include "Widget.hxx"
#include "EditTextWidget.hxx"
#include "GuiObject.hxx"

#include "TiaInfoWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaInfoWidget::TiaInfoWidget(GuiObject* boss, const GUI::Font& lfont,
                             const GUI::Font& nfont,
                             int x, int y, int max_w)
  : Widget(boss, lfont, x, y, 16, 16),
    CommandSender(boss)
{
  bool longstr = 34 * lfont.getMaxCharWidth() <= max_w;

  x += 5;
  const int lineHeight = lfont.getLineHeight();
  int xpos = x, ypos = y;
  int lwidth = lfont.getStringWidth(longstr ? "Frame Cycle:" : "F. Cycle:");
  int fwidth = 5 * lfont.getMaxCharWidth() + 4;

  // Add frame info
  xpos = x;  ypos = y + 10;
  new StaticTextWidget(boss, lfont, xpos, ypos, lwidth, lineHeight,
                       longstr ? "Frame Count:" : "Frame:",
                       kTextAlignLeft);
  xpos += lwidth;
  myFrameCount = new EditTextWidget(boss, nfont, xpos, ypos-1, fwidth, lineHeight, "");
  myFrameCount->setEditable(false);

  xpos = x;  ypos += lineHeight + 5;
  new StaticTextWidget(boss, lfont, xpos, ypos, lwidth, lineHeight,
                       longstr ? "Frame Cycle:" : "F. Cycle:",
                       kTextAlignLeft);
  xpos += lwidth;
  myFrameCycles = new EditTextWidget(boss, nfont, xpos, ypos-1, fwidth, lineHeight, "");
  myFrameCycles->setEditable(false);

  xpos = x + 20;  ypos += lineHeight + 8;
  myVSync = new CheckboxWidget(boss, lfont, xpos, ypos-3, "VSync", 0);
  myVSync->setEditable(false);

  xpos = x + 20;  ypos += lineHeight + 5;
  myVBlank = new CheckboxWidget(boss, lfont, xpos, ypos-3, "VBlank", 0);
  myVBlank->setEditable(false);

  xpos = x + lwidth + myFrameCycles->getWidth() + 8;  ypos = y + 10;
  lwidth = lfont.getStringWidth(longstr ? "Color Clock:" : "Pixel Pos:");
  fwidth = 3 * lfont.getMaxCharWidth() + 4;
  new StaticTextWidget(boss, lfont, xpos, ypos, lwidth, lineHeight,
                       "Scanline:", kTextAlignLeft);

  myScanlineCount = new EditTextWidget(boss, nfont, xpos+lwidth, ypos-1, fwidth,
                                       lineHeight, "");
  myScanlineCount->setEditable(false);

  ypos += lineHeight + 5;
  new StaticTextWidget(boss, lfont, xpos, ypos, lwidth, lineHeight,
                       longstr ? "Scan Cycle:" : "S. Cycle:", kTextAlignLeft);

  myScanlineCycles = new EditTextWidget(boss, nfont, xpos+lwidth, ypos-1, fwidth,
                                        lineHeight, "");
  myScanlineCycles->setEditable(false);

  ypos += lineHeight + 5;
  new StaticTextWidget(boss, lfont, xpos, ypos, lwidth, lineHeight,
                       "Pixel Pos:", kTextAlignLeft);

  myPixelPosition = new EditTextWidget(boss, nfont, xpos+lwidth, ypos-1, fwidth,
                                       lineHeight, "");
  myPixelPosition->setEditable(false);

  ypos += lineHeight + 5;
  new StaticTextWidget(boss, lfont, xpos, ypos, lwidth, lineHeight,
                       longstr ? "Color Clock:" : "Color Clk:", kTextAlignLeft);

  myColorClocks = new EditTextWidget(boss, nfont, xpos+lwidth, ypos-1, fwidth,
                                     lineHeight, "");
  myColorClocks->setEditable(false);

  // Calculate actual dimensions
  _w = myColorClocks->getAbsX() + myColorClocks->getWidth() - x;
  _h = ypos + lineHeight;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaInfoWidget::~TiaInfoWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaInfoWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
//cerr << "TiaInfoWidget button press: x = " << x << ", y = " << y << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaInfoWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaInfoWidget::loadConfig()
{
  Debugger& dbg = instance().debugger();
  TIADebug& tia = dbg.tiaDebug();

  myFrameCount->setText(Common::Base::toString(tia.frameCount(), Common::Base::F_10));
  myFrameCycles->setText(Common::Base::toString(dbg.cycles(), Common::Base::F_10));

  myVSync->setState(tia.vsync());
  myVBlank->setState(tia.vblank());

  int clk = tia.clocksThisLine();
  myScanlineCount->setText(Common::Base::toString(tia.scanlines(), Common::Base::F_10));
  myScanlineCycles->setText(Common::Base::toString(clk/3, Common::Base::F_10));
  myPixelPosition->setText(Common::Base::toString(clk-68, Common::Base::F_10));
  myColorClocks->setText(Common::Base::toString(clk, Common::Base::F_10));
}
