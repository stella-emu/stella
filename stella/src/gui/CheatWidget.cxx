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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: CheatWidget.cxx,v 1.2 2005-06-14 01:11:48 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "GuiUtils.hxx"
#include "GuiObject.hxx"
#include "Widget.hxx"

#include "ListWidget.hxx"
#include "EditTextWidget.hxx"

#include "CheatWidget.hxx"

enum {
  kSearchCmd,
  kCmpCmd,
  kRestartCmd
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatWidget::CheatWidget(GuiObject* boss, int x, int y, int w, int h)
  : Widget(boss, x, y, w, h),
    CommandSender(boss)
{
  int xpos = 10;
  const int bwidth = 50, space = 20;

  // Add the edit textbox, set to accept decimal values
  myEditBox = new EditTextWidget(boss, 10, 10, 80, 16, "");
  myEditBox->setFont(_boss->instance()->consoleFont());
  myEditBox->setEditString("HELLO!");
//  myEditBox->setTarget(this);

//  myResult = new StaticTextWidget();

  // Add the three search-related buttons
  mySearchButton  = new ButtonWidget(boss, xpos, _h - space, bwidth, 16,
                                     "Search", kSearchCmd, 0);
  mySearchButton->setTarget(this);
    xpos += 8 + bwidth;

  myCompareButton = new ButtonWidget(boss, xpos, _h - space, bwidth, 16,
                                     "Compare", kCmpCmd, 0);
  myCompareButton->setTarget(this);
    xpos += 8 + bwidth;

  myRestartButton = new ButtonWidget(boss, xpos, _h - space, bwidth, 16,
                                     "Restart", kRestartCmd, 0);
  myRestartButton->setTarget(this);
    xpos += 8 + bwidth;

  ListWidget* myList = new ListWidget(boss, 10, 24, 100, h - 50);
  myList->setTarget(this);
  myList->setEditable(false);
  myList->setNumberingMode(kListNumberingOff);
  myActiveWidget = myList;

  StringList l;
  for (int i = 0; i < 10; ++i)
    l.push_back("TESTING!!!");
  myList->setList(l);


  ListWidget* myList2 = new ListWidget(boss, 150, 24, 100, h - 70);
  myList2->setTarget(this);
  myList2->setEditable(false);
  myList2->setNumberingMode(kListNumberingOff);

  StringList l2;
  for (int i = 0; i < 10; ++i)
    l2.push_back("TESTING AGAIN!!!");
  myList2->setList(l2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatWidget::~CheatWidget()
{
cerr << "CheatWidget::~CheatWidget()\n";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatWidget::handleCommand(CommandSender* sender, int cmd, int data)
{
}
