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
// $Id: GameInfoDialog.cxx,v 1.13 2005-09-29 18:50:51 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "GuiUtils.hxx"
#include "OSystem.hxx"
#include "Props.hxx"
#include "Widget.hxx"
#include "Dialog.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "TabWidget.hxx"
#include "GameInfoDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GameInfoDialog::GameInfoDialog(
      OSystem* osystem, DialogContainer* parent, GuiObject* boss,
      int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h),
    CommandSender(boss)
{
  const GUI::Font& font = instance()->font();
  const int fontHeight = font.getFontHeight(),
            lineHeight = font.getLineHeight();

  const int vBorder = 4;
  int xpos, ypos, lwidth, fwidth, tabID;
  WidgetArray wid;

  // The tab widget
  xpos = 2; ypos = vBorder;
  myTab = new TabWidget(this, xpos, ypos, _w - 2*xpos, _h - 24 - 2*ypos);

  // 1) Cartridge properties
  wid.clear();
  tabID = myTab->addTab("Cartridge");

  xpos = 10;
  lwidth = font.getStringWidth("Manufacturer: ");
  fwidth = _w - xpos - lwidth - 10;
  new StaticTextWidget(myTab, xpos, ypos, lwidth, fontHeight,
                       "Name:", kTextAlignLeft);
  myName = new EditTextWidget(myTab, xpos+lwidth, ypos, fwidth, fontHeight, "");
  wid.push_back(myName);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, xpos, ypos, lwidth, fontHeight,
                       "MD5:", kTextAlignLeft);
  myMD5 = new StaticTextWidget(myTab, xpos+lwidth, ypos,
                               fwidth, fontHeight,
                               "", kTextAlignLeft);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, xpos, ypos, lwidth, fontHeight,
                       "Manufacturer:", kTextAlignLeft);
  myManufacturer = new EditTextWidget(myTab, xpos+lwidth, ypos,
                                      100, fontHeight, "");
  wid.push_back(myManufacturer);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, xpos, ypos, lwidth, fontHeight,
                       "Model:", kTextAlignLeft);
  myModelNo = new EditTextWidget(myTab, xpos+lwidth, ypos,
                                 100, fontHeight, "");
  wid.push_back(myModelNo);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, xpos, ypos, lwidth, fontHeight,
                       "Rarity:", kTextAlignLeft);
  myRarity = new EditTextWidget(myTab, xpos+lwidth, ypos,
                                100, fontHeight, "");
  wid.push_back(myRarity);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, xpos, ypos, lwidth, fontHeight,
                       "Note:", kTextAlignLeft);
  myNote = new EditTextWidget(myTab, xpos+lwidth, ypos,
                              fwidth, fontHeight, "");
  wid.push_back(myNote);

/*
  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, xpos, ypos, lwidth, fontHeight,
                       "Sound:", kTextAlignLeft);
  mySound = new EditTextWidget(myTab, xpos+lwidth, ypos,
                                 100, fontHeight, "");
  wid.push_back(mySound);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, xpos, ypos, lwidth, fontHeight,
                       "Type:", kTextAlignLeft);
  myType = new EditTextWidget(myTab, xpos+lwidth, ypos,
                                 100, fontHeight, "");
  wid.push_back(myType);
*/

  // Add items for tab 0
  addToFocusList(wid, tabID);

  // 2) Console/Controller properties
  wid.clear();
  tabID = myTab->addTab("Console");

  addToFocusList(wid, tabID);


  // 3) Controller properties
  wid.clear();
  tabID = myTab->addTab("Controller");

  addToFocusList(wid, tabID);


  // 4) Display properties
  wid.clear();
  tabID = myTab->addTab("Display");

  addToFocusList(wid, tabID);

/*
  // Snapshot path
  xpos = 15;
  new ButtonWidget(myTab, xpos, ypos, kButtonWidth + 14, 16, "Path",
                   kChooseSnapDirCmd, 0);
  xpos += kButtonWidth + 30;
  mySnapPath = new StaticTextWidget(myTab, xpos, ypos + 3,
                                   _w - xpos - 10, kLineHeight,
                                   "", kTextAlignLeft);

  // Snapshot save name
  xpos = 10; ypos += 22;
  mySnapTypePopup = new PopUpWidget(myTab, xpos, ypos, 140, kLineHeight,
                                    "Save snapshot as: ", 87, 0);
  mySnapTypePopup->appendEntry("romname", 1);
  mySnapTypePopup->appendEntry("md5sum", 2);

  // Snapshot single or multiple saves
  xpos = 30;  ypos += 18;
  mySnapSingleCheckbox = new CheckboxWidget(myTab, font, xpos, ypos,
                                            "Multiple snapshots");
*/

  // Activate the first tab
  myTab->setActiveTab(0);

  // Add message concerning usage
  new StaticTextWidget(this, 10, _h - 20, 120, fontHeight,
                       "(*) Requires a ROM reload", kTextAlignLeft);

  // Add Defaults, OK and Cancel buttons
#ifndef MAC_OSX
  addButton(_w - 2 * (kButtonWidth + 7), _h - 24, "OK", kOKCmd, 0);
  addButton(_w - (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd, 0);
#else
  addButton(_w - 2 * (kButtonWidth + 7), _h - 24, "Cancel", kCloseCmd, 0);
  addButton(_w - (kButtonWidth + 10), _h - 24, "OK", kOKCmd, 0);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GameInfoDialog::~GameInfoDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadConfig()
{
  string s;

  s = myGameProperties->get("Cartridge.Name");
  myName->setEditString(s);

  s = myGameProperties->get("Cartridge.MD5");
  myMD5->setLabel(s);

  s = myGameProperties->get("Cartridge.Manufacturer");
  myManufacturer->setEditString(s);

  s = myGameProperties->get("Cartridge.ModelNo");
  myModelNo->setEditString(s);

  s = myGameProperties->get("Cartridge.Rarity");
  myRarity->setEditString(s);

  s = myGameProperties->get("Cartridge.Note");
  myNote->setEditString(s);

/*
  s = instance()->settings().getString("ssname");
  if(s == "romname")
    mySnapTypePopup->setSelectedTag(1);
  else if(s == "md5sum")
    mySnapTypePopup->setSelectedTag(2);
  else
    mySnapTypePopup->setSelectedTag(0);
*/

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::saveConfig()
{
cerr << "saveConfig()\n";
/*
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
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::handleCommand(CommandSender* sender, int cmd,
                                          int data, int id)
{
  switch (cmd)
  {
    case kOKCmd:
      saveConfig();
      close();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
