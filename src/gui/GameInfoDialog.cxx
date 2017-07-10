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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "BSType.hxx"
#include "Console.hxx"
#include "MouseControl.hxx"
#include "Dialog.hxx"
#include "EditTextWidget.hxx"
#include "Launcher.hxx"
#include "OSystem.hxx"
#include "PopUpWidget.hxx"
#include "Props.hxx"
#include "PropsSet.hxx"
#include "TabWidget.hxx"
#include "FrameManager.hxx"
#include "Widget.hxx"

#include "GameInfoDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GameInfoDialog::GameInfoDialog(
      OSystem& osystem, DialogContainer& parent, const GUI::Font& font,
      GuiObject* boss)
  : Dialog(osystem, parent),
    CommandSender(boss),
    myPropertiesLoaded(false),
    myDefaultsSelected(false)
{
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            fontHeight   = font.getFontHeight(),
            buttonWidth  = font.getStringWidth("Defaults") + 20,
            buttonHeight = font.getLineHeight() + 4;
  const int vBorder = 4;
  int xpos, ypos, lwidth, fwidth, pwidth, tabID;
  WidgetArray wid;
  VariantList items, ports, ctrls;

  // Set real dimensions
  _w = 52 * fontWidth + 8;
  _h = 12 * (lineHeight + 4) + 10;

  // The tab widget
  xpos = 2; ypos = vBorder;
  myTab = new TabWidget(this, font, xpos, ypos, _w - 2*xpos,
                        _h - buttonHeight - fontHeight - ifont.getLineHeight() - 20);
  addTabWidget(myTab);

  // 1) Cartridge properties
  tabID = myTab->addTab("Cartridge");

  xpos = 10;
  lwidth = font.getStringWidth("Manufacturer ");
  fwidth = _w - xpos - lwidth - 10;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Name", kTextAlignLeft);
  myName = new EditTextWidget(myTab, font, xpos+lwidth, ypos,
                              fwidth, fontHeight, "");
  wid.push_back(myName);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "MD5", kTextAlignLeft);
  myMD5 = new StaticTextWidget(myTab, font, xpos+lwidth, ypos,
                               fwidth, fontHeight,
                               "", kTextAlignLeft);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Manufacturer", kTextAlignLeft);
  myManufacturer = new EditTextWidget(myTab, font, xpos+lwidth, ypos,
                                      fwidth, fontHeight, "");
  wid.push_back(myManufacturer);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Model", kTextAlignLeft);
  myModelNo = new EditTextWidget(myTab, font, xpos+lwidth, ypos,
                                 fwidth, fontHeight, "");
  wid.push_back(myModelNo);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Rarity", kTextAlignLeft);
  myRarity = new EditTextWidget(myTab, font, xpos+lwidth, ypos,
                                fwidth, fontHeight, "");
  wid.push_back(myRarity);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Note", kTextAlignLeft);
  myNote = new EditTextWidget(myTab, font, xpos+lwidth, ypos,
                              fwidth, fontHeight, "");
  wid.push_back(myNote);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Sound", kTextAlignLeft);
  pwidth = font.getStringWidth("Stereo");
  items.clear();
  VarList::push_back(items, "Mono", "MONO");
  VarList::push_back(items, "Stereo", "STEREO");
  mySound = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                            pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(mySound);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Type", kTextAlignLeft);
  pwidth = font.getStringWidth("CM (SpectraVideo CompuMate)");
  items.clear();
  for(int i = 0; i < int(BSType::NumSchemes); ++i)
    VarList::push_back(items, BSList[i].desc, BSList[i].name);
  myType = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                           pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(myType);

  // Add items for tab 0
  addToFocusList(wid, myTab, tabID);


  // 2) Console properties
  wid.clear();
  tabID = myTab->addTab("Console");

  xpos = 10; ypos = vBorder;
  lwidth = font.getStringWidth("Right Difficulty ");
  pwidth = font.getStringWidth("B & W");
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Left Difficulty", kTextAlignLeft);
  items.clear();
  VarList::push_back(items, "B", "B");
  VarList::push_back(items, "A", "A");
  myLeftDiff = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                               pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(myLeftDiff);

  ypos += lineHeight + 5;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Right Difficulty", kTextAlignLeft);
  // ... use same items as above
  myRightDiff = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                                pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(myRightDiff);

  ypos += lineHeight + 5;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "TV Type", kTextAlignLeft);
  items.clear();
  VarList::push_back(items, "Color", "COLOR");
  VarList::push_back(items, "B & W", "BW");
  myTVType = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                             pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(myTVType);

  // Add items for tab 1
  addToFocusList(wid, myTab, tabID);


  // 3) Controller properties
  wid.clear();
  tabID = myTab->addTab("Controller");

  xpos = 10; ypos = vBorder;
  lwidth = font.getStringWidth("P0 Controller ");
  pwidth = font.getStringWidth("Paddles_IAxis");
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "P0 Controller", kTextAlignLeft);
  ctrls.clear();
  VarList::push_back(ctrls, "Joystick",      "JOYSTICK"     );
  VarList::push_back(ctrls, "Paddles",       "PADDLES"      );
  VarList::push_back(ctrls, "Paddles_IAxis", "PADDLES_IAXIS");
  VarList::push_back(ctrls, "Paddles_IDir",  "PADDLES_IDIR" );
  VarList::push_back(ctrls, "Paddles_IAxDr", "PADDLES_IAXDR");
  VarList::push_back(ctrls, "BoosterGrip",   "BOOSTERGRIP"  );
  VarList::push_back(ctrls, "Driving",       "DRIVING"      );
  VarList::push_back(ctrls, "Keyboard",      "KEYBOARD"     );
  VarList::push_back(ctrls, "AmigaMouse",    "AMIGAMOUSE"   );
  VarList::push_back(ctrls, "AtariMouse",    "ATARIMOUSE"   );
  VarList::push_back(ctrls, "Trakball",      "TRAKBALL"     );
  VarList::push_back(ctrls, "AtariVox",      "ATARIVOX"     );
  VarList::push_back(ctrls, "SaveKey",       "SAVEKEY"      );
  VarList::push_back(ctrls, "Sega Genesis",  "GENESIS"      );
  VarList::push_back(ctrls, "CompuMate",     "COMPUMATE"    );
//  VarList::push_back(ctrls, "KidVid",        "KIDVID"      );
  VarList::push_back(ctrls, "MindLink",      "MINDLINK"     );
  myP0Controller = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                                   pwidth, lineHeight, ctrls, "", 0, 0);
  wid.push_back(myP0Controller);

  xpos += lwidth+myP0Controller->getWidth() + 4;
  new StaticTextWidget(myTab, font, xpos, ypos+1, font.getStringWidth("in "),
                       fontHeight, "in ", kTextAlignLeft);
  xpos += font.getStringWidth("in ");
  pwidth = font.getStringWidth("right port");
  ports.clear();
  VarList::push_back(ports, "left port", "L");
  VarList::push_back(ports, "right port", "R");
  myLeftPort = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                               ports, "", 0, kLeftCChanged);
  wid.push_back(myLeftPort);

  xpos = 10;  ypos += lineHeight + 5;
  pwidth = font.getStringWidth("Paddles_IAxis");
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "P1 Controller", kTextAlignLeft);
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
                       "Swap Paddles", kTextAlignLeft);
  items.clear();
  VarList::push_back(items, "Yes", "YES");
  VarList::push_back(items, "No", "NO");
  mySwapPaddles = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                                  pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(mySwapPaddles);

  ypos += lineHeight + 8;
  lwidth = font.getStringWidth("Mouse axis mode ");
  pwidth = font.getStringWidth("Specific axis");
  items.clear();
  VarList::push_back(items, "Automatic", "AUTO");
  VarList::push_back(items, "Specific axis", "specific");
  myMouseControl =
    new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight, items,
                   "Mouse axis mode ", lwidth, kMCtrlChanged);
  wid.push_back(myMouseControl);

  // Mouse controller specific axis
  lwidth = font.getStringWidth("X-Axis is ");
  pwidth = font.getStringWidth("MindLink 0");
  items.clear();
  VarList::push_back(items, "None",       MouseControl::NoControl);
  VarList::push_back(items, "Paddle 0",   MouseControl::Paddle0);
  VarList::push_back(items, "Paddle 1",   MouseControl::Paddle1);
  VarList::push_back(items, "Paddle 2",   MouseControl::Paddle2);
  VarList::push_back(items, "Paddle 3",   MouseControl::Paddle3);
  VarList::push_back(items, "Driving 0",  MouseControl::Driving0);
  VarList::push_back(items, "Driving 1",  MouseControl::Driving1);
  VarList::push_back(items, "MindLink 0", MouseControl::MindLink0);
  VarList::push_back(items, "MindLink 1", MouseControl::MindLink1);

  xpos = 45;  ypos += lineHeight + 4;
  myMouseX = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight, items,
               "X-Axis is ", lwidth);
  wid.push_back(myMouseX);

  ypos += lineHeight + 4;
  myMouseY = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight, items,
               "Y-Axis is ", lwidth);
  wid.push_back(myMouseY);

  xpos = 10;  ypos += lineHeight + 8;
  lwidth = font.getStringWidth("Mouse axis range ");
  myMouseRange = new SliderWidget(myTab, font, xpos, ypos, 8*fontWidth, lineHeight,
                                  "Mouse axis range ", lwidth, kMRangeChanged);
  myMouseRange->setMinValue(1); myMouseRange->setMaxValue(100);
  wid.push_back(myMouseRange);

  myMouseRangeLabel = new StaticTextWidget(myTab, font,
                            xpos + myMouseRange->getWidth() + 4, ypos+1,
                            3*fontWidth, fontHeight, "", kTextAlignLeft);
  myMouseRangeLabel->setFlags(WIDGET_CLEARBG);

  // Add items for tab 2
  addToFocusList(wid, myTab, tabID);


  // 4) Display properties
  wid.clear();
  tabID = myTab->addTab("Display");

  xpos = 10; ypos = vBorder;
  lwidth = font.getStringWidth("Use Phosphor ");
  pwidth = font.getStringWidth("Auto-detect");
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Format", kTextAlignLeft);
  items.clear();
  VarList::push_back(items, "Auto-detect", "AUTO");
  VarList::push_back(items, "NTSC",    "NTSC");
  VarList::push_back(items, "PAL",     "PAL");
  VarList::push_back(items, "SECAM",   "SECAM");
  VarList::push_back(items, "NTSC50",  "NTSC50");
  VarList::push_back(items, "PAL60",   "PAL60");
  VarList::push_back(items, "SECAM60", "SECAM60");
  myFormat = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                             pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(myFormat);

  ypos += lineHeight + 5;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "YStart", kTextAlignLeft);

  myYStart = new SliderWidget(myTab, font, xpos+lwidth, ypos, 8*fontWidth, lineHeight,
                              "", 0, kYStartChanged);
  myYStart->setMinValue(FrameManager::minYStart-1);
  myYStart->setMaxValue(FrameManager::maxYStart);
  wid.push_back(myYStart);
  myYStartLabel = new StaticTextWidget(myTab, font, xpos+lwidth+myYStart->getWidth() + 4,
                                       ypos+1, 5*fontWidth, fontHeight, "", kTextAlignLeft);
  myYStartLabel->setFlags(WIDGET_CLEARBG);

  ypos += lineHeight + 5;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Height", kTextAlignLeft);
  myHeight = new SliderWidget(myTab, font, xpos+lwidth, ypos, 8*fontWidth, lineHeight,
                              "", 0, kHeightChanged);
  myHeight->setMinValue(FrameManager::minViewableHeight-1);
  myHeight->setMaxValue(FrameManager::maxViewableHeight);
  wid.push_back(myHeight);
  myHeightLabel = new StaticTextWidget(myTab, font, xpos+lwidth+myHeight->getWidth() + 4,
                                       ypos+1, 5*fontWidth, fontHeight, "", kTextAlignLeft);
  myHeightLabel->setFlags(WIDGET_CLEARBG);

  ypos += lineHeight + 5;
  pwidth = font.getStringWidth("Yes");
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Use Phosphor", kTextAlignLeft);
  items.clear();
  VarList::push_back(items, "Yes", "YES");
  VarList::push_back(items, "No", "NO");
  myPhosphor = new PopUpWidget(myTab, font, xpos+lwidth, ypos, pwidth,
                               lineHeight, items, "", 0, kPhosphorChanged);
  wid.push_back(myPhosphor);

  myPPBlend = new SliderWidget(myTab, font, xpos + lwidth + myPhosphor->getWidth() + 10,
                               ypos, 8*fontWidth, lineHeight, "Blend ",
                               font.getStringWidth("Blend "),
                               kPPBlendChanged);
  myPPBlend->setMinValue(0); myPPBlend->setMaxValue(100);
  wid.push_back(myPPBlend);

  myPPBlendLabel = new StaticTextWidget(myTab, font,
                                        xpos + lwidth + myPhosphor->getWidth() + 10 +
                                        myPPBlend->getWidth() + 4, ypos+1,
                                        5*fontWidth, fontHeight, "", kTextAlignLeft);
  myPPBlendLabel->setFlags(WIDGET_CLEARBG);

  // Add items for tab 3
  addToFocusList(wid, myTab, tabID);


  // Activate the first tab
  myTab->setActiveTab(0);

  // Add message concerning usage
  lwidth = ifont.getStringWidth("(*) Changes to properties require a ROM reload");
  new StaticTextWidget(this, ifont, 10, _h - buttonHeight - fontHeight - 20,
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
void GameInfoDialog::loadConfig()
{
  myPropertiesLoaded = false;
  myDefaultsSelected = false;

  if(instance().hasConsole())
  {
    myGameProperties = instance().console().properties();
    myPropertiesLoaded = true;
    loadView();
  }
  else
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
  myName->setText(myGameProperties.get(Cartridge_Name));
  myMD5->setLabel(myGameProperties.get(Cartridge_MD5));
  myManufacturer->setText(myGameProperties.get(Cartridge_Manufacturer));
  myModelNo->setText(myGameProperties.get(Cartridge_ModelNo));
  myRarity->setText(myGameProperties.get(Cartridge_Rarity));
  myNote->setText(myGameProperties.get(Cartridge_Note));
  mySound->setSelected(myGameProperties.get(Cartridge_Sound), "MONO");
  myType->setSelected(myGameProperties.get(Cartridge_Type), "AUTO");

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

  // MouseAxis property (potentially contains 'range' information)
  istringstream m_axis(myGameProperties.get(Controller_MouseAxis));
  string m_control, m_range;
  m_axis >> m_control;
  bool autoAxis = BSPF::equalsIgnoreCase(m_control, "AUTO");
  if(autoAxis)
  {
    myMouseControl->setSelectedIndex(0);
    myMouseX->setSelectedIndex(0);
    myMouseY->setSelectedIndex(0);
  }
  else
  {
    myMouseControl->setSelectedIndex(1);
    myMouseX->setSelected(m_control[0] - '0');
    myMouseY->setSelected(m_control[1] - '0');
  }
  myMouseX->setEnabled(!autoAxis);
  myMouseY->setEnabled(!autoAxis);
  if(m_axis >> m_range)
  {
    myMouseRange->setValue(atoi(m_range.c_str()));
    myMouseRangeLabel->setLabel(m_range);
  }
  else
  {
    myMouseRange->setValue(100);
    myMouseRangeLabel->setLabel("100");
  }

  // Display properties
  myFormat->setSelected(myGameProperties.get(Display_Format), "AUTO");

  const string& ystart = myGameProperties.get(Display_YStart);
  myYStart->setValue(atoi(ystart.c_str()));
  myYStartLabel->setLabel(ystart == "0" ? "Auto" : ystart);

  const string& height = myGameProperties.get(Display_Height);
  myHeight->setValue(atoi(height.c_str()));
  myHeightLabel->setLabel(height == "0" ? "Auto" : height);

  const string& phos = myGameProperties.get(Display_Phosphor);
  myPhosphor->setSelected(phos, "NO");
  myPPBlend->setEnabled(phos != "NO");
  myPPBlendLabel->setEnabled(phos != "NO");

  const string& blend = myGameProperties.get(Display_PPBlend);
  myPPBlend->setValue(atoi(blend.c_str()));
  myPPBlendLabel->setLabel(blend == "0" ? "Auto" : blend);

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::saveConfig()
{
  if(!myPropertiesLoaded)
    return;

  // Cartridge properties
  myGameProperties.set(Cartridge_Name, myName->getText());
  myGameProperties.set(Cartridge_Manufacturer, myManufacturer->getText());
  myGameProperties.set(Cartridge_ModelNo, myModelNo->getText());
  myGameProperties.set(Cartridge_Rarity, myRarity->getText());
  myGameProperties.set(Cartridge_Note, myNote->getText());
  myGameProperties.set(Cartridge_Sound, mySound->getSelectedTag().toString());
  myGameProperties.set(Cartridge_Type, myType->getSelectedTag().toString());

  // Console properties
  myGameProperties.set(Console_LeftDifficulty, myLeftDiff->getSelectedTag().toString());
  myGameProperties.set(Console_RightDifficulty, myRightDiff->getSelectedTag().toString());
  myGameProperties.set(Console_TelevisionType, myTVType->getSelectedTag().toString());

  // Controller properties
  myGameProperties.set(Controller_Left, myP0Controller->getSelectedTag().toString());
  myGameProperties.set(Controller_Right, myP1Controller->getSelectedTag().toString());
  myGameProperties.set(Console_SwapPorts,
    myLeftPort->getSelectedTag().toString() == "L" ? "NO" : "YES");
  myGameProperties.set(Controller_SwapPaddles, mySwapPaddles->getSelectedTag().toString());

  // MouseAxis property (potentially contains 'range' information)
  string mcontrol = myMouseControl->getSelectedTag().toString();
  if(mcontrol != "AUTO")
    mcontrol = myMouseX->getSelectedTag().toString() +
               myMouseY->getSelectedTag().toString();
  string range = myMouseRangeLabel->getLabel();
  if(range != "100")
    mcontrol += " " + range;
  myGameProperties.set(Controller_MouseAxis, mcontrol);

  // Display properties
  myGameProperties.set(Display_Format, myFormat->getSelectedTag().toString());
  myGameProperties.set(Display_YStart, myYStartLabel->getLabel() == "Auto" ? "0" :
                       myYStartLabel->getLabel());
  myGameProperties.set(Display_Height, myHeightLabel->getLabel() == "Auto" ? "0" :
                       myHeightLabel->getLabel());
  myGameProperties.set(Display_Phosphor, myPhosphor->getSelectedTag().toString());
  myGameProperties.set(Display_PPBlend, myPPBlendLabel->getLabel() == "Auto" ? "0" :
                       myPPBlendLabel->getLabel());

  // Determine whether to add or remove an entry from the properties set
  if(myDefaultsSelected)
    instance().propSet().removeMD5(myGameProperties.get(Cartridge_MD5));
  else
    instance().propSet().insert(myGameProperties);

  // In any event, inform the Console
  if(instance().hasConsole())
    instance().console().setProperties(myGameProperties);
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
      myRightPort->setSelectedIndex(
        myLeftPort->getSelected() == 1 ? 0 : 1);
      break;

    case kRightCChanged:
      myLeftPort->setSelectedIndex(
        myRightPort->getSelected() == 1 ? 0 : 1);
      break;

    case kPhosphorChanged:
    {
      bool status = myPhosphor->getSelectedTag().toString() == "YES";
      myPPBlend->setEnabled(status);
      myPPBlendLabel->setEnabled(status);
      break;
    }

    case kYStartChanged:
      if(myYStart->getValue() == FrameManager::minYStart-1)
        myYStartLabel->setLabel("Auto");
      else
        myYStartLabel->setValue(myYStart->getValue());
      break;

    case kHeightChanged:
      if(myHeight->getValue() == FrameManager::minViewableHeight-1)
        myHeightLabel->setLabel("Auto");
      else
        myHeightLabel->setValue(myHeight->getValue());
      break;

    case kPPBlendChanged:
      if(myPPBlend->getValue() == 0)
        myPPBlendLabel->setLabel("Auto");
      else
        myPPBlendLabel->setValue(myPPBlend->getValue());
      break;

    case kMRangeChanged:
      myMouseRangeLabel->setValue(myMouseRange->getValue());
      break;

    case kMCtrlChanged:
    {
      bool state = myMouseControl->getSelectedTag().toString() != "auto";
      myMouseX->setEnabled(state);
      myMouseY->setEnabled(state);
      break;
    }

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
