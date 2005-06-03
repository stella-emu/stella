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
// $Id: DebuggerDialog.cxx,v 1.1 2005-06-03 17:52:06 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "DialogContainer.hxx"
#include "BrowserDialog.hxx"
#include "PopUpWidget.hxx"
#include "TabWidget.hxx"
#include "FSNode.hxx"
#include "bspf.hxx"
#include "DebuggerDialog.hxx"

enum {
  kChooseRomDirCmd  = 'roms',  // rom select
  kChooseSnapDirCmd = 'snps',  // snap select
  kRomDirChosenCmd  = 'romc',  // rom chosen
  kSnapDirChosenCmd = 'snpc'   // snap chosen
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DebuggerDialog::DebuggerDialog(
      OSystem* osystem, DialogContainer* parent,
      int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h)
{
  const int vBorder = 4;
  int yoffset;

  // The tab widget
  TabWidget* tab = new TabWidget(this, 0, vBorder, _w, _h);

  // 1) The command prompt
  tab->addTab("Prompt");
  yoffset = vBorder;

  // FIXME - add scrollable console/edittext window

  // 2) The CPU contents
  tab->addTab(" CPU ");
  yoffset = vBorder;

  // FIXME - add CPU registers

  // 3) The RAM contents
  tab->addTab(" RAM ");
  yoffset = vBorder;

  // FIXME - add 16x8 list of RAM contents
/*
  // Snapshot path
  new ButtonWidget(tab, 15, yoffset, kButtonWidth + 14, 16, "Path", kChooseSnapDirCmd, 0);
  mySnapPath = new StaticTextWidget(tab, 5 + kButtonWidth + 30,
                                    yoffset + 3, _w - (5 + kButtonWidth + 20) - 10,
                                    kLineHeight, "", kTextAlignLeft);
  yoffset += 22;

  // Snapshot save name
  mySnapTypePopup = new PopUpWidget(tab, 10, yoffset, 140, kLineHeight,
                                    "Save snapshot as: ", 87, 0);
  mySnapTypePopup->appendEntry("romname", 1);
  mySnapTypePopup->appendEntry("md5sum", 2);
  yoffset += 18;

  // Snapshot single or multiple saves
  mySnapSingleCheckbox = new CheckboxWidget(tab, 30, yoffset, 80, kLineHeight,
                                            "Multiple snapshots");
*/

  // 4) The ROM contents
  tab->addTab(" ROM ");
  yoffset = vBorder;

  // FIXME - add ROM contents, somehow taking into account which bank is being
  //         viewed (maybe a button to switch banks??

  // 5) The TIA contents
  tab->addTab(" TIA ");
  yoffset = vBorder;

  // FIXME - TIA registers

  // 6) The code listing
  tab->addTab(" Code");
  yoffset = vBorder;

  // FIXME - Add code listing in assembly language

  // Activate the first tab
  tab->setActiveTab(0);

/*
  // Create file browser dialog
  int baseW = instance()->frameBuffer().baseWidth();
  int baseH = instance()->frameBuffer().baseHeight();
  myBrowser = new BrowserDialog(this, 60, 20, baseW - 120, baseH - 40);
*/
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
void DebuggerDialog::saveConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::openRomBrowser()
{
/*
  myBrowser->setTitle("Select ROM directory:");
  myBrowser->setEmitSignal(kRomDirChosenCmd);
  myBrowser->setStartPath(myRomPath->getLabel());

  parent()->addDialog(myBrowser);
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::openSnapBrowser()
{
/*
  myBrowser->setTitle("Select snapshot directory:");
  myBrowser->setEmitSignal(kSnapDirChosenCmd);
  myBrowser->setStartPath(mySnapPath->getLabel());

  parent()->addDialog(myBrowser);
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::handleCommand(CommandSender* sender, int cmd, int data)
{
  switch (cmd)
  {
    case kOKCmd:
      saveConfig();
      close();
      break;

    case kChooseRomDirCmd:
      openRomBrowser();
      break;

    case kChooseSnapDirCmd:
      openSnapBrowser();
      break;

    case kRomDirChosenCmd:
    {
      FilesystemNode dir(myBrowser->getResult());
      myRomPath->setLabel(dir.path());
      break;
    }

    case kSnapDirChosenCmd:
    {
      FilesystemNode dir(myBrowser->getResult());
      mySnapPath->setLabel(dir.path());
      break;
    }

    default:
      Dialog::handleCommand(sender, cmd, data);
      break;
  }
}
