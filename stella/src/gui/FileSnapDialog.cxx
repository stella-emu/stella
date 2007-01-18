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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FileSnapDialog.cxx,v 1.4 2007-01-18 16:45:22 knakos Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifdef _WIN32_WCE
  // different include order for the ce compiler
  #include "FSNode.hxx"
#endif
#include "DialogContainer.hxx"
#include "BrowserDialog.hxx"
#include "TabWidget.hxx"
#include "FSNode.hxx"
#include "bspf.hxx"
#include "LauncherDialog.hxx"
#include "FileSnapDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FileSnapDialog::FileSnapDialog(
      OSystem* osystem, DialogContainer* parent,
      const GUI::Font& font, GuiObject* boss,
      int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h),
    CommandSender(boss),
    myBrowser(NULL),
    myIsGlobal(boss != 0)
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

  // 1) The browser settings tab
  wid.clear();
  tabID = myTab->addTab("Browser Settings");

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

  // ROM settings are disabled while in game mode
  if(!myIsGlobal)
  {
    myTab->disableTab(0);
    // TODO - until I get the above method working, we also need to
    //        disable the specific widgets ourself
    myRomPath->clearFlags(WIDGET_ENABLED);
    for(unsigned int i = 0; i < wid.size(); ++i)
      wid[i]->clearFlags(WIDGET_ENABLED);
  }
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
  myBrowser = new BrowserDialog(this, font, 60, 20, 200, 200);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FileSnapDialog::~FileSnapDialog()
{
  delete myBrowser;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileSnapDialog::loadConfig()
{
  string s;
  bool b;

  s = instance()->settings().getString("romdir");
  myRomPath->setLabel(s);

  b = instance()->settings().getBool("rombrowse");
  myBrowseCheckbox->setState(b);
  myReloadButton->setEnabled(myIsGlobal && !b);

  s = instance()->settings().getString("ssdir");
  mySnapPath->setLabel(s);

  b = instance()->settings().getBool("sssingle");
  mySnapSingleCheckbox->setState(!b);

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileSnapDialog::saveConfig()
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
void FileSnapDialog::openRomBrowser()
{
  parent()->addDialog(myBrowser);

  myBrowser->setTitle("Select ROM directory:");
  myBrowser->setEmitSignal(kRomDirChosenCmd);
  myBrowser->setStartPath(myRomPath->getLabel());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileSnapDialog::openSnapBrowser()
{
  parent()->addDialog(myBrowser);

  myBrowser->setTitle("Select snapshot directory:");
  myBrowser->setEmitSignal(kSnapDirChosenCmd);
  myBrowser->setStartPath(mySnapPath->getLabel());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileSnapDialog::handleCommand(CommandSender* sender, int cmd,
                                   int data, int id)
{
  switch (cmd)
  {
    case kOKCmd:
      saveConfig();
      close();
      if(myIsGlobal)
      {
        sendCommand(kBrowseChangedCmd, 0, 0); // Call this before refreshing ROMs
        sendCommand(kRomDirChosenCmd, 0, 0);  // Let the boss know romdir has changed
      }
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
