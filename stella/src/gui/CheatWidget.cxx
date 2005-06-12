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
// $Id: CheatWidget.cxx,v 1.1 2005-06-12 20:12:10 stephena Exp $
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

#include "CheatWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatWidget::CheatWidget(GuiObject* boss, int x, int y, int w, int h)
  : Widget(boss, x, y, w, h),
    CommandSender(boss)
{
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
