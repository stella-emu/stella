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
// $Id: LauncherOptionsDialog.cxx,v 1.14 2006-02-22 17:38:04 stephena Exp $
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
#include "LauncherDialog.hxx"
#include "LauncherOptionsDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LauncherOptionsDialog::LauncherOptionsDialog(
      OSystem* osystem, DialogContainer* parent,
      const GUI::Font& font, GuiObject* boss,
      int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h),
    CommandSender(boss),
    myBrowser(NULL)
{
  const int vBorder = 4;
  int xpos, ypos, bwidth, bheight;

  bwidth  = font.getStringWidth("Cancel") + 20;
  bheight = font.getLineHeight() + 4;

  // The tab widget
  xpos = 2; ypos = vBorder;
  myTab = new TabWidget(this, font, xpos, ypos, _w - 2*xpos, _h - 2*bheight - ypos);

  // 1) The ROM locations tab
  myTab->addTab("ROM Settings");

  // ROM path
  xpos = 15;  ypos += 5;
  new ButtonWidget(myTab, font, xpos, ypos, bwidth, bheight, "Path",
                   kChooseRomDirCmd, 0);
  xpos += bwidth + 20;
  myRomPath = new StaticTextWidget(myTab, font, xpos, ypos + 3,
                                   _w - xpos - 10, font.getLineHeight(),
                                   "", kTextAlignLeft);

  // 2) The snapshot settings tab
  myTab->addTab(" Snapshot Settings ");

  // Snapshot path
  xpos = 15;
  new ButtonWidget(myTab, font, xpos, ypos, bwidth, bheight, "Path",
                   kChooseSnapDirCmd, 0);
  xpos += bwidth + 20;
  mySnapPath = new StaticTextWidget(myTab, font, xpos, ypos + 3,
                                    _w - xpos - 10, font.getLineHeight(),
                                    "", kTextAlignLeft);

  // Snapshot save name
  xpos = 10; ypos += mySnapPath->getHeight() + 8;
  mySnapTypePopup = new PopUpWidget(myTab, font, xpos, ypos,
                                    font.getStringWidth("romname"),
                                    font.getLineHeight(),
                                    "Save snapshot as: ",
                                    font.getStringWidth("Save snapshot as: "), 0);
  mySnapTypePopup->appendEntry("romname", 1);
  mySnapTypePopup->appendEntry("md5sum", 2);

  // Snapshot single or multiple saves
  xpos = 30;  ypos += mySnapTypePopup->getHeight() + 8;
  mySnapSingleCheckbox = new CheckboxWidget(myTab, font, xpos, ypos,
                                            "Multiple snapshots");

  // Activate the first tab
  myTab->setActiveTab(0);

  // Add OK & Cancel buttons
#ifndef MAC_OSX
  xpos = _w - 2 *(bwidth + 10);  ypos = _h - bheight - 8;
  new ButtonWidget(this, font, xpos, ypos, bwidth, bheight, "OK",
                   kOKCmd, 0);
  xpos += bwidth + 10;
  new ButtonWidget(this, font, xpos, ypos, bwidth, bheight, "Cancel",
                   kCloseCmd, 0);
#else
  xpos = _w - 2 *(bwidth + 10);  ypos = _h - bheight - 8;
  new ButtonWidget(this, font, xpos, ypos, bwidth, bheight, "Cancel",
                   kCloseCmd, 0);
  xpos += bwidth + 10;
  new ButtonWidget(this, font, xpos, ypos, bwidth, bheight, "OK",
                   kOKCmd, 0);
#endif

  // Create file browser dialog
  int baseW = instance()->frameBuffer().baseWidth();
  int baseH = instance()->frameBuffer().baseHeight();
  myBrowser = new BrowserDialog(this, font, 60, 20, baseW - 120, baseH - 40);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LauncherOptionsDialog::~LauncherOptionsDialog()
{
  delete myBrowser;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherOptionsDialog::loadConfig()
{
  string s;
  bool b;

  s = instance()->settings().getString("romdir");
  myRomPath->setLabel(s);

  s = instance()->settings().getString("ssdir");
  mySnapPath->setLabel(s);

  s = instance()->settings().getString("ssname");
  if(s == "romname")
    mySnapTypePopup->setSelectedTag(1);
  else if(s == "md5sum")
    mySnapTypePopup->setSelectedTag(2);
  else
    mySnapTypePopup->setSelectedTag(0);

  b = instance()->settings().getBool("sssingle");
  mySnapSingleCheckbox->setState(!b);

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherOptionsDialog::saveConfig()
{
  string s;
  bool b;

  s  = myRomPath->getLabel();
  instance()->settings().setString("romdir", s);

  s = mySnapPath->getLabel();
  instance()->settings().setString("ssdir", s);

  s = mySnapTypePopup->getSelectedString();
  instance()->settings().setString("ssname", s);

  b = mySnapSingleCheckbox->getState();
  instance()->settings().setBool("sssingle", !b);

  // Flush changes to disk
  instance()->settings().saveConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherOptionsDialog::openRomBrowser()
{
  myBrowser->setTitle("Select ROM directory:");
  myBrowser->setEmitSignal(kRomDirChosenCmd);
  myBrowser->setStartPath(myRomPath->getLabel());

  parent()->addDialog(myBrowser);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherOptionsDialog::openSnapBrowser()
{
  myBrowser->setTitle("Select snapshot directory:");
  myBrowser->setEmitSignal(kSnapDirChosenCmd);
  myBrowser->setStartPath(mySnapPath->getLabel());

  parent()->addDialog(myBrowser);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherOptionsDialog::handleCommand(CommandSender* sender, int cmd,
                                          int data, int id)
{
  switch (cmd)
  {
    case kOKCmd:
      saveConfig();
      close();
      sendCommand(kRomDirChosenCmd, 0, 0);  // Let the boss know romdir has changed
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
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
