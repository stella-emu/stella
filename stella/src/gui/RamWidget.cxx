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
// $Id: RamWidget.cxx,v 1.2 2005-06-16 22:18:02 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <sstream>

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "GuiUtils.hxx"
#include "GuiObject.hxx"
#include "Debugger.hxx"
#include "Widget.hxx"
#include "ByteGridWidget.hxx"

#include "RamWidget.hxx"

/*
enum {
  kSearchCmd  = 'CSEA',
  kCmpCmd     = 'CCMP',
  kRestartCmd = 'CRST'
};
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RamWidget::RamWidget(GuiObject* boss, int x, int y, int w, int h)
  : Widget(boss, x, y, w, h),
    CommandSender(boss)
{
  int xpos   = 10;
  int ypos   = 20;
  int lwidth = 30;

  // Create a 16x8 grid (16 x 8 = 128 RAM bytes) with labels
  for(int col = 0; col < 8; ++col)
  {
    StaticTextWidget* t = new StaticTextWidget(boss, xpos, ypos + col*kLineHeight + 2,
                          lwidth, kLineHeight,
                          Debugger::to_hex_16(col*16 + kRamStart) + string(":"),
                          kTextAlignLeft);
    t->setFont(instance()->consoleFont());
  }
  
  myRamGrid = new ByteGridWidget(boss, xpos+lwidth + 5, ypos, 16, 8);
  myRamGrid->setTarget(this);
  myActiveWidget = myRamGrid;

  fillGrid();

#if 0
  const int border = 20;
  const int bwidth = 50;
  const int charWidth  = instance()->consoleFont().getMaxCharWidth();
  const int charHeight = instance()->consoleFont().getFontHeight() + 2;
  int xpos = x + border;
  int ypos = y + border;

  // Add the numedit label and box
  new StaticTextWidget(boss, xpos, ypos, 70, kLineHeight,
                       "Enter a value:", kTextAlignLeft);

  myEditBox = new EditNumWidget(boss, 90, ypos - 2, charWidth*10, charHeight, "");
  myEditBox->setFont(instance()->consoleFont());
//  myEditBox->setTarget(this);
  myActiveWidget = myEditBox;
    ypos += border;

  // Add the result text string area
  myResult = new StaticTextWidget(boss, border + 5, ypos, 175, kLineHeight,
                       "", kTextAlignLeft);
  myResult->setColor(kTextColorEm);

  // Add the three search-related buttons
  xpos = x + border;
  ypos += border * 2;
  mySearchButton  = new ButtonWidget(boss, xpos, ypos, bwidth, 16,
                                     "Search", kSearchCmd, 0);
  mySearchButton->setTarget(this);
    xpos += 8 + bwidth;

  myCompareButton = new ButtonWidget(boss, xpos, ypos, bwidth, 16,
                                     "Compare", kCmpCmd, 0);
  myCompareButton->setTarget(this);
    xpos += 8 + bwidth;

  myRestartButton = new ButtonWidget(boss, xpos, ypos, bwidth, 16,
                                     "Restart", kRestartCmd, 0);
  myRestartButton->setTarget(this);

  // Add the list showing the results of a search/compare
  xpos = 200;  ypos = border/2;
  new StaticTextWidget(boss, xpos + 10, ypos, 70, kLineHeight,
                       "Address  Value", kTextAlignLeft);
  ypos += kLineHeight;

  myResultsList = new AddrValueWidget(boss, xpos, ypos, 100, 75);
  myResultsList->setNumberingMode(kHexNumbering);
  myResultsList->setFont(instance()->consoleFont());
  myResultsList->setTarget(this);

  // Start in a known state
  doRestart();
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RamWidget::~RamWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::handleCommand(CommandSender* sender, int cmd, int data)
{
  switch(cmd)
  {
/*
    case kBGItemDoubleClickedCmd:
      cerr << "double-clicked on " << data << endl;
      break;

    case kBGItemActivatedCmd:
      cerr << "item activated on " << data << endl;
      break;

    case kBGSelectionChangedCmd:
      cerr << "selection changed on " << data << endl;
      break;
*/
    case kBGItemDataChangedCmd:
      cerr << "data changed on " << data << endl;
/*
      int addr  = myResultsList->getSelectedAddr() - kRamStart;
      int value = myResultsList->getSelectedValue();
      instance()->debugger().writeRAM(addr, value);
*/
      break;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::loadConfig()
{
cerr << "RamWidget::loadConfig()\n";
  fillGrid();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::fillGrid()
{
  ByteAddrList alist;
  ByteValueList vlist;

  for(unsigned int i = 0; i < kRamSize; i++)
  {
    alist.push_back(kRamStart + i);
    vlist.push_back(instance()->debugger().readRAM(i));
  }

  myRamGrid->setList(alist, vlist);
}
