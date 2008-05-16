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
// $Id: GameInfoDialog.cxx,v 1.56 2008-05-16 12:17:23 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "Console.hxx"
#include "Dialog.hxx"
#include "EditTextWidget.hxx"
#include "Launcher.hxx"
#include "OSystem.hxx"
#include "PopUpWidget.hxx"
#include "Props.hxx"
#include "PropsSet.hxx"
#include "TabWidget.hxx"
#include "Widget.hxx"

#include "GameInfoDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GameInfoDialog::GameInfoDialog(
      OSystem* osystem, DialogContainer* parent, const GUI::Font& font,
      GuiObject* boss, int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h),
    CommandSender(boss),
    myPropertiesLoaded(false),
    myDefaultsSelected(false)
{
  const int lineHeight   = font.getLineHeight(),
            fontHeight   = font.getFontHeight(),
            buttonWidth  = font.getStringWidth("Defaults") + 20,
            buttonHeight = font.getLineHeight() + 4;
  const int vBorder = 4;
  int xpos, ypos, lwidth, fwidth, pwidth, tabID;
  unsigned int i;
  WidgetArray wid;

  // The tab widget
  xpos = 2; ypos = vBorder;
  myTab = new TabWidget(this, font, xpos, ypos, _w - 2*xpos, _h - 24 - 2*ypos - 15);
  addTabWidget(myTab);
  addFocusWidget(myTab);

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
  pwidth = font.getStringWidth("SB (128-256k SUPERbanking)");
  myType = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                           pwidth, lineHeight, "", 0, 0);
  for(i = 0; i < kNumCartTypes; ++i)
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

  ypos += lineHeight + 5;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Right Difficulty:", kTextAlignLeft);
  myRightDiff = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                                pwidth, lineHeight, "", 0, 0);
  myRightDiff->appendEntry("B", 1);
  myRightDiff->appendEntry("A", 2);
  wid.push_back(myRightDiff);

  ypos += lineHeight + 5;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "TV Type:", kTextAlignLeft);
  myTVType = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                             pwidth, lineHeight, "", 0, 0);
  myTVType->appendEntry("Color", 1);
  myTVType->appendEntry("B & W", 2);
  wid.push_back(myTVType);

  // Add items for tab 1
  addToFocusList(wid, tabID);


  // 3) Controller properties
  wid.clear();
  tabID = myTab->addTab("Controller");

  xpos = 10; ypos = vBorder;
  lwidth = font.getStringWidth("P0 Controller: ");
  pwidth = font.getStringWidth("CX-22 Trakball");
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "P0 Controller:", kTextAlignLeft);
  myP0Controller = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                                   pwidth, lineHeight, "", 0, 0);
  for(i = 0; i < kNumControllerTypes; ++i)
    myP0Controller->appendEntry(ourControllerList[i][0], i+1);
  wid.push_back(myP0Controller);

  xpos += lwidth+myP0Controller->getWidth() + 4;
  new StaticTextWidget(myTab, font, xpos, ypos+1, font.getStringWidth("in "),
                       fontHeight, "in ", kTextAlignLeft);
  xpos += font.getStringWidth("in ");
  myLeftPort = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                               "", 0, kLeftCChanged);
  myLeftPort->appendEntry("left port", 1);
  myLeftPort->appendEntry("right port", 2);
  wid.push_back(myLeftPort);

  xpos = 10;  ypos += lineHeight + 5;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "P1 Controller:", kTextAlignLeft);
  myP1Controller = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                                   pwidth, lineHeight, "", 0, 0);
  for(i = 0; i < kNumControllerTypes; ++i)
    myP1Controller->appendEntry(ourControllerList[i][0], i+1);
  wid.push_back(myP1Controller);

  xpos += lwidth+myP1Controller->getWidth() + 4;
  new StaticTextWidget(myTab, font, xpos, ypos+1, font.getStringWidth("in "),
                       fontHeight, "in ", kTextAlignLeft);
  xpos += font.getStringWidth("in ");
  myRightPort = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                "", 0, kRightCChanged);
  myRightPort->appendEntry("left port", 1);
  myRightPort->appendEntry("right port", 2);
  wid.push_back(myRightPort);

  xpos = 10;  ypos += lineHeight + 5;
  pwidth = font.getStringWidth("Yes");
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Swap Paddles:", kTextAlignLeft);
  mySwapPaddles = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                                  pwidth, lineHeight, "", 0, 0);
  mySwapPaddles->appendEntry("Yes", 1);
  mySwapPaddles->appendEntry("No", 2);
  wid.push_back(mySwapPaddles);


  // Add items for tab 2
  addToFocusList(wid, tabID);


  // 4) Display properties
  wid.clear();
  tabID = myTab->addTab("Display");

  xpos = 10; ypos = vBorder;
  lwidth = font.getStringWidth("Use Phosphor: ");
  pwidth = font.getStringWidth("Auto-detect");
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Format:", kTextAlignLeft);
  myFormat = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                             pwidth, lineHeight, "", 0, 0);
  myFormat->appendEntry("Auto-detect", 1);
  myFormat->appendEntry("NTSC", 2);
  myFormat->appendEntry("PAL", 3);
  myFormat->appendEntry("SECAM", 4);
  myFormat->appendEntry("NTSC50", 5);
  myFormat->appendEntry("PAL60", 6);
  myFormat->appendEntry("SECAM60", 7);

  wid.push_back(myFormat);

  ypos += lineHeight + 5;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "YStart:", kTextAlignLeft);
  myYStart = new EditTextWidget(myTab, font, xpos+lwidth, ypos,
                                25, fontHeight, "");
  wid.push_back(myYStart);

  ypos += lineHeight + 5;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Height:", kTextAlignLeft);
  myHeight = new EditTextWidget(myTab, font, xpos+lwidth, ypos,
                                25, fontHeight, "");
  wid.push_back(myHeight);

  ypos += lineHeight + 5;
  pwidth = font.getStringWidth("Yes");
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Use Phosphor:", kTextAlignLeft);
  myPhosphor = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                               pwidth, lineHeight, "", 0, kPhosphorChanged);
  myPhosphor->appendEntry("Yes", 1);
  myPhosphor->appendEntry("No", 2);
  wid.push_back(myPhosphor);

  myPPBlend = new SliderWidget(myTab, font, xpos + lwidth + myPhosphor->getWidth() + 10,
                               ypos, 30, lineHeight, "Blend: ",
                               font.getStringWidth("Blend: "),
                               kPPBlendChanged);
  myPPBlend->setMinValue(1); myPPBlend->setMaxValue(100);
  wid.push_back(myPPBlend);

  myPPBlendLabel = new StaticTextWidget(myTab, font,
                                        xpos + lwidth + myPhosphor->getWidth() + 10 + \
                                        myPPBlend->getWidth() + 4, ypos+1,
                                        15, fontHeight, "", kTextAlignLeft);
  myPPBlendLabel->setFlags(WIDGET_CLEARBG);

  ypos += lineHeight + 5;
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
  lwidth = font.getStringWidth("(*) Changes to properties require a ROM reload");
  new StaticTextWidget(this, font, 10, _h - 38, lwidth, fontHeight,
                       "(*) Changes to properties require a ROM reload",
                       kTextAlignLeft);

  // Add Defaults, OK and Cancel buttons
  wid.clear();
  ButtonWidget* b;
  b = new ButtonWidget(this, font, 10, _h - buttonHeight - 10,
                       buttonWidth, buttonHeight, "Defaults", kDefaultsCmd);
  wid.push_back(b);
  addOKCancelBGroup(wid, font);
  addBGroupToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GameInfoDialog::~GameInfoDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadConfig()
{
  myPropertiesLoaded = false;
  myDefaultsSelected = false;

  if(&myOSystem->console())
  {
    myGameProperties = myOSystem->console().properties();
    myPropertiesLoaded = true;
    loadView();
  }
  else if(&myOSystem->launcher())
  {
    const string& md5 = myOSystem->launcher().selectedRomMD5();
    if(md5 != "")
    {
      instance()->propSet().getMD5(md5, myGameProperties);
      myPropertiesLoaded = true;
      loadView();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadView()
{
  if(!myPropertiesLoaded)
    return;

  string s;
  int i;

  // Cartridge properties
  s = myGameProperties.get(Cartridge_Name);
  myName->setEditString(s);

  s = myGameProperties.get(Cartridge_MD5);
  myMD5->setLabel(s);

  s = myGameProperties.get(Cartridge_Manufacturer);
  myManufacturer->setEditString(s);

  s = myGameProperties.get(Cartridge_ModelNo);
  myModelNo->setEditString(s);

  s = myGameProperties.get(Cartridge_Rarity);
  myRarity->setEditString(s);

  s = myGameProperties.get(Cartridge_Note);
  myNote->setEditString(s);

  s = myGameProperties.get(Cartridge_Sound);
  if(s == "MONO")
    mySound->setSelectedTag(1);
  else if(s == "STEREO")
    mySound->setSelectedTag(2);
  else
    mySound->setSelectedTag(0);

  s = myGameProperties.get(Cartridge_Type);
  for(i = 0; i < kNumCartTypes; ++i)
  {
    if(s == ourCartridgeList[i][1])
      break;
  }
  i = (i == kNumCartTypes) ? 0: i + 1;
  myType->setSelectedTag(i);

  // Console properties
  s = myGameProperties.get(Console_LeftDifficulty);
  if(s == "B")
    myLeftDiff->setSelectedTag(1);
  else if(s == "A")
    myLeftDiff->setSelectedTag(2);
  else
    myLeftDiff->setSelectedTag(0);

  s = myGameProperties.get(Console_RightDifficulty);
  if(s == "B")
    myRightDiff->setSelectedTag(1);
  else if(s == "A")
    myRightDiff->setSelectedTag(2);
  else
    myRightDiff->setSelectedTag(0);

  s = myGameProperties.get(Console_TelevisionType);
  if(s == "COLOR")
    myTVType->setSelectedTag(1);
  else if(s == "BLACKANDWHITE")
    myTVType->setSelectedTag(2);
  else
    myTVType->setSelectedTag(0);

  s = myGameProperties.get(Console_SwapPorts);
  myLeftPort->setSelectedTag(s == "NO" ? 1 : 2);
  myRightPort->setSelectedTag(s == "NO" ? 2 : 1);

  // Controller properties
  s = myGameProperties.get(Controller_Left);
  for(i = 0; i < kNumControllerTypes; ++i)
  {
    if(s == ourControllerList[i][1])
      break;
  }
  i = (i == kNumControllerTypes) ? 0: i + 1;
  myP0Controller->setSelectedTag(i);

  s = myGameProperties.get(Controller_Right);
  for(i = 0; i < kNumControllerTypes; ++i)
  {
    if(s == ourControllerList[i][1])
      break;
  }
  i = (i == kNumControllerTypes) ? 0: i + 1;
  myP1Controller->setSelectedTag(i);

  s = myGameProperties.get(Controller_SwapPaddles);
  if(s == "YES")
    mySwapPaddles->setSelectedTag(1);
  else if(s == "NO")
    mySwapPaddles->setSelectedTag(2);
  else
    mySwapPaddles->setSelectedTag(0);

  // Display properties
  s = myGameProperties.get(Display_Format);
  if(s == "AUTO-DETECT")
    myFormat->setSelectedTag(1);
  else if(s == "NTSC")
    myFormat->setSelectedTag(2);
  else if(s == "PAL")
    myFormat->setSelectedTag(3);
  else if(s == "SECAM")
    myFormat->setSelectedTag(4);
  else if(s == "NTSC50")
    myFormat->setSelectedTag(5);
  else if(s == "PAL60")
    myFormat->setSelectedTag(6);
  else if(s == "SECAM60")
    myFormat->setSelectedTag(7);
  else
    myFormat->setSelectedTag(0);

  s = myGameProperties.get(Display_YStart);
  myYStart->setEditString(s);

  s = myGameProperties.get(Display_Height);
  myHeight->setEditString(s);

  myPPBlend->setEnabled(false);
  myPPBlendLabel->setEnabled(false);
  s = myGameProperties.get(Display_Phosphor);
  if(s == "YES")
  {
    myPhosphor->setSelectedTag(1);
    myPPBlend->setEnabled(true);
    myPPBlendLabel->setEnabled(true);
  }
  else if(s == "NO")
    myPhosphor->setSelectedTag(2);
  else
    myPhosphor->setSelectedTag(0);

  s = myGameProperties.get(Display_PPBlend);
  myPPBlend->setValue(atoi(s.c_str()));
  myPPBlendLabel->setLabel(s);

  s = myGameProperties.get(Emulation_HmoveBlanks);
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
  if(!myPropertiesLoaded)
    return;

  string s;
  int i, tag;

  // Cartridge properties
  s = myName->getEditString();
  myGameProperties.set(Cartridge_Name, s);

  s = myManufacturer->getEditString();
  myGameProperties.set(Cartridge_Manufacturer, s);

  s = myModelNo->getEditString();
  myGameProperties.set(Cartridge_ModelNo, s);

  s = myRarity->getEditString();
  myGameProperties.set(Cartridge_Rarity, s);

  s = myNote->getEditString();
  myGameProperties.set(Cartridge_Note, s);

  tag = mySound->getSelectedTag();
  s = (tag == 1) ? "Mono" : "Stereo";
  myGameProperties.set(Cartridge_Sound, s);

  tag = myType->getSelectedTag();
  for(i = 0; i < kNumCartTypes; ++i)
  {
    if(i == tag-1)
    {
      myGameProperties.set(Cartridge_Type, ourCartridgeList[i][1]);
      break;
    }
  }

  // Console properties
  tag = myLeftDiff->getSelectedTag();
  s = (tag == 1) ? "B" : "A";
  myGameProperties.set(Console_LeftDifficulty, s);

  tag = myRightDiff->getSelectedTag();
  s = (tag == 1) ? "B" : "A";
  myGameProperties.set(Console_RightDifficulty, s);

  tag = myTVType->getSelectedTag();
  s = (tag == 1) ? "Color" : "BlackAndWhite";
  myGameProperties.set(Console_TelevisionType, s);

  // Controller properties
  tag = myP0Controller->getSelectedTag();
  for(i = 0; i < kNumControllerTypes; ++i)
  {
    if(i == tag-1)
    {
      myGameProperties.set(Controller_Left, ourControllerList[i][1]);
      break;
    }
  }

  tag = myP1Controller->getSelectedTag();
  for(i = 0; i < kNumControllerTypes; ++i)
  {
    if(i == tag-1)
    {
      myGameProperties.set(Controller_Right, ourControllerList[i][1]);
      break;
    }
  }

  tag = myLeftPort->getSelectedTag();
  s = (tag == 1) ? "No" : "Yes";
  myGameProperties.set(Console_SwapPorts, s);

  tag = mySwapPaddles->getSelectedTag();
  s = (tag == 1) ? "Yes" : "No";
  myGameProperties.set(Controller_SwapPaddles, s);

  // Display properties
  tag = myFormat->getSelectedTag();
  s = (tag == 7) ? "SECAM60" : (tag == 6) ? "PAL60" : (tag == 5) ? "NTSC50" :
      (tag == 4) ? "SECAM"   : (tag == 3) ? "PAL"   : (tag == 2) ? "NTSC" :
      "AUTO-DETECT";
  myGameProperties.set(Display_Format, s);

  s = myYStart->getEditString();
  myGameProperties.set(Display_YStart, s);

  s = myHeight->getEditString();
  myGameProperties.set(Display_Height, s);

  tag = myPhosphor->getSelectedTag();
  s = (tag == 1) ? "Yes" : "No";
  myGameProperties.set(Display_Phosphor, s);

  s = myPPBlendLabel->getLabel();
  myGameProperties.set(Display_PPBlend, s);

  tag = myHmoveBlanks->getSelectedTag();
  s = (tag == 1) ? "Yes" : "No";
  myGameProperties.set(Emulation_HmoveBlanks, s);

  // Determine whether to add or remove an entry from the properties set
  if(myDefaultsSelected)
    instance()->propSet().removeMD5(myGameProperties.get(Cartridge_MD5));
  else
    instance()->propSet().insert(myGameProperties, true);

  // In any event, inform the Console and save the properties
  if(&myOSystem->console())
    instance()->console().setProperties(myGameProperties);
  instance()->propSet().save(myOSystem->propertiesFile());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::setDefaults()
{
  // Load the default properties
  string md5 = myGameProperties.get(Cartridge_MD5);
  instance()->propSet().getMD5(md5, myGameProperties, true);

  // Reload the current dialog
  loadView();
  myDefaultsSelected = true;
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

    case kDefaultsCmd:
      setDefaults();
      break;

    case kLeftCChanged:
      myRightPort->setSelectedTag(
        myLeftPort->getSelectedTag() == 2 ? 1 : 2);
      break;

    case kRightCChanged:
      myLeftPort->setSelectedTag(
        myRightPort->getSelectedTag() == 2 ? 1 : 2);
      break;

    case kPhosphorChanged:
    {
      bool status = myPhosphor->getSelectedTag() == 1 ? true : false;
      myPPBlend->setEnabled(status);
      myPPBlendLabel->setEnabled(status);
      break;
    }

    case kPPBlendChanged:
      myPPBlendLabel->setValue(myPPBlend->getValue());
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* GameInfoDialog::ourControllerList[kNumControllerTypes][2] = {
  { "Joystick",       "JOYSTICK"    },
  { "Paddles",        "PADDLES"     },
  { "BoosterGrip",    "BOOSTERGRIP" },
  { "Driving",        "DRIVING"     },
  { "Keyboard",       "KEYBOARD"    },
  { "CX-22 Trakball", "TRACKBALL22" },
  { "CX-80 Mouse",    "TRACKBALL80" },
  { "AmigaMouse",     "AMIGAMOUSE"  },
  { "AtariVox",       "ATARIVOX"    },
  { "SaveKey",        "SAVEKEY"     }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* GameInfoDialog::ourCartridgeList[kNumCartTypes][2] = {
  { "Auto-detect",       "AUTO-DETECT"   },
  { "0840 (8K ECONObanking)",     "0840" },
  { "2K (2K Atari)",              "2K"   },
  { "3E (32K Tigervision)",       "3E"   },
  { "3F (512K Tigervision)",      "3F"   },
  { "4A50 (64K 4A50 + ram)",      "4A50" },
  { "4K (4K Atari)",              "4K"   },
  { "AR (Supercharger)",          "AR"   },
  { "CV (Commavid extra ram)",    "CV"   },
  { "DPC (Pitfall II)",           "DPC"  },
  { "E0 (8K Parker Bros)",        "E0"   },
  { "E7 (16K M-network)",         "E7"   },
  { "F4 (32K Atari)",             "F4"   },
  { "F4SC (32K Atari + ram)",     "F4SC" },
  { "F6 (16K Atari)",             "F6"   },
  { "F6SC (16K Atari + ram)",     "F6SC" },
  { "F8 (8K Atari)",              "F8"   },
  { "F8SC (8K Atari + ram)",      "F8SC" },
  { "FASC (CBS RAM Plus)",        "FASC" },
  { "FE (8K Decathlon)",          "FE"   },
  { "MB (Dynacom Megaboy)",       "MB"   },
  { "MC (C. Wilkson Megacart)",   "MC"   },
  { "SB (128-256k SUPERbanking)", "SB"   },
  { "UA (8K UA Ltd.)",            "UA"   },
  { "X07 (64K AtariAge)",         "X07"  }
};
