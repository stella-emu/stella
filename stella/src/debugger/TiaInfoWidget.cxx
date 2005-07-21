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
// $Id: TiaInfoWidget.cxx,v 1.1 2005-07-21 19:30:15 stephena Exp $
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
TiaInfoWidget::TiaInfoWidget(GuiObject* boss, int x, int y, int w, int h)
  : Widget(boss, x, y, w, h),
    CommandSender(boss)
{
  int xpos = x, ypos = y, lwidth = 45;
  const GUI::Font& font = instance()->consoleFont();

  // Add frame info
  xpos = x + 10;  ypos = y + 10;
  new StaticTextWidget(boss, xpos, ypos, lwidth, kLineHeight, "Frame:", kTextAlignLeft);
  xpos += lwidth;
  myFrameCount = new EditTextWidget(boss, xpos, ypos-2, 45, kLineHeight, "");
  myFrameCount->clearFlags(WIDGET_TAB_NAVIGATE);
  myFrameCount->setFont(font);
  myFrameCount->setEditable(false);

  xpos = x + 10;  ypos += kLineHeight + 5;
  new StaticTextWidget(boss, xpos, ypos, lwidth, kLineHeight, "F. Cycles:", kTextAlignLeft);
  xpos += lwidth;
  myFrameCycles = new EditTextWidget(boss, xpos, ypos-2, 45, kLineHeight, "");
  myFrameCycles->clearFlags(WIDGET_TAB_NAVIGATE);
  myFrameCycles->setFont(font);
  myFrameCycles->setEditable(false);

  xpos = x + 20;  ypos += kLineHeight + 5;
  myVSync = new CheckboxWidget(boss, xpos, ypos-3, 25, kLineHeight, "VSync", 0);
  myVSync->clearFlags(WIDGET_TAB_NAVIGATE);
  myVSync->setEditable(false);

  xpos = x + 20;  ypos += kLineHeight + 5;
  myVBlank = new CheckboxWidget(boss, xpos, ypos-3, 30, kLineHeight, "VBlank", 0);
  myVBlank->clearFlags(WIDGET_TAB_NAVIGATE);
  myVBlank->setEditable(false);

  xpos = x + 10 + 100;  ypos = y + 10;
  new StaticTextWidget(boss, xpos, ypos, lwidth, kLineHeight, "Scanline:", kTextAlignLeft);
  xpos += lwidth;
  myScanlineCount = new EditTextWidget(boss, xpos, ypos-2, 30, kLineHeight, "");
  myScanlineCount->clearFlags(WIDGET_TAB_NAVIGATE);
  myScanlineCount->setFont(font);
  myScanlineCount->setEditable(false);

  xpos = x + 10 + 100;  ypos += kLineHeight + 5;
  new StaticTextWidget(boss, xpos, ypos, lwidth, kLineHeight, "S. Cycles:", kTextAlignLeft);
  xpos += lwidth;
  myScanlineCycles = new EditTextWidget(boss, xpos, ypos-2, 30, kLineHeight, "");
  myScanlineCycles->clearFlags(WIDGET_TAB_NAVIGATE);
  myScanlineCycles->setFont(font);
  myScanlineCycles->setEditable(false);

  xpos = x + 10 + 100;  ypos += kLineHeight + 5;
  new StaticTextWidget(boss, xpos, ypos, lwidth, kLineHeight, "Pixel Pos:", kTextAlignLeft);
  xpos += lwidth;
  myPixelPosition = new EditTextWidget(boss, xpos, ypos-2, 30, kLineHeight, "");
  myPixelPosition->clearFlags(WIDGET_TAB_NAVIGATE);
  myPixelPosition->setFont(font);
  myPixelPosition->setEditable(false);

  xpos = x + 10 + 100;  ypos += kLineHeight + 5;
  new StaticTextWidget(boss, xpos, ypos, lwidth, kLineHeight, "Color Clk:", kTextAlignLeft);
  xpos += lwidth;
  myColorClocks = new EditTextWidget(boss, xpos, ypos-2, 30, kLineHeight, "");
  myColorClocks->clearFlags(WIDGET_TAB_NAVIGATE);
  myColorClocks->setFont(font);
  myColorClocks->setEditable(false);
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
