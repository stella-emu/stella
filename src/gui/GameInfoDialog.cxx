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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "BSType.hxx"
#include "Console.hxx"
#include "MouseControl.hxx"
#include "SaveKey.hxx"
#include "Dialog.hxx"
#include "EditTextWidget.hxx"
#include "RadioButtonWidget.hxx"
#include "Launcher.hxx"
#include "OSystem.hxx"
#include "PopUpWidget.hxx"
#include "Props.hxx"
#include "PropsSet.hxx"
#include "TabWidget.hxx"
#include "TIAConstants.hxx"
#include "Widget.hxx"
#include "Font.hxx"

#include "GameInfoDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GameInfoDialog::GameInfoDialog(
      OSystem& osystem, DialogContainer& parent, const GUI::Font& font,
      GuiObject* boss)
  : Dialog(osystem, parent, font, "Game properties"),
    CommandSender(boss),
    myPropertiesLoaded(false),
    myDefaultsSelected(false)
{
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            fontHeight   = font.getFontHeight(),
            buttonHeight = font.getLineHeight() + 4;
  const int vBorder = 8;
  const int hBorder = 8;

  const int hSpace = 10;
  const int VGAP = 4;

  int xpos, ypos, lwidth, fwidth, pwidth, swidth, tabID;
  WidgetArray wid;
  VariantList items, ports, ctrls;
  StaticTextWidget* t;

  // Set real dimensions
  _w = 53 * fontWidth + 8;
  _h = 9 * (lineHeight + VGAP) + vBorder * 2 + _th + buttonHeight + fontHeight + ifont.getLineHeight() + 20;

  // The tab widget
  xpos = hBorder; ypos = vBorder;
  myTab = new TabWidget(this, font, 2, 4 + _th, _w - 2 * 2,
                        _h - (_th + buttonHeight + fontHeight + ifont.getLineHeight() + 20));
  addTabWidget(myTab);

  //////////////////////////////////////////////////////////////////////////////
  // 1) Cartridge properties
  tabID = myTab->addTab("Cartridge");

  xpos = hSpace;
  lwidth = font.getStringWidth("Manufacturer ");
  fwidth = _w - xpos - lwidth - hSpace - hBorder * 2;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight, "Name");
  myName = new EditTextWidget(myTab, font, xpos+lwidth, ypos-1,
                              fwidth, lineHeight, "");
  wid.push_back(myName);

  ypos += lineHeight + VGAP;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight, "MD5");
  myMD5 = new EditTextWidget(myTab, font, xpos + lwidth, ypos-1,
                     fwidth, lineHeight, "");
  myMD5->setEditable(false);

  ypos += lineHeight + VGAP;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight, "Manufacturer");
  myManufacturer = new EditTextWidget(myTab, font, xpos+lwidth, ypos-1,
                                      fwidth, lineHeight, "");
  wid.push_back(myManufacturer);

  ypos += lineHeight + VGAP;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight,
                       "Model", TextAlign::Left);
  myModelNo = new EditTextWidget(myTab, font, xpos+lwidth, ypos-1,
                                 fwidth, lineHeight, "");
  wid.push_back(myModelNo);

  ypos += lineHeight + VGAP;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight, "Rarity");
  myRarity = new EditTextWidget(myTab, font, xpos+lwidth, ypos-1,
                                fwidth, lineHeight, "");
  wid.push_back(myRarity);

  ypos += lineHeight + VGAP;
  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight, "Note");
  myNote = new EditTextWidget(myTab, font, xpos+lwidth, ypos-1,
                              fwidth, lineHeight, "");
  wid.push_back(myNote);
  ypos += lineHeight + VGAP;

  new StaticTextWidget(myTab, font, xpos, ypos+1, lwidth, fontHeight, "Type");
  pwidth = font.getStringWidth("CM (SpectraVideo CompuMate)");
  items.clear();
  for(int i = 0; i < int(BSType::NumSchemes); ++i)
    VarList::push_back(items, BSList[i].desc, BSList[i].name);
  myType = new PopUpWidget(myTab, font, xpos+lwidth, ypos,
                           pwidth, lineHeight, items, "");
  wid.push_back(myType);
  ypos += lineHeight + VGAP;

  mySound = new CheckboxWidget(myTab, font, xpos, ypos + 1, "Stereo sound");
  wid.push_back(mySound);

  // Add items for tab 0
  addToFocusList(wid, myTab, tabID);

  //////////////////////////////////////////////////////////////////////////////
  // 2) Console properties
  wid.clear();
  tabID = myTab->addTab("Console");

  xpos = hSpace; ypos = vBorder;
  StaticTextWidget* s = new StaticTextWidget(myTab, font, xpos, ypos+1, "Left difficulty  ");
  myLeftDiffGroup = new RadioButtonGroup();
  RadioButtonWidget* r = new RadioButtonWidget(myTab, font, s->getRight(), ypos + 1,
                                               "A", myLeftDiffGroup);
  wid.push_back(r);
  ypos += lineHeight;
  r = new RadioButtonWidget(myTab, font, s->getRight(), ypos + 1,
                            "B", myLeftDiffGroup);
  wid.push_back(r);
  ypos += lineHeight + VGAP * 2;

  s = new StaticTextWidget(myTab, font, xpos, ypos+1, "Right difficulty ");
  myRightDiffGroup = new RadioButtonGroup();
  r = new RadioButtonWidget(myTab, font, s->getRight(), ypos + 1,
                            "A", myRightDiffGroup);
  wid.push_back(r);
  ypos += lineHeight;
  r = new RadioButtonWidget(myTab, font, s->getRight(), ypos + 1,
                            "B", myRightDiffGroup);
  wid.push_back(r);
  ypos += lineHeight + VGAP * 2;

  s = new StaticTextWidget(myTab, font, xpos, ypos+1, "TV type          ");
  myTVTypeGroup = new RadioButtonGroup();
  r = new RadioButtonWidget(myTab, font, s->getRight(), ypos + 1,
                            "Color", myTVTypeGroup);
  wid.push_back(r);
  ypos += lineHeight;
  r = new RadioButtonWidget(myTab, font, s->getRight(), ypos + 1,
                            "B/W", myTVTypeGroup);
  wid.push_back(r);

  // Add items for tab 1
  addToFocusList(wid, myTab, tabID);

  //////////////////////////////////////////////////////////////////////////////
  // 3) Controller properties
  wid.clear();
  tabID = myTab->addTab("Controller");

  ypos = vBorder;
  pwidth = font.getStringWidth("Paddles_IAxis");
  myP0Label = new StaticTextWidget(myTab, font, hSpace, ypos+1, "P0 controller    ");
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
//  VarList::push_back(ctrls, "KidVid",        "KIDVID"      );
  VarList::push_back(ctrls, "MindLink",      "MINDLINK"     );

  myP0Controller = new PopUpWidget(myTab, font, myP0Label->getRight(), myP0Label->getTop()-1,
                                   pwidth, lineHeight, ctrls, "", 0, kLeftCChanged);
  wid.push_back(myP0Controller);

  ypos += lineHeight + VGAP;
  pwidth = font.getStringWidth("Paddles_IAxis");
  myP1Label = new StaticTextWidget(myTab, font, hSpace, ypos+1, "P1 controller    ");
  myP1Controller = new PopUpWidget(myTab, font, myP1Label->getRight(), myP1Label->getTop()-1,
                                   pwidth, lineHeight, ctrls, "", 0, kRightCChanged);
  wid.push_back(myP1Controller);

  //ypos += lineHeight + VGAP;
  mySwapPorts = new CheckboxWidget(myTab, font, myP0Controller->getRight() + fontWidth*4, myP0Controller->getTop()+1,
                                   "Swap ports");
  wid.push_back(mySwapPorts);
  //ypos += lineHeight + VGAP;
  mySwapPaddles = new CheckboxWidget(myTab, font, myP1Controller->getRight() + fontWidth*4, myP1Controller->getTop()+1,
                                     "Swap paddles");
  wid.push_back(mySwapPaddles);

  // EEPROM erase button for P0/P1
  ypos += lineHeight + VGAP + 4;
  pwidth = myP1Controller->getWidth();   //font.getStringWidth("Erase EEPROM ") + 23;
  myEraseEEPROMLabel = new StaticTextWidget(myTab, font, hSpace, ypos, "AtariVox/SaveKey ");
  myEraseEEPROMButton = new ButtonWidget(myTab, font, myEraseEEPROMLabel->getRight(), ypos - 4,
                                         pwidth, buttonHeight, "Erase EEPROM", kEEButtonPressed);
  wid.push_back(myEraseEEPROMButton);
  myEraseEEPROMInfo = new StaticTextWidget(myTab, ifont, myEraseEEPROMButton->getRight() + 4,
                                           myEraseEEPROMLabel->getTop() + 3, "(for this game only)");

  ypos += lineHeight + VGAP * 4;
  myMouseControl = new CheckboxWidget(myTab, font, xpos, ypos + 1, "Specific mouse axes", kMCtrlChanged);
  wid.push_back(myMouseControl);

  // Mouse controller specific axis
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

  lwidth = font.getStringWidth("X-Axis is ");
  xpos += 20;
  ypos += lineHeight + VGAP;
  myMouseX = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight, items,
               "X-Axis is ");
  wid.push_back(myMouseX);

  ypos += lineHeight + VGAP;
  myMouseY = new PopUpWidget(myTab, font, myMouseX->getLeft(), ypos, pwidth, lineHeight, items,
               "Y-Axis is ");
  wid.push_back(myMouseY);

  xpos = hSpace; ypos += lineHeight + VGAP;
  lwidth = font.getStringWidth("Mouse axis range ");
  myMouseRange = new SliderWidget(myTab, font, hSpace, ypos,
                                  "Mouse axis range ", lwidth, 0, fontWidth * 4, "%");
  myMouseRange->setMinValue(1); myMouseRange->setMaxValue(100);
  wid.push_back(myMouseRange);

  // Add items for tab 2
  addToFocusList(wid, myTab, tabID);

  //////////////////////////////////////////////////////////////////////////////
  // 4) Display properties
  wid.clear();
  tabID = myTab->addTab("Display");

  ypos = vBorder;
  pwidth = font.getStringWidth("Auto-detect");
  t = new StaticTextWidget(myTab, font, hSpace, ypos+1, "Format  ");
  items.clear();
  VarList::push_back(items, "Auto-detect", "AUTO");
  VarList::push_back(items, "NTSC",    "NTSC");
  VarList::push_back(items, "PAL",     "PAL");
  VarList::push_back(items, "SECAM",   "SECAM");
  VarList::push_back(items, "NTSC50",  "NTSC50");
  VarList::push_back(items, "PAL60",   "PAL60");
  VarList::push_back(items, "SECAM60", "SECAM60");
  myFormat = new PopUpWidget(myTab, font, t->getRight(), ypos,
                             pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(myFormat);

  ypos += lineHeight + VGAP;
  swidth = myFormat->getWidth();
  t = new StaticTextWidget(myTab, font, hSpace, ypos+1, "Y-Start ");
  myYStart = new SliderWidget(myTab, font, t->getRight(), ypos, swidth, lineHeight,
                              "", 0, kYStartChanged, 4 * fontWidth, "px");
  myYStart->setMinValue(TIAConstants::minYStart-1);
  myYStart->setMaxValue(TIAConstants::maxYStart);
  wid.push_back(myYStart);

  ypos += lineHeight + VGAP;
  t = new StaticTextWidget(myTab, font, hSpace, ypos+1, "Height  ");
  myHeight = new SliderWidget(myTab, font, t->getRight(), ypos, swidth, lineHeight,
                              "", 0, kHeightChanged, 5 * fontWidth, "px");
  myHeight->setMinValue(TIAConstants::minViewableHeight-1);
  myHeight->setMaxValue(TIAConstants::maxViewableHeight);
  wid.push_back(myHeight);

  // Phosphor
  ypos += lineHeight + VGAP*4;
  myPhosphor = new CheckboxWidget(myTab, font, hSpace, ypos+1, "Phosphor", kPhosphorChanged);
  wid.push_back(myPhosphor);

  myPPBlend = new SliderWidget(myTab, font,
                               myPhosphor->getRight() + fontWidth * 3, myPhosphor->getTop()-2,
                               "Blend ", 0, kPPBlendChanged, 7 * fontWidth, "%");
  myPPBlend->setMinValue(0); myPPBlend->setMaxValue(100);
  wid.push_back(myPPBlend);

  // Add items for tab 3
  addToFocusList(wid, myTab, tabID);


  // Activate the first tab
  myTab->setActiveTab(0);

  // Add message concerning usage
  lwidth = ifont.getStringWidth("(*) Changes to properties require a ROM reload");
  new StaticTextWidget(this, ifont, hSpace, _h - (buttonHeight + fontHeight + 20),
                       "(*) Changes to properties require a ROM reload");

  // Add Defaults, OK and Cancel buttons
  wid.clear();
  addDefaultsOKCancelBGroup(wid, font);
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
      instance().propSet(md5).getMD5(md5, myGameProperties);
      myPropertiesLoaded = true;
      loadView();
    }
  }

  updateControllerStates();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadView()
{
  if(!myPropertiesLoaded)
    return;

  // Cartridge properties
  myName->setText(myGameProperties.get(Cartridge_Name));
  myMD5->setText(myGameProperties.get(Cartridge_MD5));
  myManufacturer->setText(myGameProperties.get(Cartridge_Manufacturer));
  myModelNo->setText(myGameProperties.get(Cartridge_ModelNo));
  myRarity->setText(myGameProperties.get(Cartridge_Rarity));
  myNote->setText(myGameProperties.get(Cartridge_Note));
  mySound->setState(myGameProperties.get(Cartridge_Sound) == "STEREO");
  myType->setSelected(myGameProperties.get(Cartridge_Type), "AUTO");

  // Console properties
  myLeftDiffGroup->setSelected(myGameProperties.get(Console_LeftDifficulty) == "A" ? 0 : 1);
  myRightDiffGroup->setSelected(myGameProperties.get(Console_RightDifficulty) == "A" ? 0 : 1);
  myTVTypeGroup->setSelected(myGameProperties.get(Console_TelevisionType) == "BW" ? 1 : 0);

  // Controller properties
  myP0Controller->setSelected(myGameProperties.get(Controller_Left), "JOYSTICK");
  myP1Controller->setSelected(myGameProperties.get(Controller_Right), "JOYSTICK");
  mySwapPorts->setState(myGameProperties.get(Console_SwapPorts) == "YES");
  mySwapPaddles->setState(myGameProperties.get(Controller_SwapPaddles) == "YES");

  // MouseAxis property (potentially contains 'range' information)
  istringstream m_axis(myGameProperties.get(Controller_MouseAxis));
  string m_control, m_range;
  m_axis >> m_control;
  bool autoAxis = BSPF::equalsIgnoreCase(m_control, "AUTO");
  myMouseControl->setState(!autoAxis);
  if(autoAxis)
  {
    //myMouseControl->setSelectedIndex(0);
    myMouseX->setSelectedIndex(0);
    myMouseY->setSelectedIndex(0);
  }
  else
  {
    //myMouseControl->setSelectedIndex(1);
    myMouseX->setSelected(m_control[0] - '0');
    myMouseY->setSelected(m_control[1] - '0');
  }
  myMouseX->setEnabled(!autoAxis);
  myMouseY->setEnabled(!autoAxis);
  if(m_axis >> m_range)
  {
    myMouseRange->setValue(atoi(m_range.c_str()));
  }
  else
  {
    myMouseRange->setValue(100);
  }

  // Display properties
  myFormat->setSelected(myGameProperties.get(Display_Format), "AUTO");

  const string& ystart = myGameProperties.get(Display_YStart);
  myYStart->setValue(atoi(ystart.c_str()));
  myYStart->setValueLabel(ystart == "0" ? "Auto" : ystart);
  myYStart->setValueUnit(ystart == "0" ? "" : "px");

  const string& height = myGameProperties.get(Display_Height);
  myHeight->setValue(atoi(height.c_str()));
  myHeight->setValueLabel(height == "0" ? "Auto" : height);
  myHeight->setValueUnit(height == "0" ? "" : "px");

  bool usePhosphor = myGameProperties.get(Display_Phosphor) == "YES";
  myPhosphor->setState(usePhosphor);
  myPPBlend->setEnabled(usePhosphor);

  const string& blend = myGameProperties.get(Display_PPBlend);
  myPPBlend->setValue(atoi(blend.c_str()));
  myPPBlend->setValueLabel(blend == "0" ? "Default" : blend);
  myPPBlend->setValueUnit(blend == "0" ? "" : "%");

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
  myGameProperties.set(Cartridge_Sound, mySound->getState() ? "STEREO" : "MONO");
  myGameProperties.set(Cartridge_Type, myType->getSelectedTag().toString());

  // Console properties
  myGameProperties.set(Console_LeftDifficulty, myLeftDiffGroup->getSelected() ? "B" : "A");
  myGameProperties.set(Console_RightDifficulty, myRightDiffGroup->getSelected() ? "B" : "A");
  myGameProperties.set(Console_TelevisionType, myTVTypeGroup->getSelected() ? "BW" : "COLOR");

  // Controller properties
  myGameProperties.set(Controller_Left, myP0Controller->getSelectedTag().toString());
  myGameProperties.set(Controller_Right, myP1Controller->getSelectedTag().toString());
  myGameProperties.set(Console_SwapPorts, (mySwapPorts->isEnabled() && mySwapPorts->getState()) ? "YES" : "NO");
  myGameProperties.set(Controller_SwapPaddles, (mySwapPaddles->isEnabled() && mySwapPaddles->getState()) ? "YES" : "NO");

  // MouseAxis property (potentially contains 'range' information)
  string mcontrol = "AUTO";
  if(myMouseControl->getState())
    mcontrol = myMouseX->getSelectedTag().toString() +
               myMouseY->getSelectedTag().toString();
  string range = myMouseRange->getValueLabel();
  if(range != "100")
    mcontrol += " " + range;
  myGameProperties.set(Controller_MouseAxis, mcontrol);

  // Display properties
  myGameProperties.set(Display_Format, myFormat->getSelectedTag().toString());
  myGameProperties.set(Display_YStart, myYStart->getValueLabel() == "Auto" ? "0" :
                       myYStart->getValueLabel());
  myGameProperties.set(Display_Height, myHeight->getValueLabel() == "Auto" ? "0" :
                       myHeight->getValueLabel());
  myGameProperties.set(Display_Phosphor, myPhosphor->getState() ? "YES" : "NO");

  myGameProperties.set(Display_PPBlend, myPPBlend->getValueLabel() == "Default" ? "0" :
                       myPPBlend->getValueLabel());

  // Determine whether to add or remove an entry from the properties set
  const string& md5 = myGameProperties.get(Cartridge_MD5);
  if(myDefaultsSelected)
    instance().propSet(md5).removeMD5(myGameProperties.get(Cartridge_MD5));
  else
    instance().propSet(md5).insert(myGameProperties);

  instance().saveGamePropSet(myGameProperties.get(Cartridge_MD5));

  // In any event, inform the Console
  if(instance().hasConsole())
    instance().console().setProperties(myGameProperties);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::setDefaults()
{
  // Load the default properties
  string md5 = myGameProperties.get(Cartridge_MD5);
  instance().propSet(md5).getMD5(md5, myGameProperties, true);

  // Reload the current dialog
  loadView();
  myDefaultsSelected = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::updateControllerStates()
{
  const string& contrP0 = myP0Controller->getSelectedTag().toString();
  const string& contrP1 = myP1Controller->getSelectedTag().toString();
  bool enableEEEraseButton = false;
  bool enableSwapPaddles = false;
  bool enableSwapPorts = false;

  // Compumate bankswitching scheme doesn't allow to select controllers
  bool enableSelectControl = myType->getSelectedTag() != "CM";

  enableSwapPorts = enableSelectControl;
  enableSwapPaddles = BSPF::startsWithIgnoreCase(contrP0, "PADDLES") ||
    BSPF::startsWithIgnoreCase(contrP1, "PADDLES");

  if(instance().hasConsole())
  {
    const Controller& lport = instance().console().leftController();
    const Controller& rport = instance().console().rightController();

    // we only enable the button if we have a valid previous and new controller.
    enableEEEraseButton = ((lport.type() == Controller::SaveKey && contrP0 == "SAVEKEY")
                           || (lport.type() == Controller::AtariVox && contrP0 == "ATARIVOX")
                           || (rport.type() == Controller::SaveKey && contrP1 == "SAVEKEY")
                           || (rport.type() == Controller::AtariVox && contrP1 == "ATARIVOX"));
  }

  myP0Label->setEnabled(enableSelectControl);
  myP1Label->setEnabled(enableSelectControl);
  myP0Controller->setEnabled(enableSelectControl);
  myP1Controller->setEnabled(enableSelectControl);

  mySwapPorts->setEnabled(enableSwapPorts);
  mySwapPaddles->setEnabled(enableSwapPaddles);

  myEraseEEPROMLabel->setEnabled(enableEEEraseButton);
  myEraseEEPROMButton->setEnabled(enableEEEraseButton);
  myEraseEEPROMInfo->setEnabled(enableEEEraseButton);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::eraseEEPROM()
{
  Controller& lport = instance().console().leftController();
  Controller& rport = instance().console().rightController();

  if(lport.type() == Controller::SaveKey || lport.type() == Controller::AtariVox)
  {
    SaveKey& skey = static_cast<SaveKey&>(lport);
    skey.eraseCurrent();
  }

  if(rport.type() == Controller::SaveKey || rport.type() == Controller::AtariVox)
  {
    SaveKey& skey = static_cast<SaveKey&>(rport);
    skey.eraseCurrent();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::handleCommand(CommandSender* sender, int cmd,
                                   int data, int id)
{
  switch (cmd)
  {
    case GuiObject::kOKCmd:
      saveConfig();
      close();
      break;

    case GuiObject::kDefaultsCmd:
      setDefaults();
      break;

    case TabWidget::kTabChangedCmd:
      if(data == 2)  // 'Controller' tab selected
        updateControllerStates();

      // The underlying dialog still needs access to this command
      Dialog::handleCommand(sender, cmd, data, 0);
      break;

    case kLeftCChanged:
    case kRightCChanged:
      updateControllerStates();
      break;

    case kEEButtonPressed:
      eraseEEPROM();
      break;

    case kPhosphorChanged:
    {
      bool status = myPhosphor->getState();
      myPPBlend->setEnabled(status);
      break;
    }

    case kYStartChanged:
      if(myYStart->getValue() == TIAConstants::minYStart-1)
      {
        myYStart->setValueLabel("Auto");
        myYStart->setValueUnit("");
      }
      else
        myYStart->setValueUnit("px");

      break;

    case kHeightChanged:
      if(myHeight->getValue() == TIAConstants::minViewableHeight-1)
      {
        myHeight->setValueLabel("Auto");
        myHeight->setValueUnit("");
      }
      else
        myHeight->setValueUnit("px");
      break;

    case kPPBlendChanged:
      if(myPPBlend->getValue() == 0)
      {
        myPPBlend->setValueLabel("Default");
        myPPBlend->setValueUnit("");
      }
      else
        myPPBlend->setValueUnit("%");
      break;

    case kMCtrlChanged:
    {
      //bool state = myMouseControl->getSelectedTag() != "AUTO";
      bool state = myMouseControl->getState();
      myMouseX->setEnabled(state);
      myMouseY->setEnabled(state);
      break;
    }

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
