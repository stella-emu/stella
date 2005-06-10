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
// $Id: DebuggerDialog.cxx,v 1.7 2005-06-10 17:46:06 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "TabWidget.hxx"
#include "ListWidget.hxx"
#include "PromptWidget.hxx"
#include "DebuggerDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DebuggerDialog::DebuggerDialog(OSystem* osystem, DialogContainer* parent,
                               int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h),
    myTab(NULL)
{
  const int vBorder = 4;
  int yoffset;

  // The tab widget
  myTab = new TabWidget(this, 0, vBorder, _w, _h - vBorder - 1);

  // 1) The Prompt/console tab
  myTab->addTab("Prompt");
  yoffset = vBorder;

  PromptWidget* prompt = new PromptWidget(myTab, 2, 2,
                                          _w - vBorder, _h - 25);
  myTab->setActiveWidget(0, prompt);

  // 2) The CPU tab
  myTab->addTab("CPU");
  yoffset = vBorder;

/////////////////////  FIXME - just testing, will be removed
  ListWidget* myList = new ListWidget(myTab, 10, 24, 100, _h - 24 - 26 - 10 - 10);
  myList->setEditable(false);
  myList->setNumberingMode(kListNumberingOff);

  StringList l;
  for (int i = 0; i < 10; ++i)
    l.push_back("TESTING!!!");
  myList->setList(l);

  ListWidget* myList2 = new ListWidget(myTab, 150, 24, 100, _h - 24 - 26 - 10 - 10);
  myList2->setEditable(false);
  myList2->setNumberingMode(kListNumberingOff);

  StringList l2;
  for (int i = 0; i < 10; ++i)
    l2.push_back("TESTING AGAIN!!!");
  myList2->setList(l2);

  ListWidget* myList3 = new ListWidget(myTab, 300, 24, 100, _h - 24 - 26 - 10 - 10);
  myList3->setEditable(false);
  myList3->setNumberingMode(kListNumberingOff);

  StringList l3;
  for (int i = 0; i < 10; ++i)
    l3.push_back("CPU_TESTING!!!");
  myList3->setList(l3);

  myTab->setActiveWidget(1, myList);
/////////////////////////////

  // 3) The RAM tab
  myTab->addTab("RAM");
  yoffset = vBorder;


  // 4) The ROM tab
  myTab->addTab("ROM");
  yoffset = vBorder;


  // 5) The TIA tab
  myTab->addTab("TIA");
  yoffset = vBorder;


  // 6) The RAM tab
  myTab->addTab("Code");
  yoffset = vBorder;




  // Set active tab to prompt
  myTab->setActiveTab(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DebuggerDialog::~DebuggerDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::loadConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::handleKeyDown(int ascii, int keycode, int modifiers)
{
  myTab->handleKeyDown(ascii, keycode, modifiers);
}
