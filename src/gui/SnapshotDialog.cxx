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
//============================================================================

#include "bspf.hxx"

#include "BrowserDialog.hxx"
#include "EditTextWidget.hxx"
#include "FSNode.hxx"
#include "LauncherDialog.hxx"
#include "PopUpWidget.hxx"
#include "Settings.hxx"

#include "SnapshotDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SnapshotDialog::SnapshotDialog(
      OSystem* osystem, DialogContainer* parent,
      const GUI::Font& font, GuiObject* boss,
      int max_w, int max_h)
  : Dialog(osystem, parent, 0, 0, 0, 0),
    myBrowser(NULL)
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

  // Snapshot path (save files)
  b = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                       "Snapshot save path:", kChooseSnapSaveDirCmd);
  wid.push_back(b);
  xpos += buttonWidth + 10;
  mySnapSavePath = new EditTextWidget(this, font, xpos, ypos + 2,
                                  _w - xpos - 10, lineHeight, "");
  wid.push_back(mySnapSavePath);

  // Snapshot path (load files)
  xpos = vBorder;  ypos += buttonHeight + 3;
  b = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                       "Snapshot load path:", kChooseSnapLoadDirCmd);
  wid.push_back(b);
  xpos += buttonWidth + 10;
  mySnapLoadPath = new EditTextWidget(this, font, xpos, ypos + 2,
                                  _w - xpos - 10, lineHeight, "");
  wid.push_back(mySnapLoadPath);

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

  // Create file browser dialog
  myBrowser = new BrowserDialog(this, font, max_w, max_h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SnapshotDialog::~SnapshotDialog()
{
  delete myBrowser;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SnapshotDialog::loadConfig()
{
  const Settings& settings = instance().settings();
  mySnapSavePath->setText(settings.getString("snapsavedir"));
  mySnapLoadPath->setText(settings.getString("snaploaddir"));
  mySnapSingle->setState(settings.getBool("sssingle"));
  mySnap1x->setState(settings.getBool("ss1x"));
  mySnapInterval->setSelected(instance().settings().getString("ssinterval"), "2");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SnapshotDialog::saveConfig()
{
  instance().settings().setValue("snapsavedir", mySnapSavePath->getText());
  instance().settings().setValue("snaploaddir", mySnapLoadPath->getText());
  instance().settings().setValue("sssingle", mySnapSingle->getState());
  instance().settings().setValue("ss1x", mySnap1x->getState());
  instance().settings().setValue("ssinterval",
    mySnapInterval->getSelectedTag().toString());

  // Flush changes to disk and inform the OSystem
  instance().saveConfig();
  instance().setConfigPaths();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SnapshotDialog::setDefaults()
{
  mySnapSavePath->setText(instance().defaultSnapSaveDir());
  mySnapLoadPath->setText(instance().defaultSnapLoadDir());
 
  mySnapSingle->setState(false);
  mySnap1x->setState(false);
  mySnapInterval->setSelected("2", "2");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SnapshotDialog::handleCommand(CommandSender* sender, int cmd,
                                   int data, int id)
{
  switch (cmd)
  {
    case kOKCmd:
      saveConfig();
      close();
      break;

    case kDefaultsCmd:
      setDefaults();
      break;

    case kChooseSnapSaveDirCmd:
      myBrowser->show("Select snapshot save directory:", mySnapSavePath->getText(),
                      BrowserDialog::Directories, kSnapSaveDirChosenCmd);
      break;

    case kChooseSnapLoadDirCmd:
      myBrowser->show("Select snapshot load directory:", mySnapLoadPath->getText(),
                      BrowserDialog::Directories, kSnapLoadDirChosenCmd);
      break;

    case kSnapSaveDirChosenCmd:
      mySnapSavePath->setText(myBrowser->getResult().getShortPath());
      break;

    case kSnapLoadDirChosenCmd:
      mySnapLoadPath->setText(myBrowser->getResult().getShortPath());
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
