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
// $Id: DebuggerDialog.cxx,v 1.32 2005-08-10 14:44:01 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "Widget.hxx"
#include "Dialog.hxx"
#include "TabWidget.hxx"
#include "TiaOutputWidget.hxx"
#include "TiaInfoWidget.hxx"
#include "PromptWidget.hxx"
#include "CpuWidget.hxx"
#include "RamWidget.hxx"
#include "TiaWidget.hxx"
#include "CheatWidget.hxx"
#include "Rect.hxx"
#include "Debugger.hxx"

#include "DebuggerDialog.hxx"

enum {
  kDDStepCmd  = 'DDst',
  kDDTraceCmd = 'DDtr',
  kDDAdvCmd   = 'DDav',
  kDDSAdvCmd  = 'DDsv',
  kDDExitCmd  = 'DDex'
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DebuggerDialog::DebuggerDialog(OSystem* osystem, DialogContainer* parent,
                               int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h),
    myTab(NULL)
{
  addTiaArea();
  addTabArea();
  addStatusArea();
  addRomArea();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DebuggerDialog::~DebuggerDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::loadConfig()
{
cerr << " ==> DebuggerDialog::loadConfig()\n";
  myTab->loadConfig();
  myTiaInfo->loadConfig();
  myTiaOutput->loadConfig();
  myCpu->loadConfig();
  myRam->loadConfig();
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
    else if(ascii == 'l')
      doScanlineAdvance();
  }
  else
    Dialog::handleKeyDown(ascii, keycode, modifiers);
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

    case kDDSAdvCmd:
      doScanlineAdvance();
      break;

    case kDDExitCmd:
      doExit();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::addTiaArea()
{
  GUI::Rect r = instance()->debugger().getTiaBounds();

  myTiaOutput = new TiaOutputWidget(this, r.left, r.top, r.width(), r.height());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::addTabArea()
{
  GUI::Rect r = instance()->debugger().getTabBounds();

  const int vBorder = 4;
  const int widWidth  = r.width() - vBorder;
  const int widHeight = r.height() - 25; // FIXME - magic number/font height
  int tabID;

  // The tab widget
  myTab = new TabWidget(this, r.left, r.top + vBorder,
                        r.width(), r.height() - vBorder);
  addTabWidget(myTab);

  // The Prompt/console tab
  tabID = myTab->addTab("Prompt");
  myPrompt = new PromptWidget(myTab, 2, 2, widWidth, widHeight);
  myTab->setParentWidget(tabID, myPrompt);
  addToFocusList(myPrompt->getFocusList(), tabID);

  // The TIA tab
  tabID = myTab->addTab("TIA");
  TiaWidget* tia = new TiaWidget(myTab, 2, 2, widWidth, widHeight);
  myTab->setParentWidget(tabID, tia);
  addToFocusList(tia->getFocusList(), tabID);

  // The input/output tab (part of RIOT)
  tabID = myTab->addTab("I/O");

  // The Cheat tab
  tabID = myTab->addTab("Cheat");
  CheatWidget* cheat = new CheatWidget(myTab, 2, 2, widWidth, widHeight);
  myTab->setParentWidget(tabID, cheat);
  addToFocusList(cheat->getFocusList(), tabID);

  myTab->setActiveTab(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::addStatusArea()
{
  GUI::Rect r = instance()->debugger().getStatusBounds();
  myTiaInfo = new TiaInfoWidget(this, r.left, r.top, r.width(), r.height());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::addRomArea()
{
  GUI::Rect r = instance()->debugger().getRomBounds();
  int xpos, ypos;

  xpos = r.left + 10; ypos = 10;
  myCpu = new CpuWidget(this, instance()->consoleFont(), xpos, ypos);
  addToFocusList(myCpu->getFocusList());

  ypos += myCpu->getHeight() + 10;
  myRam = new RamWidget(this, instance()->consoleFont(), xpos, ypos);
  addToFocusList(myRam->getFocusList());

  // Add some buttons that are always shown, no matter which tab we're in
  // FIXME - these positions will definitely change
  xpos = r.right - 100; ypos = r.bottom - 150;
  addButton(xpos, ypos, "Step", kDDStepCmd, 0);
  ypos += 22;
  addButton(xpos, ypos, "Trace", kDDTraceCmd, 0);
  ypos += 22;
  addButton(xpos, ypos, "Scan +1", kDDSAdvCmd, 0);
  ypos += 22;
  addButton(xpos, ypos, "Frame +1", kDDAdvCmd, 0);

  addButton(xpos, r.bottom - 22 - 10, "Exit", kDDExitCmd, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doStep()
{
  instance()->debugger().step();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doTrace()
{
  instance()->debugger().trace();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doAdvance()
{
  instance()->debugger().nextFrame(1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doScanlineAdvance()
{
  instance()->debugger().nextScanline(1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doExit()
{
  instance()->debugger().quit();
}
