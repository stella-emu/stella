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
// $Id: DebuggerDialog.cxx,v 1.15 2005-06-18 13:45:34 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "Widget.hxx"
#include "Dialog.hxx"
#include "TabWidget.hxx"
#include "ListWidget.hxx"
#include "PromptWidget.hxx"
#include "CheatWidget.hxx"
#include "RamWidget.hxx"
#include "Debugger.hxx"

#include "DebuggerDialog.hxx"

enum {
  kDDStepCmd  = 'DDst',
  kDDTraceCmd = 'DDtr',
  kDDAdvCmd   = 'DDav',
  kDDExitCmd  = 'DDex'
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DebuggerDialog::DebuggerDialog(OSystem* osystem, DialogContainer* parent,
                               int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h),
    myTab(NULL)
{
  const int vBorder = 4;
  const int vWidth = _w - kButtonWidth - 20;

  // The tab widget
  myTab = new TabWidget(this, 0, vBorder, vWidth, _h - vBorder - 1);

  // 1) The Prompt/console tab
  myTab->addTab("Prompt");
  myPrompt = new PromptWidget(myTab, 2, 2, vWidth - vBorder, _h - 25);
  myTab->setParentWidget(0, myPrompt, myPrompt);

  // 2) The CPU tab
  myTab->addTab("CPU");


  // 3) The RAM tab
  myTab->addTab("RAM");
  RamWidget* ram = new RamWidget(myTab, 2, 2, vWidth - vBorder, _h - 25);
  myTab->setParentWidget(2, ram, ram->activeWidget());

  // 4) The ROM tab
  myTab->addTab("ROM");


  // 5) The TIA tab
  myTab->addTab("TIA");


  // 6) The Cheat tab
  myTab->addTab("Cheat");
  CheatWidget* cheat = new CheatWidget(myTab, 2, 2,
                                       vWidth - vBorder, _h - 25);
  myTab->setParentWidget(5, cheat, cheat->activeWidget());

  // Set active tab to prompt
  myTab->setActiveTab(0);

  // Add some buttons that are always shown, no matter which tab we're in
  int yoff = vBorder + kTabHeight + 5;
  addButton(vWidth + 10, yoff, "Step", kDDStepCmd, 0);
  yoff += 22;
  addButton(vWidth + 10, yoff, "Trace", kDDTraceCmd, 0);
  yoff += 22;
  addButton(vWidth + 10, yoff, "Frame +1", kDDAdvCmd, 0);

  addButton(vWidth + 10, _h - 22 - 10, "Exit", kDDExitCmd, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DebuggerDialog::~DebuggerDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::loadConfig()
{
  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::handleKeyDown(int ascii, int keycode, int modifiers)
{
  myTab->handleKeyDown(ascii, keycode, modifiers);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::handleCommand(CommandSender* sender, int cmd, int data)
{
  // We reload the tabs in the cases where the actions could possibly
  // change their contents
  switch(cmd)
  {
    case kDDStepCmd:
      instance()->debugger().step();
      myTab->loadConfig();
      break;

    case kDDTraceCmd:
      instance()->debugger().trace();
      myTab->loadConfig();
      break;

    case kDDAdvCmd:
      instance()->debugger().nextFrame();
      myTab->loadConfig();
      break;

    case kDDExitCmd:
      instance()->debugger().quit();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PromptWidget *DebuggerDialog::prompt()
{
  return myPrompt;
}
