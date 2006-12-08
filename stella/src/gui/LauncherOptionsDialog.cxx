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
// Copyright (c) 1995-2006 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: LauncherOptionsDialog.cxx,v 1.20 2006-12-08 16:49:35 stephena Exp $
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
  WidgetArray wid;
  ButtonWidget* b;
  int tabID;

  bwidth  = font.getStringWidth("Cancel") + 20;
  bheight = font.getLineHeight() + 4;

  // The tab widget
  xpos = 2; ypos = vBorder;
  myTab = new TabWidget(this, font, xpos, ypos, _w - 2*xpos, _h - 2*bheight - ypos);
  addTabWidget(myTab);

  // 1) The ROM locations tab
  wid.clear();
  tabID = myTab->addTab("ROM Settings");

  // ROM path
  xpos = 15;  ypos += 5;
  b = new ButtonWidget(myTab, font, xpos, ypos, bwidth, bheight, "Path",
                       kChooseRomDirCmd);
  wid.push_back(b);
  xpos += bwidth + 20;
  myRomPath = new StaticTextWidget(myTab, font, xpos, ypos + 3,
                                   _w - xpos - 10, font.getLineHeight(),
                                   "", kTextAlignLeft);

  // Use ROM browse mode
  xpos = 30;  ypos += myRomPath->getHeight() + 8;
  myBrowseCheckbox = new CheckboxWidget(myTab, font, xpos, ypos,
                                        "Browse folders", kBrowseDirCmd);
  wid.push_back(myBrowseCheckbox);

  // Reload current ROM listing
  xpos += myBrowseCheckbox->getWidth() + 20;
  myReloadButton = new ButtonWidget(myTab, font, xpos, ypos-2,
                                    font.getStringWidth("Reload ROM Listing") + 20,
                                    bheight,
                                    "Reload ROM Listing", kReloadRomDirCmd);
//  myReloadButton->setEditable(true);
  wid.push_back(myReloadButton); 

  // Add focus widgets for ROM tab
  addToFocusList(wid, tabID);

  // 2) The snapshot settings tab
  wid.clear();
  tabID = myTab->addTab(" Snapshot Settings ");

  // Snapshot path
  xpos = 15;  ypos = vBorder + 5;
  b = new ButtonWidget(myTab, font, xpos, ypos, bwidth, bheight, "Path",
                       kChooseSnapDirCmd);
  wid.push_back(b);
  xpos += bwidth + 20;
  mySnapPath = new StaticTextWidget(myTab, font, xpos, ypos + 3,
                                    _w - xpos - 10, font.getLineHeight(),
                                    "", kTextAlignLeft);

  // Snapshot single or multiple saves
  xpos = 30;  ypos += mySnapPath->getHeight() + 8;
  mySnapSingleCheckbox = new CheckboxWidget(myTab, font, xpos, ypos,
                                            "Multiple snapshots");
  wid.push_back(mySnapSingleCheckbox);

  // Add focus widgets for Snapshot tab
  addToFocusList(wid, tabID);

  // Activate the first tab
  myTab->setActiveTab(0);

  // Add OK & Cancel buttons
  wid.clear();
#ifndef MAC_OSX
  xpos = _w - 2 *(bwidth + 10);  ypos = _h - bheight - 8;
  b = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight, "OK", kOKCmd);
  wid.push_back(b);
  addOKWidget(b);
  xpos += bwidth + 10;
  b = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight, "Cancel", kCloseCmd);
  wid.push_back(b);
  addCancelWidget(b);
#else
  xpos = _w - 2 *(bwidth + 10);  ypos = _h - bheight - 8;
  b = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight, "Cancel", kCloseCmd);
  wid.push_back(b);
  addCancelWidget(b);
  xpos += bwidth + 10;
  b = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight, "OK", kOKCmd);
  wid.push_back(b);
  addOKWidget(b);
#endif
  // Add focus widgets for OK/Cancel buttons
  addToFocusList(wid);

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

  b = instance()->settings().getBool("rombrowse");
  myBrowseCheckbox->setState(b);
  myReloadButton->setEnabled(!b);

  s = instance()->settings().getString("ssdir");
  mySnapPath->setLabel(s);

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

  b = myBrowseCheckbox->getState();
  instance()->settings().setBool("rombrowse", b);

  s = mySnapPath->getLabel();
  instance()->settings().setString("ssdir", s);

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
      sendCommand(kBrowseChangedCmd, 0, 0); // Call this before refreshing ROMs
      sendCommand(kRomDirChosenCmd, 0, 0);  // Let the boss know romdir has changed
      break;

    case kChooseRomDirCmd:
      openRomBrowser();
      break;

    case kChooseSnapDirCmd:
      openSnapBrowser();
      break;

/*
    case kBrowseDirCmd:
      myReloadButton->setEnabled(!myBrowseCheckbox->getState());
      break;
*/

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

    case kReloadRomDirCmd:
    {
      sendCommand(kReloadRomDirCmd, 0, 0);
      break;
    }

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
