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
// $Id: FileSnapDialog.cxx,v 1.13 2007-09-03 18:37:23 stephena Exp $
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
  const int vBorder = 8;
  int xpos, ypos, bwidth, bheight;
  WidgetArray wid;
  ButtonWidget* b;

  bwidth  = font.getStringWidth("Properties file:") + 20;
  bheight = font.getLineHeight() + 4;

  xpos = vBorder;  ypos = vBorder;

  // ROM path
  ButtonWidget* romButton = 
    new ButtonWidget(this, font, xpos, ypos, bwidth, bheight, "Rom path:",
                     kChooseRomDirCmd);
  wid.push_back(romButton);
  xpos += bwidth + 10;
  myRomPath = new EditTextWidget(this, font, xpos, ypos + 2,
                                 _w - xpos - 10, font.getLineHeight(), "");
  wid.push_back(myRomPath);

  // State directory
  xpos = vBorder;  ypos += romButton->getHeight() + 3;
  b = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight, "State path:",
                       kChooseStateDirCmd);
  wid.push_back(b);
  xpos += bwidth + 10;
  myStatePath = new EditTextWidget(this, font, xpos, ypos + 2,
                                   _w - xpos - 10, font.getLineHeight(), "");
  wid.push_back(myStatePath);

  // Cheat file
  xpos = vBorder;  ypos += b->getHeight() + 3;
  b = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight, "Cheat file:",
                       kChooseCheatFileCmd);
  wid.push_back(b);
  xpos += bwidth + 10;
  myCheatFile = new EditTextWidget(this, font, xpos, ypos + 2,
                                   _w - xpos - 10, font.getLineHeight(), "");
  wid.push_back(myCheatFile);

  // Palette file
  xpos = vBorder;  ypos += b->getHeight() + 3;
  b = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight, "Palette file:",
                       kChoosePaletteFileCmd);
  wid.push_back(b);
  xpos += bwidth + 10;
  myPaletteFile = new EditTextWidget(this, font, xpos, ypos + 2,
                                     _w - xpos - 10, font.getLineHeight(), "");
  wid.push_back(myPaletteFile);

  // Properties file
  xpos = vBorder;  ypos += b->getHeight() + 3;
  b = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight, "Properties file:",
                       kChoosePropsFileCmd);
  wid.push_back(b);
  xpos += bwidth + 10;
  myPropsFile = new EditTextWidget(this, font, xpos, ypos + 2,
                                   _w - xpos - 10, font.getLineHeight(), "");
  wid.push_back(myPropsFile);

  // Snapshot path
  xpos = vBorder;  ypos += b->getHeight() + 3;
  b = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight, "Snapshot path:",
                       kChooseSnapDirCmd);
  wid.push_back(b);
  xpos += bwidth + 10;
  mySnapPath = new EditTextWidget(this, font, xpos, ypos + 2,
                                  _w - xpos - 10, font.getLineHeight(), "");
  wid.push_back(mySnapPath);

  // Use ROM browse mode
  xpos = 30;  ypos += mySnapPath->getHeight() + 8;
  myBrowseCheckbox =
    new CheckboxWidget(this, font, xpos, ypos, "Browse folders", kBrowseDirCmd);
  wid.push_back(myBrowseCheckbox);
	 	 
  // Reload current ROM listing (in non-browse mode)
  xpos += myBrowseCheckbox->getWidth() + 20;
  myReloadRomButton =
    new ButtonWidget(this, font, xpos, ypos-2,
                     font.getStringWidth(" Reload ROM Listing "), bheight,
                     "Reload ROM Listing", kReloadRomDirCmd);
  //myReloadButton->setEditable(true);
  wid.push_back(myReloadRomButton);

  // Snapshot single or multiple saves
  xpos = 30;  ypos += myBrowseCheckbox->getHeight() + 4;
  mySnapSingleCheckbox = new CheckboxWidget(this, font, xpos, ypos,
                                            "Multiple snapshots");
  wid.push_back(mySnapSingleCheckbox);

  // Add OK & Cancel buttons
  b = addButton(font, 10, _h - 24, "Defaults", kDefaultsCmd);
  wid.push_back(b);
#ifndef MAC_OSX
  b = addButton(font, _w - 2 * (kButtonWidth + 7), _h - 24, "OK", kOKCmd);
  wid.push_back(b);
  addOKWidget(b);
  b = addButton(font, _w - (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd);
  wid.push_back(b);
  addCancelWidget(b);
#else
  b = addButton(font, _w - 2 * (kButtonWidth + 7), _h - 24, "Cancel", kCloseCmd);
  wid.push_back(b);
  addCancelWidget(b);
  b = addButton(font, _w - (kButtonWidth + 10), _h - 24, "OK", kOKCmd);
  wid.push_back(b);
  addOKWidget(b);
#endif

  addToFocusList(wid);

  // All ROM settings are disabled while in game mode
  if(!myIsGlobal)
  {
    romButton->clearFlags(WIDGET_ENABLED);
    myRomPath->setEditable(false);
    myBrowseCheckbox->clearFlags(WIDGET_ENABLED);
    myReloadRomButton->clearFlags(WIDGET_ENABLED);
  }

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
  myRomPath->setEditString(instance()->settings().getString("romdir"));
  myStatePath->setEditString(instance()->stateDir());
  myCheatFile->setEditString(instance()->cheatFile());
  myPaletteFile->setEditString(instance()->paletteFile());
  myPropsFile->setEditString(instance()->propertiesFile());
  mySnapPath->setEditString(instance()->settings().getString("ssdir"));
  bool b = instance()->settings().getBool("rombrowse");
  myBrowseCheckbox->setState(b);
  myReloadRomButton->setEnabled(myIsGlobal && !b);
  mySnapSingleCheckbox->setState(!instance()->settings().getBool("sssingle"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileSnapDialog::saveConfig()
{
  instance()->settings().setString("romdir", myRomPath->getEditString());
  instance()->settings().setString("statedir", myStatePath->getEditString());
  instance()->settings().setString("cheatfile", myCheatFile->getEditString());
  instance()->settings().setString("palettefile", myPaletteFile->getEditString());
  instance()->settings().setString("propsfile", myPropsFile->getEditString());
  instance()->settings().setString("ssdir", mySnapPath->getEditString());
  instance()->settings().setBool("rombrowse", myBrowseCheckbox->getState());
  instance()->settings().setBool("sssingle", !mySnapSingleCheckbox->getState());

  // Flush changes to disk and inform the OSystem
  instance()->settings().saveConfig();
  instance()->setConfigPaths();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileSnapDialog::setDefaults()
{
  const string& basedir = instance()->baseDir();
  const string& romdir = "roms";
  const string& statedir = basedir + BSPF_PATH_SEPARATOR + "state";
  const string& cheatfile = basedir + BSPF_PATH_SEPARATOR + "stella.cht";
  const string& palettefile = basedir + BSPF_PATH_SEPARATOR + "stella.pal";
  const string& propsfile = basedir + BSPF_PATH_SEPARATOR + "stella.pro";

  myRomPath->setEditString(romdir);
  myStatePath->setEditString(statedir);
  myCheatFile->setEditString(cheatfile);
  myPaletteFile->setEditString(palettefile);
  myPropsFile->setEditString(propsfile);

  mySnapPath->setEditString(string(".") + BSPF_PATH_SEPARATOR);
  mySnapSingleCheckbox->setState(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileSnapDialog::openBrowser(const string& title, const string& startpath,
                                 FilesystemNode::ListMode mode, int cmd)
{
  parent()->addDialog(myBrowser);

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
      {
        sendCommand(kBrowseChangedCmd, 0, 0); // Call this before refreshing ROMs
        sendCommand(kRomDirChosenCmd, 0, 0);  // Let the boss know romdir has changed
      }
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
