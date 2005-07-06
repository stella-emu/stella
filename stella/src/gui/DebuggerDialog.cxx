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
// $Id: DebuggerDialog.cxx,v 1.22 2005-07-06 15:09:16 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "Widget.hxx"
#include "Dialog.hxx"
#include "TabWidget.hxx"
#include "PromptWidget.hxx"
#include "CpuWidget.hxx"
#include "RamWidget.hxx"
#include "TiaWidget.hxx"
#include "CheatWidget.hxx"
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
  CpuWidget* cpu = new CpuWidget(myTab, 2, 2, vWidth - vBorder, _h - 25);
  myTab->setParentWidget(1, cpu, cpu->activeWidget());

  // 3) The RAM tab (part of RIOT)
  myTab->addTab("RAM");
  RamWidget* ram = new RamWidget(myTab, 2, 2, vWidth - vBorder, _h - 25);
  myTab->setParentWidget(2, ram, ram->activeWidget());

  // 4) The input/output tab (part of RIOT)
  myTab->addTab("I/O");


  // 5) The TIA tab
  myTab->addTab("TIA");
  TiaWidget* tia = new TiaWidget(myTab, 2, 2, vWidth - vBorder, _h - 25);
  myTab->setParentWidget(3, tia, tia->activeWidget());


  // 6) The ROM tab
  myTab->addTab("ROM");


  // 7) The Cheat tab
  myTab->addTab("Cheat");
  CheatWidget* cheat = new CheatWidget(myTab, 2, 2,
                                       vWidth - vBorder, _h - 25);
  myTab->setParentWidget(6, cheat, cheat->activeWidget());

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
  // Doing this means we disallow 'Alt xxx' events to any widget in the tabset
  if(instance()->eventHandler().kbdAlt(modifiers))
  {
    if(ascii == 's')
      doStep();
    else if(ascii == 't')
      doTrace();
    else if(ascii == 'f')
      doAdvance();
  }
  else
    myTab->handleKeyDown(ascii, keycode, modifiers);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::handleCommand(CommandSender* sender, int cmd,
                                   int data, int id)
{
  // We reload the tabs in the cases where the actions could possibly
  // change their contents
  switch(cmd)
  {
    case kDDStepCmd:
      doStep();
      break;

    case kDDTraceCmd:
      doTrace();
      break;

    case kDDAdvCmd:
      doAdvance();
      break;

    case kDDExitCmd:
      doExit();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doStep()
{
  instance()->debugger().step();
  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doTrace()
{
  instance()->debugger().trace();
  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doAdvance()
{
  instance()->debugger().nextFrame(1);
  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doExit()
{
  instance()->debugger().quit();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PromptWidget *DebuggerDialog::prompt()
{
  return myPrompt;
}
