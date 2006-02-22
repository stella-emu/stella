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
// $Id: TiaInfoWidget.cxx,v 1.4 2006-02-22 17:38:04 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "Debugger.hxx"
#include "TIADebug.hxx"
#include "Widget.hxx"
#include "EditTextWidget.hxx"
#include "GuiObject.hxx"

#include "TiaInfoWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaInfoWidget::TiaInfoWidget(GuiObject* boss, const GUI::Font& font,
                             int x, int y)
  : Widget(boss, font, x, y, 16, 16),
    CommandSender(boss)
{
  const int lineHeight = font.getLineHeight();
  int xpos = x, ypos = y, lwidth = 45;

  // Add frame info
  xpos = x;  ypos = y + 10;
  new StaticTextWidget(boss, font, xpos, ypos, lwidth, lineHeight,
                       "Frame:", kTextAlignLeft);
  xpos += lwidth;
  myFrameCount = new EditTextWidget(boss, font, xpos, ypos-2, 45, lineHeight, "");
  myFrameCount->setEditable(false);

  xpos = x;  ypos += lineHeight + 5;
  new StaticTextWidget(boss, font, xpos, ypos, lwidth, lineHeight,
                       "F. Cycles:", kTextAlignLeft);
  xpos += lwidth;
  myFrameCycles = new EditTextWidget(boss, font, xpos, ypos-2, 45, lineHeight, "");
  myFrameCycles->setEditable(false);

  xpos = x + 10;  ypos += lineHeight + 5;
  myVSync = new CheckboxWidget(boss, font, xpos, ypos-3, "VSync", 0);
  myVSync->setEditable(false);

  xpos = x + 10;  ypos += lineHeight + 5;
  myVBlank = new CheckboxWidget(boss, font, xpos, ypos-3, "VBlank", 0);
  myVBlank->setEditable(false);

  xpos = x + 100;  ypos = y + 10;
  new StaticTextWidget(boss, font, xpos, ypos, lwidth, lineHeight,
                       "Scanline:", kTextAlignLeft);
  xpos += lwidth;
  myScanlineCount = new EditTextWidget(boss, font, xpos, ypos-2, 30, lineHeight, "");
  myScanlineCount->setEditable(false);

  xpos = x + 100;  ypos += lineHeight + 5;
  new StaticTextWidget(boss, font, xpos, ypos, lwidth, lineHeight,
                       "S. Cycles:", kTextAlignLeft);
  xpos += lwidth;
  myScanlineCycles = new EditTextWidget(boss, font, xpos, ypos-2, 30, lineHeight, "");
  myScanlineCycles->setEditable(false);

  xpos = x + 100;  ypos += lineHeight + 5;
  new StaticTextWidget(boss, font, xpos, ypos, lwidth, lineHeight,
                       "Pixel Pos:", kTextAlignLeft);
  xpos += lwidth;
  myPixelPosition = new EditTextWidget(boss, font, xpos, ypos-2, 30, lineHeight, "");
  myPixelPosition->setEditable(false);

  xpos = x + 100;  ypos += lineHeight + 5;
  new StaticTextWidget(boss, font, xpos, ypos, lwidth, lineHeight,
                       "Color Clk:", kTextAlignLeft);
  xpos += lwidth;
  myColorClocks = new EditTextWidget(boss, font, xpos, ypos-2, 30, lineHeight, "");
  myColorClocks->setEditable(false);

  // Calculate actual dimensions
  _w = 100 + 30 + lwidth;
  _h = ypos + lineHeight;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaInfoWidget::~TiaInfoWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaInfoWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
cerr << "TiaInfoWidget button press: x = " << x << ", y = " << y << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaInfoWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaInfoWidget::loadConfig()
{
  Debugger& dbg = instance()->debugger();
  TIADebug& tia = dbg.tiaDebug();

  myFrameCount->setEditString(dbg.valueToString(tia.frameCount(), kBASE_10));
  myFrameCycles->setEditString(dbg.valueToString(dbg.cycles(), kBASE_10));

  myVSync->setState(tia.vsync());
  myVBlank->setState(tia.vblank());

  int clk = tia.clocksThisLine();
  myScanlineCount->setEditString(dbg.valueToString(tia.scanlines(), kBASE_10));
  myScanlineCycles->setEditString(dbg.valueToString(clk/3, kBASE_10));
  myPixelPosition->setEditString(dbg.valueToString(clk-68, kBASE_10));
  myColorClocks->setEditString(dbg.valueToString(clk, kBASE_10));
}
