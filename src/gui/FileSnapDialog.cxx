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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
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
            buttonWidth  = font.getStringWidth("Properties file:") + 20,
            buttonHeight = font.getLineHeight() + 4;
  const int vBorder = 8;
  int xpos, ypos;
  WidgetArray wid;
  ButtonWidget* b;

  // Set real dimensions
  _w = 52 * fontWidth + 8;
  _h = 12 * (lineHeight + 4) + 10;

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

  // Snapshot path
  xpos = vBorder;  ypos += romButton->getHeight() + 3;
  b = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                       "Snapshot path:", kChooseSnapDirCmd);
  wid.push_back(b);
  xpos += buttonWidth + 10;
  mySnapPath = new EditTextWidget(this, font, xpos, ypos + 2,
                                  _w - xpos - 10, lineHeight, "");
  wid.push_back(mySnapPath);

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

  // EEPROM directory
  xpos = vBorder;  ypos += b->getHeight() + 3;
  b = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                       "EEPROM path:", kChooseEEPROMDirCmd);
  wid.push_back(b);
  xpos += buttonWidth + 10;
  myEEPROMPath = new EditTextWidget(this, font, xpos, ypos + 2,
                                    _w - xpos - 10, lineHeight, "");
  wid.push_back(myEEPROMPath);

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
  StringMap items;
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
  myRomPath->setEditString(settings.getString("romdir"));
  mySnapPath->setEditString(settings.getString("snapdir"));
  myCheatFile->setEditString(settings.getString("cheatfile"));
  myPaletteFile->setEditString(settings.getString("palettefile"));
  myPropsFile->setEditString(settings.getString("propsfile"));
  myEEPROMPath->setEditString(settings.getString("eepromdir"));
  myStatePath->setEditString(settings.getString("statedir"));
  mySnapSingle->setState(settings.getBool("sssingle"));
  mySnap1x->setState(settings.getBool("ss1x"));
  mySnapInterval->setSelected(instance().settings().getString("ssinterval"), "2");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileSnapDialog::saveConfig()
{
  instance().settings().setString("romdir", myRomPath->getEditString());
  instance().settings().setString("snapdir", mySnapPath->getEditString());
  instance().settings().setString("cheatfile", myCheatFile->getEditString());
  instance().settings().setString("palettefile", myPaletteFile->getEditString());
  instance().settings().setString("propsfile", myPropsFile->getEditString());
  instance().settings().setString("statedir", myStatePath->getEditString());
  instance().settings().setString("eepromdir", myEEPROMPath->getEditString());
  instance().settings().setBool("sssingle", mySnapSingle->getState());
  instance().settings().setBool("ss1x", mySnap1x->getState());
  instance().settings().setString("ssinterval", mySnapInterval->getSelectedTag());

  // Flush changes to disk and inform the OSystem
  instance().settings().saveConfig();
  instance().setConfigPaths();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileSnapDialog::setDefaults()
{
  FilesystemNode node;
  const string& basedir = instance().baseDir();

  node = FilesystemNode("~");
  myRomPath->setEditString(node.getPath(false));

  mySnapPath->setEditString(instance().defaultSnapDir());
 
  const string& cheatfile = basedir + "stella.cht";
  node = FilesystemNode(cheatfile);
  myCheatFile->setEditString(node.getPath(false));

  const string& palettefile = basedir + "stella.pal";
  node = FilesystemNode(palettefile);
  myPaletteFile->setEditString(node.getPath(false));

  const string& propsfile = basedir + "stella.pro";
  node = FilesystemNode(propsfile);
  myPropsFile->setEditString(node.getPath(false));

  const string& eepromdir = basedir;
  node = FilesystemNode(eepromdir);
  myEEPROMPath->setEditString(node.getPath(false));

  const string& statedir = basedir + "state";
  node = FilesystemNode(statedir);
  myStatePath->setEditString(node.getPath(false));

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
      myBrowser->show("Select ROM directory:", myRomPath->getEditString(),
                      FilesystemNode::kListDirectoriesOnly, kRomDirChosenCmd);
      break;

    case kChooseSnapDirCmd:
      myBrowser->show("Select snapshot directory:", mySnapPath->getEditString(),
                      FilesystemNode::kListDirectoriesOnly, kSnapDirChosenCmd);
      break;

    case kChooseCheatFileCmd:
      myBrowser->show("Select cheat file:", myCheatFile->getEditString(),
                      FilesystemNode::kListAll, kCheatFileChosenCmd);
      break;

    case kChoosePaletteFileCmd:
      myBrowser->show("Select palette file:", myPaletteFile->getEditString(),
                      FilesystemNode::kListAll, kPaletteFileChosenCmd);
      break;

    case kChoosePropsFileCmd:
      myBrowser->show("Select properties file:", myPropsFile->getEditString(),
                      FilesystemNode::kListAll, kPropsFileChosenCmd);
      break;

    case kChooseEEPROMDirCmd:
      myBrowser->show("Select EEPROM directory:", myEEPROMPath->getEditString(),
                      FilesystemNode::kListDirectoriesOnly, kEEPROMDirChosenCmd);
      break;

    case kChooseStateDirCmd:
      myBrowser->show("Select state directory:", myStatePath->getEditString(),
                      FilesystemNode::kListDirectoriesOnly, kStateDirChosenCmd);
      break;

    case kRomDirChosenCmd:
    {
      FilesystemNode dir(myBrowser->getResult());
      myRomPath->setEditString(dir.getPath(false));
      break;
    }

    case kSnapDirChosenCmd:
    {
      FilesystemNode dir(myBrowser->getResult());
      mySnapPath->setEditString(dir.getPath(false));
      break;
    }

    case kCheatFileChosenCmd:
    {
      FilesystemNode dir(myBrowser->getResult());
      myCheatFile->setEditString(dir.getPath(false));
      break;
    }

    case kPaletteFileChosenCmd:
    {
      FilesystemNode dir(myBrowser->getResult());
      myPaletteFile->setEditString(dir.getPath(false));
      break;
    }

    case kPropsFileChosenCmd:
    {
      FilesystemNode dir(myBrowser->getResult());
      myPropsFile->setEditString(dir.getPath(false));
      break;
    }

    case kEEPROMDirChosenCmd:
    {
      FilesystemNode dir(myBrowser->getResult());
      myEEPROMPath->setEditString(dir.getPath(false));
      break;
    }

    case kStateDirChosenCmd:
    {
      FilesystemNode dir(myBrowser->getResult());
      myStatePath->setEditString(dir.getPath(false));
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
