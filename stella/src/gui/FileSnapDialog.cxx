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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FileSnapDialog.cxx,v 1.21 2008-12-25 23:05:16 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "bspf.hxx"

#include "BrowserDialog.hxx"
#include "DialogContainer.hxx"
#include "EditTextWidget.hxx"
#include "FSNode.hxx"
#include "LauncherDialog.hxx"
#include "Settings.hxx"

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
  const int lineHeight   = font.getLineHeight(),
            buttonWidth  = font.getStringWidth("Properties file:") + 20,
            buttonHeight = font.getLineHeight() + 4;
  const int vBorder = 8;
  int xpos, ypos;
  WidgetArray wid;
  ButtonWidget* b;

  // Set real dimensions
//  _w = 50 * fontWidth + 10;
//  _h = 11 * (lineHeight + 4) + 10;

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

  // State directory
  xpos = vBorder;  ypos += romButton->getHeight() + 3;
  b = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                       "State path:", kChooseStateDirCmd);
  wid.push_back(b);
  xpos += buttonWidth + 10;
  myStatePath = new EditTextWidget(this, font, xpos, ypos + 2,
                                   _w - xpos - 10, lineHeight, "");
  wid.push_back(myStatePath);

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

  // Snapshot path
  xpos = vBorder;  ypos += b->getHeight() + 3;
  b = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                       "Snapshot path:", kChooseSnapDirCmd);
  wid.push_back(b);
  xpos += buttonWidth + 10;
  mySnapPath = new EditTextWidget(this, font, xpos, ypos + 2,
                                  _w - xpos - 10, lineHeight, "");
  wid.push_back(mySnapPath);

  // Snapshot single or multiple saves
  xpos = 30;  ypos += b->getHeight() + 5;
  mySnapSingleCheckbox = new CheckboxWidget(this, font, xpos, ypos,
                                            "Multiple snapshots");
  wid.push_back(mySnapSingleCheckbox);

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
  // FIXME - let dialog determine its own size
  myBrowser = new BrowserDialog(this, font, 0, 0, 400, 320);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FileSnapDialog::~FileSnapDialog()
{
  delete myBrowser;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileSnapDialog::loadConfig()
{
  myRomPath->setEditString(instance().settings().getString("romdir"));
  myStatePath->setEditString(instance().stateDir());
  myCheatFile->setEditString(instance().cheatFile());
  myPaletteFile->setEditString(instance().paletteFile());
  myPropsFile->setEditString(instance().propertiesFile());
  mySnapPath->setEditString(instance().settings().getString("ssdir"));
  mySnapSingleCheckbox->setState(!instance().settings().getBool("sssingle"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileSnapDialog::saveConfig()
{
  instance().settings().setString("romdir", myRomPath->getEditString());
  instance().settings().setString("statedir", myStatePath->getEditString());
  instance().settings().setString("cheatfile", myCheatFile->getEditString());
  instance().settings().setString("palettefile", myPaletteFile->getEditString());
  instance().settings().setString("propsfile", myPropsFile->getEditString());
  instance().settings().setString("ssdir", mySnapPath->getEditString());
  instance().settings().setBool("sssingle", !mySnapSingleCheckbox->getState());

  // Flush changes to disk and inform the OSystem
  instance().settings().saveConfig();
  instance().setConfigPaths();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileSnapDialog::setDefaults()
{
  const string& basedir = instance().baseDir();
  const string& romdir = "roms";
  const string& statedir = basedir + BSPF_PATH_SEPARATOR + "state";
  const string& cheatfile = basedir + BSPF_PATH_SEPARATOR + "stella.cht";
  const string& palettefile = basedir + BSPF_PATH_SEPARATOR + "stella.pal";
  const string& propsfile = basedir + BSPF_PATH_SEPARATOR + "stella.pro";
  const string& ssdir = basedir + BSPF_PATH_SEPARATOR + "snapshots";

  myRomPath->setEditString(romdir);
  myStatePath->setEditString(statedir);
  myCheatFile->setEditString(cheatfile);
  myPaletteFile->setEditString(palettefile);
  myPropsFile->setEditString(propsfile);

  mySnapPath->setEditString(ssdir);
  mySnapSingleCheckbox->setState(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileSnapDialog::openBrowser(const string& title, const string& startpath,
                                 FilesystemNode::ListMode mode, int cmd)
{
cerr << " ==> add browser: " << title << " -> " << startpath << endl;
  parent().addDialog(myBrowser);

  myBrowser->setTitle(title);
  myBrowser->setEmitSignal(cmd);
  myBrowser->setStartPath(startpath, mode);
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
      openBrowser("Select ROM directory:", myRomPath->getEditString(),
                  FilesystemNode::kListDirectoriesOnly, kRomDirChosenCmd);
      break;

    case kChooseStateDirCmd:
      openBrowser("Select state directory:", myStatePath->getEditString(),
                  FilesystemNode::kListDirectoriesOnly, kStateDirChosenCmd);
      break;

    case kChooseCheatFileCmd:
      openBrowser("Select cheat file:", myCheatFile->getEditString(),
                  FilesystemNode::kListAll, kCheatFileChosenCmd);
      break;

    case kChoosePaletteFileCmd:
      openBrowser("Select palette file:", myPaletteFile->getEditString(),
                  FilesystemNode::kListAll, kPaletteFileChosenCmd);
      break;

    case kChoosePropsFileCmd:
      openBrowser("Select properties file:", myPropsFile->getEditString(),
                  FilesystemNode::kListAll, kPropsFileChosenCmd);
      break;

    case kChooseSnapDirCmd:
      openBrowser("Select snapshot directory:", mySnapPath->getEditString(),
                  FilesystemNode::kListDirectoriesOnly, kSnapDirChosenCmd);
      break;

    case kRomDirChosenCmd:
    {
      FilesystemNode dir(myBrowser->getResult());
      myRomPath->setEditString(dir.path());
      break;
    }

    case kStateDirChosenCmd:
    {
      FilesystemNode dir(myBrowser->getResult());
      myStatePath->setEditString(dir.path());
      break;
    }

    case kCheatFileChosenCmd:
    {
      FilesystemNode dir(myBrowser->getResult());
      myCheatFile->setEditString(dir.path());
      break;
    }

    case kPaletteFileChosenCmd:
    {
      FilesystemNode dir(myBrowser->getResult());
      myPaletteFile->setEditString(dir.path());
      break;
    }

    case kPropsFileChosenCmd:
    {
      FilesystemNode dir(myBrowser->getResult());
      myPropsFile->setEditString(dir.path());
      break;
    }

    case kSnapDirChosenCmd:
    {
      FilesystemNode dir(myBrowser->getResult());
      mySnapPath->setEditString(dir.path());
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
