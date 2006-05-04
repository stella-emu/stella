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
// $Id: GameInfoDialog.cxx,v 1.25 2006-05-04 17:45:25 stephena Exp $
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
      OSystem* osystem, DialogContainer* parent, const GUI::Font& font,
      GuiObject* boss, int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h),
    CommandSender(boss)
{
  const int fontHeight = font.getFontHeight(),
            lineHeight = font.getLineHeight();

  const int vBorder = 4;
  int xpos, ypos, lwidth, fwidth, pwidth, tabID;
  unsigned int i;
  WidgetArray wid;

  // The tab widget
  xpos = 2; ypos = vBorder;
  myTab = new TabWidget(this, font, xpos, ypos, _w - 2*xpos, _h - 24 - 2*ypos);
  addTabWidget(myTab);

  // 1) Cartridge properties
  wid.clear();
  tabID = myTab->addTab("Cartridge");

  xpos = 10;
  lwidth = font.getStringWidth("Manufacturer: ");
  fwidth = _w - xpos - lwidth - 10;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Name:", kTextAlignLeft);
  myName = new EditTextWidget(myTab, font, xpos+lwidth, ypos,
                              fwidth, fontHeight, "");
  wid.push_back(myName);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "MD5:", kTextAlignLeft);
  myMD5 = new StaticTextWidget(myTab, font, xpos+lwidth, ypos,
                               fwidth, fontHeight,
                               "", kTextAlignLeft);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Manufacturer:", kTextAlignLeft);
  myManufacturer = new EditTextWidget(myTab, font, xpos+lwidth, ypos,
                                      fwidth, fontHeight, "");
  wid.push_back(myManufacturer);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Model:", kTextAlignLeft);
  myModelNo = new EditTextWidget(myTab, font, xpos+lwidth, ypos,
                                 fwidth, fontHeight, "");
  wid.push_back(myModelNo);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Rarity:", kTextAlignLeft);
  myRarity = new EditTextWidget(myTab, font, xpos+lwidth, ypos,
                                fwidth, fontHeight, "");
  wid.push_back(myRarity);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Note:", kTextAlignLeft);
  myNote = new EditTextWidget(myTab, font, xpos+lwidth, ypos,
                              fwidth, fontHeight, "");
  wid.push_back(myNote);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Sound:", kTextAlignLeft);
  pwidth = font.getStringWidth("Stereo");
  mySound = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                            pwidth, lineHeight, "", 0, 0);
  mySound->appendEntry("Mono", 1);
  mySound->appendEntry("Stereo", 2);
  wid.push_back(mySound);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Type:", kTextAlignLeft);
  pwidth = font.getStringWidth("CV (Commavid extra ram)");
  myType = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                           pwidth, lineHeight, "", 0, 0);
  for(i = 0; i < 21; ++i)
    myType->appendEntry(ourCartridgeList[i][0], i+1);
  wid.push_back(myType);

  // Add items for tab 0
  addToFocusList(wid, tabID);


  // 2) Console properties
  wid.clear();
  tabID = myTab->addTab("Console");

  xpos = 10; ypos = vBorder;
  lwidth = font.getStringWidth("Right Difficulty: ");
  pwidth = font.getStringWidth("B & W");
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Left Difficulty:", kTextAlignLeft);
  myLeftDiff = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                               pwidth, lineHeight, "", 0, 0);
  myLeftDiff->appendEntry("B", 1);
  myLeftDiff->appendEntry("A", 2);
  wid.push_back(myLeftDiff);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Right Difficulty:", kTextAlignLeft);
  myRightDiff = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                                pwidth, lineHeight, "", 0, 0);
  myRightDiff->appendEntry("B", 1);
  myRightDiff->appendEntry("A", 2);
  wid.push_back(myRightDiff);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "TV Type:", kTextAlignLeft);
  myTVType = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                             pwidth, lineHeight, "", 0, 0);
  myTVType->appendEntry("Color", 1);
  myTVType->appendEntry("B & W", 2);
  wid.push_back(myTVType);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Swap ports:", kTextAlignLeft);
  mySwapPorts = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                                pwidth, lineHeight, "", 0, 0);
  mySwapPorts->appendEntry("Yes", 1);
  mySwapPorts->appendEntry("No", 2);
  wid.push_back(mySwapPorts);

  // Add items for tab 1
  addToFocusList(wid, tabID);


  // 3) Controller properties
  wid.clear();
  tabID = myTab->addTab("Controller");

  xpos = 10; ypos = vBorder;
  lwidth = font.getStringWidth("Right Controller: ");
  pwidth = font.getStringWidth("Booster-Grip");
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Left Controller:", kTextAlignLeft);
  myLeftController = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                                     pwidth, lineHeight, "", 0, 0);
  for(i = 0; i < 5; ++i)
    myLeftController->appendEntry(ourControllerList[i][0], i+1);
  wid.push_back(myLeftController);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Right Controller:", kTextAlignLeft);
  myRightController = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                                      pwidth, lineHeight, "", 0, 0);
  for(i = 0; i < 5; ++i)
    myRightController->appendEntry(ourControllerList[i][0], i+1);
  wid.push_back(myRightController);

  // Add items for tab 2
  addToFocusList(wid, tabID);


  // 4) Display properties
  wid.clear();
  tabID = myTab->addTab("Display");

  xpos = 10; ypos = vBorder;
  lwidth = font.getStringWidth("Use Phosphor: ");
  pwidth = font.getStringWidth("NTSC");
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Format:", kTextAlignLeft);
  myFormat = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                             pwidth, lineHeight, "", 0, 0);
  myFormat->appendEntry("NTSC", 1);
  myFormat->appendEntry("PAL", 2);
  wid.push_back(myFormat);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "XStart:", kTextAlignLeft);
  myXStart = new EditTextWidget(myTab, font, xpos+lwidth, ypos,
                                25, fontHeight, "");
  wid.push_back(myXStart);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Width:", kTextAlignLeft);
  myWidth = new EditTextWidget(myTab, font, xpos+lwidth, ypos,
                               25, fontHeight, "");
  wid.push_back(myWidth);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "YStart:", kTextAlignLeft);
  myYStart = new EditTextWidget(myTab, font, xpos+lwidth, ypos,
                                25, fontHeight, "");
  wid.push_back(myYStart);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Height:", kTextAlignLeft);
  myHeight = new EditTextWidget(myTab, font, xpos+lwidth, ypos,
                                25, fontHeight, "");
  wid.push_back(myHeight);

  ypos += lineHeight + 3;
  pwidth = font.getStringWidth("Yes");
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Use Phosphor:", kTextAlignLeft);
  myPhosphor = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                               pwidth, lineHeight, "", 0, 0);
  myPhosphor->appendEntry("Yes", 1);
  myPhosphor->appendEntry("No", 2);
  wid.push_back(myPhosphor);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Use HMBlanks:", kTextAlignLeft);
  myHmoveBlanks = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                                  pwidth, lineHeight, "", 0, 0);
  myHmoveBlanks->appendEntry("Yes", 1);
  myHmoveBlanks->appendEntry("No", 2);
  wid.push_back(myHmoveBlanks);

  // Add items for tab 3
  addToFocusList(wid, tabID);


  // Activate the first tab
  myTab->setActiveTab(0);

  // Add message concerning usage
  new StaticTextWidget(this, font, 10, _h - 20, 120, fontHeight,
                       "(*) Requires a ROM reload", kTextAlignLeft);

  // Add Defaults, OK and Cancel buttons
  ButtonWidget* b;
  wid.clear();
#ifndef MAC_OSX
  b = addButton(font, _w - 2 * (kButtonWidth + 7), _h - 24, "OK", kOKCmd);
  wid.push_back(b);
  b = addButton(font, _w - (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd);
  wid.push_back(b);
#else
  b = addButton(font, _w - 2 * (kButtonWidth + 7), _h - 24, "Cancel", kCloseCmd);
  wid.push_back(b);
  b = addButton(font, _w - (kButtonWidth + 10), _h - 24, "OK", kOKCmd);
  wid.push_back(b);
#endif
  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GameInfoDialog::~GameInfoDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadConfig()
{
  string s;
  int i;

  // Cartridge properties
  s = myGameProperties->get(Cartridge_Name);
  myName->setEditString(s);

  s = myGameProperties->get(Cartridge_MD5);
  myMD5->setLabel(s);

  s = myGameProperties->get(Cartridge_Manufacturer);
  myManufacturer->setEditString(s);

  s = myGameProperties->get(Cartridge_ModelNo);
  myModelNo->setEditString(s);

  s = myGameProperties->get(Cartridge_Rarity);
  myRarity->setEditString(s);

  s = myGameProperties->get(Cartridge_Note);
  myNote->setEditString(s);

  s = myGameProperties->get(Cartridge_Sound);
  if(s == "MONO")
    mySound->setSelectedTag(1);
  else if(s == "STEREO")
    mySound->setSelectedTag(2);
  else
    mySound->setSelectedTag(0);

  s = myGameProperties->get(Cartridge_Type);
  for(i = 0; i < 21; ++i)
  {
    if(s == ourCartridgeList[i][1])
      break;
  }
  i = (i == 21) ? 0: i + 1;
  myType->setSelectedTag(i);

  // Console properties
  s = myGameProperties->get(Console_LeftDifficulty);
  if(s == "B")
    myLeftDiff->setSelectedTag(1);
  else if(s == "A")
    myLeftDiff->setSelectedTag(2);
  else
    myLeftDiff->setSelectedTag(0);

  s = myGameProperties->get(Console_RightDifficulty);
  if(s == "B")
    myRightDiff->setSelectedTag(1);
  else if(s == "A")
    myRightDiff->setSelectedTag(2);
  else
    myRightDiff->setSelectedTag(0);

  s = myGameProperties->get(Console_TelevisionType);
  if(s == "COLOR")
    myTVType->setSelectedTag(1);
  else if(s == "BLACKANDWHITE")
    myTVType->setSelectedTag(2);
  else
    myTVType->setSelectedTag(0);

  s = myGameProperties->get(Console_SwapPorts);
  if(s == "YES")
    mySwapPorts->setSelectedTag(1);
  else if(s == "NO")
    mySwapPorts->setSelectedTag(2);
  else
    mySwapPorts->setSelectedTag(0);

  // Controller properties
  s = myGameProperties->get(Controller_Left);
  for(i = 0; i < 5; ++i)
  {
    if(s == ourControllerList[i][1])
      break;
  }
  i = (i == 5) ? 0: i + 1;
  myLeftController->setSelectedTag(i);

  s = myGameProperties->get(Controller_Right);
  for(i = 0; i < 5; ++i)
  {
    if(s == ourControllerList[i][1])
      break;
  }
  i = (i == 5) ? 0: i + 1;
  myRightController->setSelectedTag(i);

  // Display properties
  s = myGameProperties->get(Display_Format);
  if(s == "NTSC")
    myFormat->setSelectedTag(1);
  else if(s == "PAL")
    myFormat->setSelectedTag(2);
  else
    myFormat->setSelectedTag(0);

  s = myGameProperties->get(Display_XStart);
  myXStart->setEditString(s);

  s = myGameProperties->get(Display_Width);
  myWidth->setEditString(s);

  s = myGameProperties->get(Display_YStart);
  myYStart->setEditString(s);

  s = myGameProperties->get(Display_Height);
  myHeight->setEditString(s);

  s = myGameProperties->get(Display_Phosphor);
  if(s == "YES")
    myPhosphor->setSelectedTag(1);
  else if(s == "NO")
    myPhosphor->setSelectedTag(2);
  else
    myPhosphor->setSelectedTag(0);

  s = myGameProperties->get(Emulation_HmoveBlanks);
  if(s == "YES")
    myHmoveBlanks->setSelectedTag(1);
  else if(s == "NO")
    myHmoveBlanks->setSelectedTag(2);
  else
    myHmoveBlanks->setSelectedTag(0);

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::saveConfig()
{
  string s;
  int i, tag;

  // Cartridge properties
  s = myName->getEditString();
  myGameProperties->set(Cartridge_Name, s);

  s = myManufacturer->getEditString();
  myGameProperties->set(Cartridge_Manufacturer, s);

  s = myModelNo->getEditString();
  myGameProperties->set(Cartridge_ModelNo, s);

  s = myRarity->getEditString();
  myGameProperties->set(Cartridge_Rarity, s);

  s = myNote->getEditString();
  myGameProperties->set(Cartridge_Note, s);

  tag = mySound->getSelectedTag();
  s = (tag == 1) ? "Mono" : "Stereo";
  myGameProperties->set(Cartridge_Sound, s);

  tag = myType->getSelectedTag();
  for(i = 0; i < 21; ++i)
  {
    if(i == tag-1)
    {
      myGameProperties->set(Cartridge_Type, ourCartridgeList[i][1]);
      break;
    }
  }

  // Console properties
  tag = myLeftDiff->getSelectedTag();
  s = (tag == 1) ? "B" : "A";
  myGameProperties->set(Console_LeftDifficulty, s);

  tag = myRightDiff->getSelectedTag();
  s = (tag == 1) ? "B" : "A";
  myGameProperties->set(Console_RightDifficulty, s);

  tag = myTVType->getSelectedTag();
  s = (tag == 1) ? "Color" : "BlackAndWhite";
  myGameProperties->set(Console_TelevisionType, s);

  tag = mySwapPorts->getSelectedTag();
  s = (tag == 1) ? "Yes" : "No";
  myGameProperties->set(Console_SwapPorts, s);

  // Controller properties
  tag = myLeftController->getSelectedTag();
  for(i = 0; i < 5; ++i)
  {
    if(i == tag-1)
    {
      myGameProperties->set(Controller_Left, ourControllerList[i][0]);
      break;
    }
  }

  tag = myRightController->getSelectedTag();
  for(i = 0; i < 5; ++i)
  {
    if(i == tag-1)
    {
      myGameProperties->set(Controller_Right, ourControllerList[i][0]);
      break;
    }
  }

  // Display properties
  tag = myFormat->getSelectedTag();
  s = (tag == 1) ? "NTSC" : "PAL";
  myGameProperties->set(Display_Format, s);

  s = myXStart->getEditString();
  myGameProperties->set(Display_XStart, s);

  s = myWidth->getEditString();
  myGameProperties->set(Display_Width, s);

  s = myYStart->getEditString();
  myGameProperties->set(Display_YStart, s);

  s = myHeight->getEditString();
  myGameProperties->set(Display_Height, s);

  tag = myPhosphor->getSelectedTag();
  s = (tag == 1) ? "Yes" : "No";
  myGameProperties->set(Display_Phosphor, s);

  tag = myHmoveBlanks->getSelectedTag();
  s = (tag == 1) ? "Yes" : "No";
  myGameProperties->set(Emulation_HmoveBlanks, s);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::handleCommand(CommandSender* sender, int cmd,
                                          int data, int id)
{
  switch (cmd)
  {
    case kOKCmd:
      saveConfig();
      instance()->eventHandler().saveProperties();
      close();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* GameInfoDialog::ourControllerList[5][2] = {
  { "Booster-Grip", "BOOSTER-GRIP" },
  { "Driving",      "DRIVING"      },
  { "Keyboard",     "KEYBOARD"     },
  { "Paddles",      "PADDLES"      },
  { "Joystick",     "JOYSTICK"     }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* GameInfoDialog::ourCartridgeList[21][2] = {
  { "Auto-detect",     "AUTO-DETECT"   },
  { "2K (2K Atari)",            "2K"   },
  { "3E (32K Tigervision)",     "3E"   },
  { "3F (512K Tigervision)",    "3F"   },
  { "4K (4K Atari)",            "4K"   },
  { "AR (Supercharger)",        "AR"   },
  { "CV (Commavid extra ram)",  "CV"   },
  { "DPC (Pitfall II)",         "DPC"  },
  { "E0 (8K Parker Bros)",      "E0"   },
  { "E7 (16K M-network)",       "E7"   },
  { "F4 (32K Atari)",           "F4"   },
  { "F4SC (32K Atari + ram)",   "F4SC" },
  { "F6 (16K Atari)",           "F6"   },
  { "F6SC (16K Atari + ram)",   "F6SC" },
  { "F8 (8K Atari)",            "F8"   },
  { "F8SC (8K Atari + ram)",    "F8SC" },
  { "FASC (CBS RAM Plus)",      "FASC" },
  { "FE (8K Decathlon)",        "FE"   },
  { "MB (Dynacom Megaboy)",     "MB"   },
  { "MC (C. Wilkson Megacart)", "MC"   },
  { "UA (8K UA Ltd.)",          "UA"   }
};
