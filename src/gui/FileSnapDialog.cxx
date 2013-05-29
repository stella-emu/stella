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
// Copyright (c) 1995-2013 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "bspf.hxx"

#include "BrowserDialog.hxx"
#include "EditTextWidget.hxx"
#include "FSNode.hxx"
#include "LauncherDialog.hxx"
#include "PopUpWidget.hxx"
#include "Settings.hxx"

#include "FileSnapDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FileSnapDialog::FileSnapDialog(
      OSystem* osystem, DialogContainer* parent,
      const GUI::Font& font, GuiObject* boss,
      int max_w, int max_h)
  : Dialog(osystem, parent, 0, 0, 0, 0),
    CommandSender(boss),
    myBrowser(NULL),
    myIsGlobal(boss != 0)
{
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            buttonWidth  = font.getStringWidth("Snapshot load path:") + 20,
            buttonHeight = font.getLineHeight() + 4;
  const int vBorder = 8;
  int xpos, ypos;
  WidgetArray wid;
  ButtonWidget* b;

  // Set real dimensions
  _w = 56 * fontWidth + 8;
  _h = 13 * (lineHeight + 4) + 10;

  xpos = vBorder;  ypos = vBorder;

  // ROM path
  ButtonWidget* romButton = 
    new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                     "Rom path:", kChooseRomDirCmd);
  wid.push_back(romButton);
  xpos += buttonWidth + 10;
  myRomPath = new EditTextWidget(this, font, xpos, ypos + 2,
                                 _w - xpos - 10, lineHeight, "");
  wid.push_back(myRomPath);

  // Snapshot path (save files)
  xpos = vBorder;  ypos += romButton->getHeight() + 3;
  b = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                       "Snapshot save path:", kChooseSnapSaveDirCmd);
  wid.push_back(b);
  xpos += buttonWidth + 10;
  mySnapSavePath = new EditTextWidget(this, font, xpos, ypos + 2,
                                  _w - xpos - 10, lineHeight, "");
  wid.push_back(mySnapSavePath);

  // Snapshot path (load files)
  xpos = vBorder;  ypos += romButton->getHeight() + 3;
  b = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                       "Snapshot load path:", kChooseSnapLoadDirCmd);
  wid.push_back(b);
  xpos += buttonWidth + 10;
  mySnapLoadPath = new EditTextWidget(this, font, xpos, ypos + 2,
                                  _w - xpos - 10, lineHeight, "");
  wid.push_back(mySnapLoadPath);

  // Cheat file
  xpos = vBorder;  ypos += b->getHeight() + 3;
  b = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                       "Cheat file:", kChooseCheatFileCmd);
  wid.push_back(b);
  xpos += buttonWidth + 10;
  myCheatFile = new EditTextWidget(this, font, xpos, ypos + 2,
                                   _w - xpos - 10, lineHeight, "");
  wid.push_back(myCheatFile);

  // Palette file
  xpos = vBorder;  ypos += b->getHeight() + 3;
  b = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                       "Palette file:", kChoosePaletteFileCmd);
  wid.push_back(b);
  xpos += buttonWidth + 10;
  myPaletteFile = new EditTextWidget(this, font, xpos, ypos + 2,
                                     _w - xpos - 10, lineHeight, "");
  wid.push_back(myPaletteFile);

  // Properties file
  xpos = vBorder;  ypos += b->getHeight() + 3;
  b = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                       "Properties file:", kChoosePropsFileCmd);
  wid.push_back(b);
  xpos += buttonWidth + 10;
  myPropsFile = new EditTextWidget(this, font, xpos, ypos + 2,
                                   _w - xpos - 10, lineHeight, "");
  wid.push_back(myPropsFile);

  // State directory
  xpos = vBorder;  ypos += b->getHeight() + 3;
  b = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                       "State path:", kChooseStateDirCmd);
  wid.push_back(b);
  xpos += buttonWidth + 10;
  myStatePath = new EditTextWidget(this, font, xpos, ypos + 2,
                                   _w - xpos - 10, lineHeight, "");
  wid.push_back(myStatePath);

  // NVRAM directory
  xpos = vBorder;  ypos += b->getHeight() + 3;
  b = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                       "NVRAM path:", kChooseNVRamDirCmd);
  wid.push_back(b);
  xpos += buttonWidth + 10;
  myNVRamPath = new EditTextWidget(this, font, xpos, ypos + 2,
                                   _w - xpos - 10, lineHeight, "");
  wid.push_back(myNVRamPath);

  // Snapshot single or multiple saves
  xpos = 30;  ypos += b->getHeight() + 5;
  mySnapSingle = new CheckboxWidget(this, font, xpos, ypos,
                                    "Overwrite snapshots");
  wid.push_back(mySnapSingle);

  // Snapshot in 1x mode (ignore scaling)
  xpos += mySnapSingle->getWidth() + 20;
  mySnap1x = new CheckboxWidget(this, font, xpos, ypos,
                                "Snapshot in 1x mode");
  wid.push_back(mySnap1x);

  // Snapshot interval (continuous mode)
  VariantList items;
  items.clear();
  items.push_back("1 second", "1");
  items.push_back("2 seconds", "2");
  items.push_back("3 seconds", "3");
  items.push_back("4 seconds", "4");
  items.push_back("5 seconds", "5");
  items.push_back("6 seconds", "6");
  items.push_back("7 seconds", "7");
  items.push_back("8 seconds", "8");
  items.push_back("9 seconds", "9");
  items.push_back("10 seconds", "10");
  xpos = 30;  ypos += b->getHeight();
  mySnapInterval = new PopUpWidget(this, font, xpos, ypos,
                                   font.getStringWidth("10 seconds"), lineHeight,
                                   items, "Continuous snapshot interval: ");
  wid.push_back(mySnapInterval);

  // Add Defaults, OK and Cancel buttons
  b = new ButtonWidget(this, font, 10, _h - buttonHeight - 10,
                       font.getStringWidth("Defaults") + 20, buttonHeight,
                       "Defaults", kDefaultsCmd);
  wid.push_back(b);
  addOKCancelBGroup(wid, font);

  addToFocusList(wid);

  // All ROM settings are disabled while in game mode
  if(!myIsGlobal)
  {
    romButton->clearFlags(WIDGET_ENABLED);
    myRomPath->setEditable(false);
  }

  // Create file browser dialog
  myBrowser = new BrowserDialog(this, font, max_w, max_h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FileSnapDialog::~FileSnapDialog()
{
  delete myBrowser;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileSnapDialog::loadConfig()
{
  const Settings& settings = instance().settings();
  myRomPath->setText(settings.getString("romdir"));
  mySnapSavePath->setText(settings.getString("snapsavedir"));
  mySnapLoadPath->setText(settings.getString("snaploaddir"));
  myCheatFile->setText(settings.getString("cheatfile"));
  myPaletteFile->setText(settings.getString("palettefile"));
  myPropsFile->setText(settings.getString("propsfile"));
  myNVRamPath->setText(settings.getString("nvramdir"));
  myStatePath->setText(settings.getString("statedir"));
  mySnapSingle->setState(settings.getBool("sssingle"));
  mySnap1x->setState(settings.getBool("ss1x"));
  mySnapInterval->setSelected(instance().settings().getString("ssinterval"), "2");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileSnapDialog::saveConfig()
{
  instance().settings().setValue("romdir", myRomPath->getText());
  instance().settings().setValue("snapsavedir", mySnapSavePath->getText());
  instance().settings().setValue("snaploaddir", mySnapLoadPath->getText());
  instance().settings().setValue("cheatfile", myCheatFile->getText());
  instance().settings().setValue("palettefile", myPaletteFile->getText());
  instance().settings().setValue("propsfile", myPropsFile->getText());
  instance().settings().setValue("statedir", myStatePath->getText());
  instance().settings().setValue("nvramdir", myNVRamPath->getText());
  instance().settings().setValue("sssingle", mySnapSingle->getState());
  instance().settings().setValue("ss1x", mySnap1x->getState());
  instance().settings().setValue("ssinterval",
    mySnapInterval->getSelectedTag().toString());

  // Flush changes to disk and inform the OSystem
  instance().saveConfig();
  instance().setConfigPaths();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileSnapDialog::setDefaults()
{
  FilesystemNode node;
  const string& basedir = instance().baseDir();

  node = FilesystemNode("~");
  myRomPath->setText(node.getShortPath());

  mySnapSavePath->setText(instance().defaultSnapSaveDir());
  mySnapLoadPath->setText(instance().defaultSnapLoadDir());
 
  const string& cheatfile = basedir + "stella.cht";
  node = FilesystemNode(cheatfile);
  myCheatFile->setText(node.getShortPath());

  const string& palettefile = basedir + "stella.pal";
  node = FilesystemNode(palettefile);
  myPaletteFile->setText(node.getShortPath());

  const string& propsfile = basedir + "stella.pro";
  node = FilesystemNode(propsfile);
  myPropsFile->setText(node.getShortPath());

  const string& nvramdir = basedir + "nvram";
  node = FilesystemNode(nvramdir);
  myNVRamPath->setText(node.getShortPath());

  const string& statedir = basedir + "state";
  node = FilesystemNode(statedir);
  myStatePath->setText(node.getShortPath());

  mySnapSingle->setState(false);
  mySnap1x->setState(false);
  mySnapInterval->setSelected("2", "2");
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
        sendCommand(kRomDirChosenCmd, 0, 0);  // Let the boss know romdir has changed
      break;

    case kDefaultsCmd:
      setDefaults();
      break;

    case kChooseRomDirCmd:
      myBrowser->show("Select ROM directory:", myRomPath->getText(),
                      BrowserDialog::Directories, kRomDirChosenCmd);
      break;

    case kChooseSnapSaveDirCmd:
      myBrowser->show("Select snapshot save directory:", mySnapSavePath->getText(),
                      BrowserDialog::Directories, kSnapSaveDirChosenCmd);
      break;

    case kChooseSnapLoadDirCmd:
      myBrowser->show("Select snapshot load directory:", mySnapLoadPath->getText(),
                      BrowserDialog::Directories, kSnapLoadDirChosenCmd);
      break;

    case kChooseCheatFileCmd:
      myBrowser->show("Select cheat file:", myCheatFile->getText(),
                      BrowserDialog::FileLoad, kCheatFileChosenCmd);
      break;

    case kChoosePaletteFileCmd:
      myBrowser->show("Select palette file:", myPaletteFile->getText(),
                      BrowserDialog::FileLoad, kPaletteFileChosenCmd);
      break;

    case kChoosePropsFileCmd:
      myBrowser->show("Select properties file:", myPropsFile->getText(),
                      BrowserDialog::FileLoad, kPropsFileChosenCmd);
      break;

    case kChooseNVRamDirCmd:
      myBrowser->show("Select NVRAM directory:", myNVRamPath->getText(),
                      BrowserDialog::Directories, kNVRamDirChosenCmd);
      break;

    case kChooseStateDirCmd:
      myBrowser->show("Select state directory:", myStatePath->getText(),
                      BrowserDialog::Directories, kStateDirChosenCmd);
      break;

    case kRomDirChosenCmd:
    {
      myRomPath->setText(myBrowser->getResult().getShortPath());
      break;
    }

    case kSnapSaveDirChosenCmd:
    {
      mySnapSavePath->setText(myBrowser->getResult().getShortPath());
      break;
    }

    case kSnapLoadDirChosenCmd:
    {
      mySnapLoadPath->setText(myBrowser->getResult().getShortPath());
      break;
    }

    case kCheatFileChosenCmd:
    {
      myCheatFile->setText(myBrowser->getResult().getShortPath());
      break;
    }

    case kPaletteFileChosenCmd:
    {
      myPaletteFile->setText(myBrowser->getResult().getShortPath());
      break;
    }

    case kPropsFileChosenCmd:
    {
      myPropsFile->setText(myBrowser->getResult().getShortPath());
      break;
    }

    case kNVRamDirChosenCmd:
    {
      myNVRamPath->setText(myBrowser->getResult().getShortPath());
      break;
    }

    case kStateDirChosenCmd:
    {
      myStatePath->setText(myBrowser->getResult().getShortPath());
      break;
    }

    case kReloadRomDirCmd:
      sendCommand(kReloadRomDirCmd, 0, 0);	 
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
