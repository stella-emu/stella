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
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
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
#include "StringList.hxx"
#include "TabWidget.hxx"
#include "Widget.hxx"

#include "GameInfoDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GameInfoDialog::GameInfoDialog(
      OSystem* osystem, DialogContainer* parent, const GUI::Font& font,
      GuiObject* boss)
  : Dialog(osystem, parent, 0, 0, 0, 0),
    CommandSender(boss),
    myPropertiesLoaded(false),
    myDefaultsSelected(false)
{
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            fontHeight   = font.getFontHeight(),
            buttonWidth  = font.getStringWidth("Defaults") + 20,
            buttonHeight = font.getLineHeight() + 4;
  const int vBorder = 4;
  int xpos, ypos, lwidth, fwidth, pwidth, tabID;
  WidgetArray wid;
  StringMap items, ports, ctrls;

  // Set real dimensions
  _w = 52 * fontWidth + 8;
  _h = 12 * (lineHeight + 4) + 10;

  ////////////////////////////////////////////////////////////////////
  // Some of the following items are also present in GlobalPropsDialog
  // If any changes are ever made here, GlobalPropsDialog should also
  // be updated accordingly
  ////////////////////////////////////////////////////////////////////

  // The tab widget
  xpos = 2; ypos = vBorder;
  myTab = new TabWidget(this, font, xpos, ypos, _w - 2*xpos,
                        _h - buttonHeight - 2*fontHeight - 20);
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
  items.clear();
  items.push_back("Mono", "MONO");
  items.push_back("Stereo", "STEREO");
  mySound = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                            pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(mySound);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Type:", kTextAlignLeft);
  pwidth = font.getStringWidth("EFSC (64K H. Runner + ram)");
  items.clear();
  items.push_back("Auto-detect",          "AUTO-DETECT");
  items.push_back("0840 (8K ECONObank)",        "0840" );
  items.push_back("2IN1 Multicart (4-32K)",     "2IN1" );
  items.push_back("4IN1 Multicart (8-32K)",     "4IN1" );
  items.push_back("8IN1 Multicart (16-64K)",    "8IN1" );
  items.push_back("32IN1 Multicart (64-128K)",  "32IN1");
  items.push_back("2K (64-2048 bytes Atari)",   "2K"   );
  items.push_back("3E (32K Tigervision)",       "3E"   );
  items.push_back("3F (512K Tigervision)",      "3F"   );
  items.push_back("4A50 (64K 4A50 + ram)",      "4A50" );
  items.push_back("4K (4K Atari)",              "4K"   );
  items.push_back("AR (Supercharger)",          "AR"   );
  items.push_back("CV (Commavid extra ram)",    "CV"   );
  items.push_back("DPC (Pitfall II)",           "DPC"  );
  items.push_back("DPC+ (Enhanced DPC)",        "DPC+" );
  items.push_back("E0 (8K Parker Bros)",        "E0"   );
  items.push_back("E7 (16K M-network)",         "E7"   );
  items.push_back("EF (64K H. Runner)",         "EF"   );
  items.push_back("EFSC (64K H. Runner + ram)", "EFSC" );
  items.push_back("F0 (Dynacom Megaboy)",       "F0"   );
  items.push_back("F4 (32K Atari)",             "F4"   );
  items.push_back("F4SC (32K Atari + ram)",     "F4SC" );
  items.push_back("F6 (16K Atari)",             "F6"   );
  items.push_back("F6SC (16K Atari + ram)",     "F6SC" );
  items.push_back("F8 (8K Atari)",              "F8"   );
  items.push_back("F8SC (8K Atari + ram)",      "F8SC" );
  items.push_back("FA (CBS RAM Plus)",          "FA"   );
  items.push_back("FE (8K Decathlon)",          "FE"   );
  items.push_back("MC (C. Wilkson Megacart)",   "MC"   );
  items.push_back("SB (128-256K SUPERbank)",    "SB"   );
  items.push_back("UA (8K UA Ltd.)",            "UA"   );
  items.push_back("X07 (64K AtariAge)",         "X07"  );
  myType = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                           pwidth, lineHeight, items, "", 0, 0);
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
  items.clear();
  items.push_back("B", "B");
  items.push_back("A", "A");
  myLeftDiff = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                               pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(myLeftDiff);

  ypos += lineHeight + 5;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Right Difficulty:", kTextAlignLeft);
  // ... use same items as above
  myRightDiff = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                                pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(myRightDiff);

  ypos += lineHeight + 5;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "TV Type:", kTextAlignLeft);
  items.clear();
  items.push_back("Color", "COLOR");
  items.push_back("B & W", "BLACKANDWHITE");
  myTVType = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                             pwidth, lineHeight, items, "", 0, 0);
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
  ctrls.clear();
  ctrls.push_back("Joystick",       "JOYSTICK"    );
  ctrls.push_back("Paddles",        "PADDLES"     );
  ctrls.push_back("BoosterGrip",    "BOOSTERGRIP" );
  ctrls.push_back("Driving",        "DRIVING"     );
  ctrls.push_back("Keyboard",       "KEYBOARD"    );
  ctrls.push_back("CX-22 Trakball", "TRACKBALL22" );
  ctrls.push_back("CX-80 Mouse",    "TRACKBALL80" );
  ctrls.push_back("AmigaMouse",     "AMIGAMOUSE"  );
  ctrls.push_back("AtariVox",       "ATARIVOX"    );
  ctrls.push_back("SaveKey",        "SAVEKEY"     );
  ctrls.push_back("Sega Genesis",   "GENESIS"     );
// TODO  ctrls.push_back("KidVid",         "KIDVID"      );
  myP0Controller = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                                   pwidth, lineHeight, ctrls, "", 0, 0);
  wid.push_back(myP0Controller);

  xpos += lwidth+myP0Controller->getWidth() + 4;
  new StaticTextWidget(myTab, font, xpos, ypos+1, font.getStringWidth("in "),
                       fontHeight, "in ", kTextAlignLeft);
  xpos += font.getStringWidth("in ");
  pwidth = font.getStringWidth("right port");
  ports.clear();
  ports.push_back("left port", "L");
  ports.push_back("right port", "R");
  myLeftPort = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                               ports, "", 0, kLeftCChanged);
  wid.push_back(myLeftPort);

  xpos = 10;  ypos += lineHeight + 5;
  pwidth = font.getStringWidth("CX-22 Trakball");
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "P1 Controller:", kTextAlignLeft);
  myP1Controller = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                                   pwidth, lineHeight, ctrls, "", 0, 0);
  wid.push_back(myP1Controller);

  xpos += lwidth+myP1Controller->getWidth() + 4;
  pwidth = font.getStringWidth("right port");
  new StaticTextWidget(myTab, font, xpos, ypos+1, font.getStringWidth("in "),
                       fontHeight, "in ", kTextAlignLeft);
  xpos += font.getStringWidth("in ");
  myRightPort = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                ports, "", 0, kRightCChanged);
  wid.push_back(myRightPort);

  xpos = 10;  ypos += lineHeight + 5;
  pwidth = font.getStringWidth("Yes");
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Swap Paddles:", kTextAlignLeft);
  items.clear();
  items.push_back("Yes", "YES");
  items.push_back("No", "NO");
  mySwapPaddles = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                                  pwidth, lineHeight, items, "", 0, 0);
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
  items.clear();
  items.push_back("Auto-detect", "AUTO-DETECT");
  items.push_back("NTSC",    "NTSC");
  items.push_back("PAL",     "PAL");
  items.push_back("SECAM",   "SECAM");
  items.push_back("NTSC50",  "NTSC50");
  items.push_back("PAL60",   "PAL60");
  items.push_back("SECAM60", "SECAM60");
  myFormat = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                             pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(myFormat);

  ypos += lineHeight + 5;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "YStart:", kTextAlignLeft);
  myYStart = new EditTextWidget(myTab, font, xpos+lwidth, ypos,
                                4*fontWidth, fontHeight, "");
  wid.push_back(myYStart);

  ypos += lineHeight + 5;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Height:", kTextAlignLeft);
  myHeight = new EditTextWidget(myTab, font, xpos+lwidth, ypos,
                                4*fontWidth, fontHeight, "");
  wid.push_back(myHeight);

  ypos += lineHeight + 5;
  pwidth = font.getStringWidth("Yes");
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Use Phosphor:", kTextAlignLeft);
  items.clear();
  items.push_back("Yes", "YES");
  items.push_back("No", "NO");
  myPhosphor = new PopUpWidget(myTab, font, xpos+lwidth, ypos, pwidth,
                               lineHeight, items, "", 0, kPhosphorChanged);
  wid.push_back(myPhosphor);

  myPPBlend = new SliderWidget(myTab, font, xpos + lwidth + myPhosphor->getWidth() + 10,
                               ypos, 8*fontWidth, lineHeight, "Blend: ",
                               font.getStringWidth("Blend: "),
                               kPPBlendChanged);
  myPPBlend->setMinValue(1); myPPBlend->setMaxValue(100);
  wid.push_back(myPPBlend);

  myPPBlendLabel = new StaticTextWidget(myTab, font,
                                        xpos + lwidth + myPhosphor->getWidth() + 10 + \
                                        myPPBlend->getWidth() + 4, ypos+1,
                                        3*fontWidth, fontHeight, "", kTextAlignLeft);
  myPPBlendLabel->setFlags(WIDGET_CLEARBG);

  // Add items for tab 3
  addToFocusList(wid, tabID);


  // Activate the first tab
  myTab->setActiveTab(0);

  // Add message concerning usage
  lwidth = font.getStringWidth("(*) Changes to properties require a ROM reload");
  new StaticTextWidget(this, font, 10, _h - buttonHeight - fontHeight - 20,
                       lwidth, fontHeight,
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

  if(&instance().console())
  {
    myGameProperties = instance().console().properties();
    myPropertiesLoaded = true;
    loadView();
  }
  else if(&instance().launcher())
  {
    const string& md5 = instance().launcher().selectedRomMD5();
    if(md5 != "")
    {
      instance().propSet().getMD5(md5, myGameProperties);
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

  // Cartridge properties
  myName->setEditString(myGameProperties.get(Cartridge_Name));
  myMD5->setLabel(myGameProperties.get(Cartridge_MD5));
  myManufacturer->setEditString(myGameProperties.get(Cartridge_Manufacturer));
  myModelNo->setEditString(myGameProperties.get(Cartridge_ModelNo));
  myRarity->setEditString(myGameProperties.get(Cartridge_Rarity));
  myNote->setEditString(myGameProperties.get(Cartridge_Note));
  mySound->setSelected(myGameProperties.get(Cartridge_Sound), "MONO");
  myType->setSelected(myGameProperties.get(Cartridge_Type), "AUTO-DETECT");

  // Console properties
  myLeftDiff->setSelected(myGameProperties.get(Console_LeftDifficulty), "B");
  myRightDiff->setSelected(myGameProperties.get(Console_RightDifficulty), "B");
  myTVType->setSelected(myGameProperties.get(Console_TelevisionType), "COLOR");

  const string& swap = myGameProperties.get(Console_SwapPorts);
  myLeftPort->setSelected((swap == "NO" ? "L" : "R"), "L");
  myRightPort->setSelected((swap == "NO" ? "R" : "L"), "R");

  // Controller properties
  myP0Controller->setSelected(myGameProperties.get(Controller_Left), "JOYSTICK");
  myP1Controller->setSelected(myGameProperties.get(Controller_Right), "JOYSTICK");
  mySwapPaddles->setSelected(myGameProperties.get(Controller_SwapPaddles), "NO");

  // Display properties
  myFormat->setSelected(myGameProperties.get(Display_Format), "AUTO-DETECT");
  myYStart->setEditString(myGameProperties.get(Display_YStart));
  myHeight->setEditString(myGameProperties.get(Display_Height));

  const string& phos = myGameProperties.get(Display_Phosphor);
  myPhosphor->setSelected(phos, "NO");
  myPPBlend->setEnabled(phos != "NO");
  myPPBlendLabel->setEnabled(phos != "NO");

  const string& blend = myGameProperties.get(Display_PPBlend);
  myPPBlend->setValue(atoi(blend.c_str()));
  myPPBlendLabel->setLabel(blend);

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::saveConfig()
{
  if(!myPropertiesLoaded)
    return;

  // Cartridge properties
  myGameProperties.set(Cartridge_Name, myName->getEditString());
  myGameProperties.set(Cartridge_Manufacturer, myManufacturer->getEditString());
  myGameProperties.set(Cartridge_ModelNo, myModelNo->getEditString());
  myGameProperties.set(Cartridge_Rarity, myRarity->getEditString());
  myGameProperties.set(Cartridge_Note, myNote->getEditString());
  myGameProperties.set(Cartridge_Sound, mySound->getSelectedTag());
  myGameProperties.set(Cartridge_Type, myType->getSelectedTag());

  // Console properties
  myGameProperties.set(Console_LeftDifficulty, myLeftDiff->getSelectedTag());
  myGameProperties.set(Console_RightDifficulty, myRightDiff->getSelectedTag());
  myGameProperties.set(Console_TelevisionType, myTVType->getSelectedTag());

  // Controller properties
  myGameProperties.set(Controller_Left, myP0Controller->getSelectedTag());
  myGameProperties.set(Controller_Right, myP1Controller->getSelectedTag());
  myGameProperties.set(Console_SwapPorts,
    myLeftPort->getSelectedTag() == "L" ? "NO" : "YES");
  myGameProperties.set(Controller_SwapPaddles, mySwapPaddles->getSelectedTag());

  // Display properties
  myGameProperties.set(Display_Format, myFormat->getSelectedTag());
  myGameProperties.set(Display_YStart, myYStart->getEditString());
  myGameProperties.set(Display_Height, myHeight->getEditString());
  myGameProperties.set(Display_Phosphor, myPhosphor->getSelectedTag());
  myGameProperties.set(Display_PPBlend, myPPBlendLabel->getLabel());

  // Determine whether to add or remove an entry from the properties set
  if(myDefaultsSelected)
    instance().propSet().removeMD5(myGameProperties.get(Cartridge_MD5));
  else
    instance().propSet().insert(myGameProperties);

  // In any event, inform the Console and save the properties
  if(&instance().console())
    instance().console().setProperties(myGameProperties);
  instance().propSet().save(instance().propertiesFile());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::setDefaults()
{
  // Load the default properties
  string md5 = myGameProperties.get(Cartridge_MD5);
  instance().propSet().getMD5(md5, myGameProperties, true);

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
      myRightPort->setSelected(
        myLeftPort->getSelected() == 1 ? 0 : 1);
      break;

    case kRightCChanged:
      myLeftPort->setSelected(
        myRightPort->getSelected() == 1 ? 0 : 1);
      break;

    case kPhosphorChanged:
    {
      bool status = myPhosphor->getSelectedTag() == "YES";
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
