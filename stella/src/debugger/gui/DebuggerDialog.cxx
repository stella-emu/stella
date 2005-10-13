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
// $Id: DebuggerDialog.cxx,v 1.8 2005-10-13 18:53:07 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "Widget.hxx"
#include "Dialog.hxx"
#include "TabWidget.hxx"
#include "TiaInfoWidget.hxx"
#include "TiaOutputWidget.hxx"
#include "TiaZoomWidget.hxx"
#include "AudioWidget.hxx"
#include "PromptWidget.hxx"
#include "CpuWidget.hxx"
#include "RamWidget.hxx"
#include "RomWidget.hxx"
#include "TiaWidget.hxx"
#include "DataGridOpsWidget.hxx"
#include "EditTextWidget.hxx"
#include "Rect.hxx"
#include "Debugger.hxx"
#include "DebuggerParser.hxx"

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

  // Inform the output widget about its associated zoom widget
  myTiaOutput->setZoomWidget(myTiaZoom);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DebuggerDialog::~DebuggerDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::loadConfig()
{
  myTab->loadConfig();
  myTiaInfo->loadConfig();
  myTiaOutput->loadConfig();
  myTiaZoom->loadConfig();
  myCpu->loadConfig();
  myRam->loadConfig();
  myRom->loadConfig();

  myMessageBox->setEditString("");
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
      Dialog::handleCommand(sender, cmd, data, id);
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

  // The Audio tab
  tabID = myTab->addTab("Audio");
  AudioWidget* aud = new AudioWidget(myTab, 2, 2, widWidth, widHeight);
  myTab->setParentWidget(tabID, aud);
  addToFocusList(aud->getFocusList(), tabID);

  // The input/output tab (part of RIOT)
//  tabID = myTab->addTab("I/O");

  myTab->setActiveTab(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::addStatusArea()
{
  const GUI::Font& font = instance()->consoleFont();
  GUI::Rect r = instance()->debugger().getStatusBounds();
  int xpos, ypos;

  xpos = r.left;  ypos = r.top;
  myTiaInfo = new TiaInfoWidget(this, xpos+20, ypos);

  ypos += myTiaInfo->getHeight() + 10;
  myTiaZoom = new TiaZoomWidget(this, xpos+10, ypos);
  addToFocusList(myTiaZoom->getFocusList());

  xpos += 10;  ypos += myTiaZoom->getHeight() + 20;
  myMessageBox = new EditTextWidget(this, xpos, ypos, myTiaZoom->getWidth(),
                                    font.getLineHeight(), "");
  myMessageBox->setFont(font);
  myMessageBox->setEditable(false);
  myMessageBox->setColor(kTextColorEm);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::addRomArea()
{
  GUI::Rect r = instance()->debugger().getRomBounds();
  int xpos, ypos;

  xpos = r.left + 10;  ypos = 10;
  myCpu = new CpuWidget(this, instance()->consoleFont(), xpos, ypos);
  addToFocusList(myCpu->getFocusList());

  xpos = r.left + 10;  ypos += myCpu->getHeight() + 10;
  myRam = new RamWidget(this, instance()->consoleFont(), xpos, ypos);
  addToFocusList(myRam->getFocusList());

  xpos = r.left + 10 + myCpu->getWidth() + 20;
  DataGridOpsWidget* ops = new DataGridOpsWidget(this, xpos, 20);
  ops->setEnabled(false);

  int buttonX = r.right - kButtonWidth - 5, buttonY = r.top + 5;
  addButton(buttonX, buttonY, "Step", kDDStepCmd, 0);
  buttonY += 22;
  addButton(buttonX, buttonY, "Trace", kDDTraceCmd, 0);
  buttonY += 22;
  addButton(buttonX, buttonY, "Scan +1", kDDSAdvCmd, 0);
  buttonY += 22;
  addButton(buttonX, buttonY, "Frame +1", kDDAdvCmd, 0);
  buttonY += 22;
  addButton(buttonX, buttonY, "Exit", kDDExitCmd, 0);

  xpos = r.left + 10;  ypos += myRam->getHeight() + 15;
  myRom = new RomWidget(this, instance()->consoleFont(), xpos, ypos);
  addToFocusList(myRom->getFocusList());

  // Add the DataGridOpsWidget to any widgets which contain a
  // DataGridWidget which we want controlled
  myCpu->setOpsWidget(ops);
  myRam->setOpsWidget(ops);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doStep()
{
  instance()->debugger().parser()->run("step");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doTrace()
{
  instance()->debugger().parser()->run("trace");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doAdvance()
{
  instance()->debugger().parser()->run("frame #1");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doScanlineAdvance()
{
  instance()->debugger().parser()->run("scanline #1");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doExit()
{
  instance()->debugger().quit();
}
