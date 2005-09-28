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
// $Id: GameInfoDialog.cxx,v 1.12 2005-09-28 22:49:06 stephena Exp $
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
  int xpos, ypos, lwidth, tabID;
  WidgetArray wid;

  // The tab widget
  xpos = 2; ypos = vBorder;
  myTab = new TabWidget(this, xpos, ypos, _w - 2*xpos, _h - 24 - 2*ypos);

  // 1) Cartridge properties
  wid.clear();
  tabID = myTab->addTab("Cartridge");
//  myPrompt = new PromptWidget(myTab, 2, 2, widWidth, widHeight);
//  myTab->setParentWidget(tabID, myPrompt);
//  addToFocusList(myPrompt->getFocusList(), tabID);

  xpos = 10;
  lwidth = font.getStringWidth("Manufacturer: ");
  new StaticTextWidget(myTab, xpos, ypos, lwidth, fontHeight,
                       "Name:", kTextAlignLeft);
  myName = new EditTextWidget(myTab, xpos+lwidth, ypos, 100, fontHeight, "");
  wid.push_back(myName);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, xpos, ypos, lwidth, fontHeight,
                       "MD5:", kTextAlignLeft);
/*
  myMD5 = new StaticTextWidget(myTab, xpos, ypos,
                               xpos+lwidth, fontHeight,
                               "HAHA!", kTextAlignLeft);
  myMD5->setLabel("GAGA!");
*/
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
  myRarity = new PopUpWidget(myTab, xpos+lwidth, ypos, 100, lineHeight,
                             "", 0, 0);
  myRarity->appendEntry("(1) Common", 1);
  myRarity->appendEntry("(2) Common+", 2);
  myRarity->appendEntry("(3) Scarce", 3);
  myRarity->appendEntry("(4) Scarce+", 4);
  myRarity->appendEntry("(5) Rare", 5);
/*
  myRarity->appendEntry("(6) Rare+", 6);
  myRarity->appendEntry("(7) Very Rare", 7);
  myRarity->appendEntry("(8) Very Rare+", 8);
  myRarity->appendEntry("(9) Extremely Rare", 9);
  myRarity->appendEntry("(10) Unbelievably Rare", 10);
  myRarity->appendEntry("(H) Homebrew", 11);
  myRarity->appendEntry("(R) Reproduction", 12);
  myRarity->appendEntry("(P) Prototype", 13);
*/
  wid.push_back(myRarity);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, xpos, ypos, lwidth, fontHeight,
                       "Note:", kTextAlignLeft);
  myNote = new EditTextWidget(myTab, xpos+lwidth, ypos,
                              100, fontHeight, "");
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
//  myTab->addTab("Console");


  // 3) Controller properties
//  myTab->addTab("Controller");


  // 4) Display properties
//  myTab->addTab("Display");

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

  // Add OK & Cancel buttons
#ifndef MAC_OSX
  addButton(_w - 2 *(kButtonWidth + 10), _h - 24, "OK", kOKCmd, 0);
  addButton(_w - (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd, 0);
#else
  addButton(_w - 2 *(kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd, 0);
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
cerr << "loadConfig()\n";
/*
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
