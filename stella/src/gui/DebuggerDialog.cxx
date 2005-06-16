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
// $Id: DebuggerDialog.cxx,v 1.11 2005-06-16 22:18:02 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "TabWidget.hxx"
#include "ListWidget.hxx"
#include "PromptWidget.hxx"
#include "CheatWidget.hxx"
#include "RamWidget.hxx"

#include "DebuggerDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DebuggerDialog::DebuggerDialog(OSystem* osystem, DialogContainer* parent,
                               int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h),
    myTab(NULL)
{
  const int vBorder = 4;

  // The tab widget
  myTab = new TabWidget(this, 0, vBorder, _w, _h - vBorder - 1);

  // 1) The Prompt/console tab
  myTab->addTab("Prompt");
  PromptWidget* prompt = new PromptWidget(myTab, 2, 2, _w - vBorder, _h - 25);
  myTab->setParentWidget(0, prompt, prompt);

  // 2) The CPU tab
  myTab->addTab("CPU");


  // 3) The RAM tab
  myTab->addTab("RAM");
  RamWidget* ram = new RamWidget(myTab, 2, 2, _w - vBorder, _h - 25);
  myTab->setParentWidget(2, ram, ram->activeWidget());

  // 4) The ROM tab
  myTab->addTab("ROM");


  // 5) The TIA tab
  myTab->addTab("TIA");


  // 6) The Cheat tab
  myTab->addTab("Cheat");
  CheatWidget* cheat = new CheatWidget(myTab, 2, 2,
                                       _w - vBorder, _h - 25);
  myTab->setParentWidget(5, cheat, cheat->activeWidget());

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
