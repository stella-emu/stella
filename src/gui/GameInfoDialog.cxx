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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Bankswitch.hxx"
#include "Console.hxx"
#include "Cart.hxx"
#include "MouseControl.hxx"
#include "SaveKey.hxx"
#include "Dialog.hxx"
#include "EditTextWidget.hxx"
#include "RadioButtonWidget.hxx"
#include "Launcher.hxx"
#include "OSystem.hxx"
#include "CartDetector.hxx"
#include "ControllerDetector.hxx"
#include "Paddles.hxx"
#include "PopUpWidget.hxx"
#include "Props.hxx"
#include "PropsSet.hxx"
#include "QuadTariDialog.hxx"
#include "TabWidget.hxx"
#include "TIAConstants.hxx"
#include "Widget.hxx"
#include "Font.hxx"

#include "FrameBuffer.hxx"
#include "TIASurface.hxx"
#include "TIA.hxx"
#include "Switches.hxx"
#include "AudioSettings.hxx"

#include "GameInfoDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GameInfoDialog::GameInfoDialog(
      OSystem& osystem, DialogContainer& parent, const GUI::Font& font,
      GuiObject* boss, int max_w, int max_h)
  : Dialog(osystem, parent, font, "Game properties"),
    CommandSender(boss)
{
  const GUI::Font& ifont = instance().frameBuffer().infoFont();

  const int lineHeight = font.getLineHeight(),
            fontHeight = font.getFontHeight(),
            fontWidth = font.getMaxCharWidth(),
            buttonHeight = font.getLineHeight() * 1.25,
            infoLineHeight = ifont.getLineHeight();
  const int VBORDER = fontHeight / 2;
  const int HBORDER = fontWidth * 1.25;
  const int INDENT = fontWidth * 2;
  const int VGAP = fontHeight / 4;

  int xpos, ypos, lwidth, fwidth, pwidth, tabID;
  WidgetArray wid;
  VariantList items, ports, ctrls;
  StaticTextWidget* t;

  // Set real dimensions
  setSize(54 * fontWidth + HBORDER * 2,
          _th + VGAP * 3 + lineHeight + 8 * (lineHeight + VGAP) + 1 * (infoLineHeight + VGAP) +
            ifont.getLineHeight() + VGAP + buttonHeight + VBORDER * 2,
          max_w, max_h);

  // The tab widget
  myTab = new TabWidget(this, font, 2, 4 + _th,
                        _w - 2 * 2,
                        _h - _th - VGAP - buttonHeight - VBORDER * 2);
  addTabWidget(myTab);

  //////////////////////////////////////////////////////////////////////////////
  // 1) Emulation properties
  tabID = myTab->addTab(" Emulation ", TabWidget::AUTO_WIDTH);

  ypos = VBORDER;

  t = new StaticTextWidget(myTab, font, HBORDER, ypos + 1, "Type (*)      ");
  pwidth = font.getStringWidth("CM (SpectraVideo CompuMate)");
  items.clear();
  for(uInt32 i = 0; i < uInt32(Bankswitch::Type::NumSchemes); ++i)
    VarList::push_back(items, Bankswitch::BSList[i].desc, Bankswitch::BSList[i].name);
  myBSType = new PopUpWidget(myTab, font, t->getRight() + fontWidth, ypos,
                           pwidth, lineHeight, items, "");
  wid.push_back(myBSType);
  ypos += lineHeight + VGAP;

  myTypeDetected = new StaticTextWidget(myTab, ifont, t->getRight() + fontWidth, ypos,
                                        "CM (SpectraVideo CompuMate) detected");
  ypos += ifont.getLineHeight() + VGAP;

  // Start bank
  myStartBankLabel = new StaticTextWidget(myTab, font, HBORDER, ypos + 1, "Start bank (*) ");
  items.clear();
  myStartBank = new PopUpWidget(myTab, font, myStartBankLabel->getRight(), ypos,
                                font.getStringWidth("AUTO"), lineHeight, items, "", 0, 0);
  wid.push_back(myStartBank);
  ypos += lineHeight + VGAP * 4;

  pwidth = font.getStringWidth("Auto-detect");
  t = new StaticTextWidget(myTab, font, HBORDER, ypos + 1, "TV format      ");
  items.clear();
  VarList::push_back(items, "Auto-detect", "AUTO");
  VarList::push_back(items, "NTSC", "NTSC");
  VarList::push_back(items, "PAL", "PAL");
  VarList::push_back(items, "SECAM", "SECAM");
  VarList::push_back(items, "NTSC50", "NTSC50");
  VarList::push_back(items, "PAL60", "PAL60");
  VarList::push_back(items, "SECAM60", "SECAM60");
  myFormat = new PopUpWidget(myTab, font, t->getRight(), ypos,
                             pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(myFormat);

  myFormatDetected = new StaticTextWidget(myTab, ifont, myFormat->getRight() + fontWidth, ypos + 4,
                                          "SECAM60 detected");

  // Phosphor
  ypos += lineHeight + VGAP;
  myPhosphor = new CheckboxWidget(myTab, font, HBORDER, ypos + 1,
                                  "Phosphor (enabled for all ROMs)", kPhosphorChanged);
  wid.push_back(myPhosphor);

  ypos += lineHeight + VGAP * 0;
  myPPBlend = new SliderWidget(myTab, font,
                               HBORDER + fontWidth * 2, ypos,
                               "Blend  ", 0, kPPBlendChanged, 4 * fontWidth, "%");
  myPPBlend->setMinValue(0); myPPBlend->setMaxValue(100);
  myPPBlend->setTickmarkIntervals(2);
  wid.push_back(myPPBlend);

  ypos += lineHeight + VGAP;
  t = new StaticTextWidget(myTab, font, HBORDER, ypos + 1, "V-Center ");
  myVCenter = new SliderWidget(myTab, font, t->getRight(), ypos, "",
                               0, kVCenterChanged, 7 * fontWidth, "px", 0, true);

  myVCenter->setMinValue(TIAConstants::minVcenter);
  myVCenter->setMaxValue(TIAConstants::maxVcenter);
  myVCenter->setTickmarkIntervals(4);
  wid.push_back(myVCenter);

  ypos += lineHeight + VGAP * 3;
  mySound = new CheckboxWidget(myTab, font, HBORDER, ypos + 1, "Stereo sound");
  wid.push_back(mySound);

  // Add message concerning usage
  ypos = myTab->getHeight() - fontHeight - ifont.getFontHeight() - VGAP - VBORDER;
  new StaticTextWidget(myTab, ifont, HBORDER, ypos,
                       "(*) Change requires a ROM reload");

  // Add items for tab 0
  addToFocusList(wid, myTab, tabID);

  //////////////////////////////////////////////////////////////////////////////
  // 2) Console properties
  wid.clear();
  tabID = myTab->addTab("  Console  ", TabWidget::AUTO_WIDTH);

  xpos = HBORDER; ypos = VBORDER;
  lwidth = font.getStringWidth(GUI::RIGHT_DIFFICULTY + " ");

  new StaticTextWidget(myTab, font, xpos, ypos + 1, "TV type");
  myTVTypeGroup = new RadioButtonGroup();
  RadioButtonWidget* r = new RadioButtonWidget(myTab, font, xpos + lwidth, ypos + 1,
                            "Color", myTVTypeGroup);
  wid.push_back(r);
  ypos += lineHeight;
  r = new RadioButtonWidget(myTab, font, xpos + lwidth, ypos + 1,
                            "B/W", myTVTypeGroup);
  wid.push_back(r);
  ypos += lineHeight + VGAP * 2;

  new StaticTextWidget(myTab, font, xpos, ypos+1, GUI::LEFT_DIFFICULTY);
  myLeftDiffGroup = new RadioButtonGroup();
  r = new RadioButtonWidget(myTab, font, xpos + lwidth, ypos + 1,
                            "A (Expert)", myLeftDiffGroup);
  wid.push_back(r);
  ypos += lineHeight;
  r = new RadioButtonWidget(myTab, font, xpos + lwidth, ypos + 1,
                            "B (Novice)", myLeftDiffGroup);
  wid.push_back(r);
  ypos += lineHeight + VGAP * 2;

  new StaticTextWidget(myTab, font, xpos, ypos+1, GUI::RIGHT_DIFFICULTY);
  myRightDiffGroup = new RadioButtonGroup();
  r = new RadioButtonWidget(myTab, font, xpos + lwidth, ypos + 1,
                            "A (Expert)", myRightDiffGroup);
  wid.push_back(r);
  ypos += lineHeight;
  r = new RadioButtonWidget(myTab, font, xpos + lwidth, ypos + 1,
                            "B (Novice)", myRightDiffGroup);
  wid.push_back(r);

  // Add items for tab 1
  addToFocusList(wid, myTab, tabID);

  //////////////////////////////////////////////////////////////////////////////
  // 3) Controller properties
  wid.clear();
  tabID = myTab->addTab(" Controllers ", TabWidget::AUTO_WIDTH);

  ctrls.clear();
  VarList::push_back(ctrls, "Auto-detect", "AUTO");
  VarList::push_back(ctrls, "Joystick", "JOYSTICK");
  VarList::push_back(ctrls, "Paddles", "PADDLES");
  VarList::push_back(ctrls, "Paddles_IAxis", "PADDLES_IAXIS");
  VarList::push_back(ctrls, "Paddles_IAxDr", "PADDLES_IAXDR");
  VarList::push_back(ctrls, "BoosterGrip", "BOOSTERGRIP");
  VarList::push_back(ctrls, "Driving", "DRIVING");
  VarList::push_back(ctrls, "Keyboard", "KEYBOARD");
  VarList::push_back(ctrls, "AmigaMouse", "AMIGAMOUSE");
  VarList::push_back(ctrls, "AtariMouse", "ATARIMOUSE");
  VarList::push_back(ctrls, "Trakball", "TRAKBALL");
  VarList::push_back(ctrls, "AtariVox", "ATARIVOX");
  VarList::push_back(ctrls, "SaveKey", "SAVEKEY");
  VarList::push_back(ctrls, "Sega Genesis", "GENESIS");
  VarList::push_back(ctrls, "KidVid", "KIDVID");
  VarList::push_back(ctrls, "Lightgun", "LIGHTGUN");
  VarList::push_back(ctrls, "MindLink", "MINDLINK");
  VarList::push_back(ctrls, "QuadTari", "QUADTARI");

  ypos = VBORDER;
  pwidth = font.getStringWidth("Paddles_IAxis");
  myLeftPortLabel = new StaticTextWidget(myTab, font, HBORDER, ypos+1, "Left port        ");
  myLeftPort = new PopUpWidget(myTab, font, myLeftPortLabel->getRight(),
                               myLeftPortLabel->getTop()-1,
                               pwidth, lineHeight, ctrls, "", 0, kLeftCChanged);
  wid.push_back(myLeftPort);
  ypos += lineHeight + VGAP;

  myLeftPortDetected = new StaticTextWidget(myTab, ifont, myLeftPort->getLeft(), ypos,
                                            "Sega Genesis detected");
  ypos += ifont.getLineHeight() + VGAP;

  myRightPortLabel = new StaticTextWidget(myTab, font, HBORDER, ypos+1, "Right port       ");
  myRightPort = new PopUpWidget(myTab, font, myRightPortLabel->getRight(),
                                myRightPortLabel->getTop()-1,
                                pwidth, lineHeight, ctrls, "", 0, kRightCChanged);
  wid.push_back(myRightPort);

  myQuadTariButton = new ButtonWidget(myTab, font, myRightPort->getRight() + fontWidth * 4, myRightPort->getTop() - 2,
                                    "QuadTari" + ELLIPSIS, kQuadTariPressed);
  wid.push_back(myQuadTariButton);

  ypos += lineHeight + VGAP;
  myRightPortDetected = new StaticTextWidget(myTab, ifont, myRightPort->getLeft(), ypos,
                                             "Sega Genesis detected");
  ypos += ifont.getLineHeight() + VGAP + 4;

  mySwapPorts = new CheckboxWidget(myTab, font, myLeftPort->getRight() + fontWidth * 4,
                                   myLeftPort->getTop() + 1, "Swap ports");
  wid.push_back(mySwapPorts);

  // EEPROM erase button for left/right controller
  //ypos += lineHeight + VGAP + 4;
  pwidth = myRightPort->getWidth();   //font.getStringWidth("Erase EEPROM ") + 23;
  myEraseEEPROMLabel = new StaticTextWidget(myTab, font, HBORDER, ypos, "AtariVox/SaveKey ");
  myEraseEEPROMButton = new ButtonWidget(myTab, font, myEraseEEPROMLabel->getRight(), ypos - 4,
                                         pwidth, buttonHeight, "Erase EEPROM", kEEButtonPressed);
  wid.push_back(myEraseEEPROMButton);
  myEraseEEPROMInfo = new StaticTextWidget(myTab, ifont, myEraseEEPROMButton->getRight() + 4,
                                           myEraseEEPROMLabel->getTop() + 3,
                                           "(for this game only)");
  ypos += lineHeight + VGAP * 4;

  mySwapPaddles = new CheckboxWidget(myTab, font, xpos, ypos, "Swap paddles");
  wid.push_back(mySwapPaddles);
  ypos += lineHeight + VGAP;

  // Paddles
  myPaddlesCenter = new StaticTextWidget(myTab, font, xpos, ypos, "Paddles center:");
  ypos += lineHeight + VGAP;

  xpos += INDENT;
  myPaddleXCenter = new SliderWidget(myTab, font, xpos, ypos - 1, "X ", 0, kPXCenterChanged,
                                     fontWidth * 6, "px", 0 ,true);
  myPaddleXCenter->setMinValue(Paddles::MIN_ANALOG_CENTER);
  myPaddleXCenter->setMaxValue(Paddles::MAX_ANALOG_CENTER);
  myPaddleXCenter->setTickmarkIntervals(4);
  wid.push_back(myPaddleXCenter);
  ypos += lineHeight + VGAP;

  myPaddleYCenter = new SliderWidget(myTab, font, xpos, ypos - 1, "Y ", 0, kPYCenterChanged,
                                     fontWidth * 6, "px", 0 ,true);
  myPaddleYCenter->setMinValue(Paddles::MIN_ANALOG_CENTER);
  myPaddleYCenter->setMaxValue(Paddles::MAX_ANALOG_CENTER);
  myPaddleYCenter->setTickmarkIntervals(4);
  wid.push_back(myPaddleYCenter);

  // Mouse
  xpos = HBORDER + fontWidth * 24 - INDENT;
  ypos = mySwapPaddles->getTop() - 1;
  myMouseControl = new CheckboxWidget(myTab, font, xpos, ypos + 1, "Specific mouse axes",
                                      kMCtrlChanged);
  wid.push_back(myMouseControl);

  // Mouse controller specific axis
  pwidth = font.getStringWidth("MindLink 0");
  items.clear();
  VarList::push_back(items, "None",       static_cast<uInt32>(MouseControl::Type::NoControl));
  VarList::push_back(items, "Paddle 0",   static_cast<uInt32>(MouseControl::Type::Paddle0));
  VarList::push_back(items, "Paddle 1",   static_cast<uInt32>(MouseControl::Type::Paddle1));
  VarList::push_back(items, "Paddle 2",   static_cast<uInt32>(MouseControl::Type::Paddle2));
  VarList::push_back(items, "Paddle 3",   static_cast<uInt32>(MouseControl::Type::Paddle3));
  VarList::push_back(items, "Driving 0",  static_cast<uInt32>(MouseControl::Type::Driving0));
  VarList::push_back(items, "Driving 1",  static_cast<uInt32>(MouseControl::Type::Driving1));
  VarList::push_back(items, "MindLink 0", static_cast<uInt32>(MouseControl::Type::MindLink0));
  VarList::push_back(items, "MindLink 1", static_cast<uInt32>(MouseControl::Type::MindLink1));

  xpos += CheckboxWidget::prefixSize(font);
  ypos += lineHeight + VGAP;
  myMouseX = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight, items,
               "X-Axis is ");
  wid.push_back(myMouseX);

  ypos += lineHeight + VGAP;
  myMouseY = new PopUpWidget(myTab, font, myMouseX->getLeft(), ypos, pwidth, lineHeight, items,
               "Y-Axis is ");
  wid.push_back(myMouseY);

  xpos -= CheckboxWidget::prefixSize(font); ypos += lineHeight + VGAP;
  myMouseRange = new SliderWidget(myTab, font, xpos, ypos,
                                  "Mouse axes range ", 0, 0, fontWidth * 4, "%");
  myMouseRange->setMinValue(1); myMouseRange->setMaxValue(100);
  myMouseRange->setTickmarkIntervals(4);
  wid.push_back(myMouseRange);

  // Add items for tab 2
  addToFocusList(wid, myTab, tabID);

  //////////////////////////////////////////////////////////////////////////////
  // 4) Cartridge properties
  wid.clear();
  tabID = myTab->addTab(" Cartridge ", TabWidget::AUTO_WIDTH);

  xpos = HBORDER; ypos = VBORDER;
  lwidth = font.getStringWidth("Manufacturer ");
  fwidth = _w - lwidth - HBORDER * 2 - 2;
  new StaticTextWidget(myTab, font, xpos, ypos + 1, lwidth, fontHeight, "Name");
  myName = new EditTextWidget(myTab, font, xpos + lwidth, ypos - 1,
                              fwidth, lineHeight, "");
  wid.push_back(myName);

  ypos += lineHeight + VGAP;
  new StaticTextWidget(myTab, font, xpos, ypos + 1, lwidth, fontHeight, "MD5");
  myMD5 = new EditTextWidget(myTab, font, xpos + lwidth, ypos - 1,
                             fwidth, lineHeight, "");
  myMD5->setEditable(false);

  ypos += lineHeight + VGAP;
  new StaticTextWidget(myTab, font, xpos, ypos + 1, lwidth, fontHeight, "Manufacturer");
  myManufacturer = new EditTextWidget(myTab, font, xpos + lwidth, ypos - 1,
                                      fwidth, lineHeight, "");
  wid.push_back(myManufacturer);

  ypos += lineHeight + VGAP;
  new StaticTextWidget(myTab, font, xpos, ypos + 1, lwidth, fontHeight,
                       "Model", TextAlign::Left);
  myModelNo = new EditTextWidget(myTab, font, xpos + lwidth, ypos - 1,
                                 fwidth, lineHeight, "");
  wid.push_back(myModelNo);

  ypos += lineHeight + VGAP;
  new StaticTextWidget(myTab, font, xpos, ypos + 1, lwidth, fontHeight, "Rarity");
  myRarity = new EditTextWidget(myTab, font, xpos + lwidth, ypos - 1,
                                fwidth, lineHeight, "");
  wid.push_back(myRarity);

  ypos += lineHeight + VGAP;
  new StaticTextWidget(myTab, font, xpos, ypos + 1, lwidth, fontHeight, "Note");
  myNote = new EditTextWidget(myTab, font, xpos + lwidth, ypos - 1,
                              fwidth, lineHeight, "");
  wid.push_back(myNote);

  // Add items for tab 3
  addToFocusList(wid, myTab, tabID);

  // Activate the first tab
  myTab->setActiveTab(0);

  // Add Defaults, OK and Cancel buttons
  wid.clear();
  addDefaultsExtraOKCancelBGroup(wid, font, "Save", kSavePressed);
  addBGroupToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadConfig()
{
  if(instance().hasConsole())
  {
    myGameProperties = instance().console().properties();
    myGameFile = instance().romFile();
  }
  else
  {
    const string& md5 = instance().launcher().selectedRomMD5();
    instance().propSet().getMD5(md5, myGameProperties);
    myGameFile = FilesystemNode(instance().launcher().selectedRom());
  }

  loadEmulationProperties(myGameProperties);
  loadConsoleProperties(myGameProperties);
  loadControllerProperties(myGameProperties);
  loadCartridgeProperties(myGameProperties);

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadEmulationProperties(const Properties& props)
{
  string bsDetected = "";

  myBSType->setSelected(props.get(PropType::Cart_Type), "AUTO");
  if(myBSType->getSelectedTag().toString() == "AUTO")
  {
    if(instance().hasConsole())
    {
      string bs = instance().console().about().BankSwitch;
      size_t pos = bs.find_first_of('*');
      // remove '*':
      if(pos != string::npos)
        bs = bs.substr(0, pos) + bs.substr(pos + 1);
      bsDetected = bs + "detected";
    }
    else
    {
      const FilesystemNode& node = FilesystemNode(instance().launcher().selectedRom());
      ByteBuffer image;
      string md5 = props.get(PropType::Cart_MD5);
      size_t size = 0;

      // try to load the image for auto detection
      if(!instance().hasConsole() &&
        node.exists() && !node.isDirectory() && (image = instance().openROM(node, md5, size)) != nullptr)
      {
        bsDetected = Bankswitch::typeToDesc(CartDetector::autodetectType(image, size)) + " detected";
      }
    }
  }
  myTypeDetected->setLabel(bsDetected);

  // Start bank
  VariantList items;

  VarList::push_back(items, "Auto", "AUTO");
  if(instance().hasConsole())
  {
    uInt16 numBanks = instance().console().cartridge().romBankCount();

    for(uInt16 i = 0; i < numBanks; ++i)
      VarList::push_back(items, i, i);
    myStartBank->setEnabled(true);
  }
  else
  {
    const string& startBank = props.get(PropType::Cart_StartBank);

    VarList::push_back(items, startBank, startBank);
    myStartBank->setEnabled(false);
  }
  myStartBank->addItems(items);
  myStartBank->setSelected(props.get(PropType::Cart_StartBank), "AUTO");

  myFormat->setSelected(props.get(PropType::Display_Format), "AUTO");
  if(instance().hasConsole() && myFormat->getSelectedTag().toString() == "AUTO")
  {
    const string& format = instance().console().about().DisplayFormat;
    string label;
    if (format.at(format.length() - 1) == '*')
      label = format.substr(0, format.length() - 1);
    else
      label = format;
    myFormatDetected->setLabel(label + " detected");
  }
  else
    myFormatDetected->setLabel("");

  // if phosphor is always enabled, disable game specific phosphor settings
  bool alwaysPhosphor = instance().settings().getString("tv.phosphor") == "always";
  bool usePhosphor = props.get(PropType::Display_Phosphor) == "YES";
  myPhosphor->setState(usePhosphor);
  myPhosphor->setEnabled(!alwaysPhosphor);
  if (alwaysPhosphor)
    myPhosphor->setLabel("Phosphor (enabled for all ROMs)");
  else
    myPhosphor->setLabel("Phosphor");
  myPPBlend->setEnabled(!alwaysPhosphor && usePhosphor);

  const string& blend = props.get(PropType::Display_PPBlend);
  myPPBlend->setValue(BSPF::stringToInt(blend));

  // set vertical center
  Int32 vcenter = BSPF::stringToInt(props.get(PropType::Display_VCenter));
  myVCenter->setValueLabel(vcenter);
  myVCenter->setValue(vcenter);
  myVCenter->setValueUnit(vcenter ? "px" : "");

  mySound->setState(props.get(PropType::Cart_Sound) == "STEREO");
  // if stereo is always enabled, disable game specific stereo setting
  mySound->setEnabled(!instance().audioSettings().stereo());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadConsoleProperties(const Properties& props)
{
  myLeftDiffGroup->setSelected(props.get(PropType::Console_LeftDiff) == "A" ? 0 : 1);
  myRightDiffGroup->setSelected(props.get(PropType::Console_RightDiff) == "A" ? 0 : 1);
  myTVTypeGroup->setSelected(props.get(PropType::Console_TVType) == "BW" ? 1 : 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadControllerProperties(const Properties& props)
{
  string controller = props.get(PropType::Controller_Left);
  myLeftPort->setSelected(controller, "AUTO");
  controller = props.get(PropType::Controller_Right);
  myRightPort->setSelected(controller, "AUTO");

  mySwapPorts->setState(props.get(PropType::Console_SwapPorts) == "YES");
  mySwapPaddles->setState(props.get(PropType::Controller_SwapPaddles) == "YES");

  // Paddle centers
  myPaddleXCenter->setValue(BSPF::stringToInt(props.get(PropType::Controller_PaddlesXCenter)));
  myPaddleYCenter->setValue(BSPF::stringToInt(props.get(PropType::Controller_PaddlesYCenter)));

  // MouseAxis property (potentially contains 'range' information)
  istringstream m_axis(props.get(PropType::Controller_MouseAxis));
  string m_control, m_range;
  m_axis >> m_control;
  bool autoAxis = BSPF::equalsIgnoreCase(m_control, "AUTO");
  myMouseControl->setState(!autoAxis);
  if(autoAxis)
  {
    myMouseX->setSelectedIndex(0);
    myMouseY->setSelectedIndex(0);
  }
  else
  {
    myMouseX->setSelected(m_control[0] - '0');
    myMouseY->setSelected(m_control[1] - '0');
  }
  myMouseX->setEnabled(!autoAxis);
  myMouseY->setEnabled(!autoAxis);
  if(m_axis >> m_range)
  {
    myMouseRange->setValue(BSPF::stringToInt(m_range));
  }
  else
  {
    myMouseRange->setValue(100);
  }

  updateControllerStates();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadCartridgeProperties(const Properties& props)
{
  myName->setText(props.get(PropType::Cart_Name));
  myMD5->setText(props.get(PropType::Cart_MD5));
  myManufacturer->setText(props.get(PropType::Cart_Manufacturer));
  myModelNo->setText(props.get(PropType::Cart_ModelNo));
  myRarity->setText(props.get(PropType::Cart_Rarity));
  myNote->setText(props.get(PropType::Cart_Note));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::saveProperties()
{
  // Emulation properties
  myGameProperties.set(PropType::Cart_Type, myBSType->getSelectedTag().toString());
  myGameProperties.set(PropType::Cart_StartBank, myStartBank->getSelectedTag().toString());
  myGameProperties.set(PropType::Display_Format, myFormat->getSelectedTag().toString());
  myGameProperties.set(PropType::Display_Phosphor, myPhosphor->getState() ? "YES" : "NO");
  myGameProperties.set(PropType::Display_PPBlend, myPPBlend->getValueLabel() == "Off" ? "0" :
                       myPPBlend->getValueLabel());
  Int32 vcenter = myVCenter->getValue();

  myGameProperties.set(PropType::Display_VCenter, std::to_string(vcenter));
  myGameProperties.set(PropType::Cart_Sound, mySound->getState() ? "STEREO" : "MONO");

  // Console properties
  myGameProperties.set(PropType::Console_TVType, myTVTypeGroup->getSelected() ? "BW" : "COLOR");
  myGameProperties.set(PropType::Console_LeftDiff, myLeftDiffGroup->getSelected() ? "B" : "A");
  myGameProperties.set(PropType::Console_RightDiff, myRightDiffGroup->getSelected() ? "B" : "A");

  // Controller properties
  string controller = myLeftPort->getSelectedTag().toString();
  myGameProperties.set(PropType::Controller_Left, controller);
  if(controller != "AUTO" && controller != "QUADTARI")
  {
    myGameProperties.set(PropType::Controller_Left1, "");
    myGameProperties.set(PropType::Controller_Left2, "");
  }

  controller = myRightPort->getSelectedTag().toString();
  myGameProperties.set(PropType::Controller_Right, controller);
  if(controller != "AUTO" && controller != "QUADTARI")
  {
    myGameProperties.set(PropType::Controller_Right1, "");
    myGameProperties.set(PropType::Controller_Right2, "");
  }

  myGameProperties.set(PropType::Console_SwapPorts, (mySwapPorts->isEnabled() && mySwapPorts->getState()) ? "YES" : "NO");
  myGameProperties.set(PropType::Controller_SwapPaddles, mySwapPaddles->getState() ? "YES" : "NO");

  // Paddle center
  myGameProperties.set(PropType::Controller_PaddlesXCenter, std::to_string(myPaddleXCenter->getValue()));
  myGameProperties.set(PropType::Controller_PaddlesYCenter, std::to_string(myPaddleYCenter->getValue()));

  // MouseAxis property (potentially contains 'range' information)
  string mcontrol = "AUTO";
  if(myMouseControl->getState())
    mcontrol = myMouseX->getSelectedTag().toString() +
               myMouseY->getSelectedTag().toString();
  string range = myMouseRange->getValueLabel();
  if(range != "100")
    mcontrol += " " + range;
  myGameProperties.set(PropType::Controller_MouseAxis, mcontrol);

  // Cartridge properties
  myGameProperties.set(PropType::Cart_Name, myName->getText());
  myGameProperties.set(PropType::Cart_Manufacturer, myManufacturer->getText());
  myGameProperties.set(PropType::Cart_ModelNo, myModelNo->getText());
  myGameProperties.set(PropType::Cart_Rarity, myRarity->getText());
  myGameProperties.set(PropType::Cart_Note, myNote->getText());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::saveConfig()
{
  saveProperties();

  // Always insert; if the properties are already present, nothing will happen
  instance().propSet().insert(myGameProperties);
  instance().saveConfig();

  // In any event, inform the Console
  if(instance().hasConsole())
  {
    instance().console().setProperties(myGameProperties);

    // update 'Emulation' tab settings immediately
    instance().console().setFormat(myFormat->getSelected());
    instance().frameBuffer().tiaSurface().enablePhosphor(myPhosphor->getState(), myPPBlend->getValue());
    instance().console().updateVcenter(myVCenter->getValue());
    instance().console().initializeAudio();

    // update 'Console' tab settings immediately
    instance().console().switches().setTvColor(myTVTypeGroup->getSelected() == 0);
    instance().console().switches().setLeftDifficultyA(myLeftDiffGroup->getSelected() == 0);
    instance().console().switches().setRightDifficultyA(myRightDiffGroup->getSelected() == 0);

    // update 'Controllers' tab settings immediately
    instance().console().setControllers(myGameProperties.get(PropType::Cart_MD5));

    Paddles::setAnalogXCenter(myPaddleXCenter->getValue());
    Paddles::setAnalogYCenter(myPaddleYCenter->getValue());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::setDefaults()
{
  // Load the default properties
  Properties defaultProperties;
  const string& md5 = myGameProperties.get(PropType::Cart_MD5);

  instance().propSet().getMD5(md5, defaultProperties, true);

  switch(myTab->getActiveTab())
  {
    case 0: // Emulation properties
      loadEmulationProperties(defaultProperties);
      break;

    case 1: // Console properties
      loadConsoleProperties(defaultProperties);
      break;

    case 2: // Controller properties
      loadControllerProperties(defaultProperties);
      break;

    case 3: // Cartridge properties
      loadCartridgeProperties(defaultProperties);
      break;

    default: // make the compiler happy
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::updateControllerStates()
{
  bool swapPorts = mySwapPorts->getState();
  bool autoDetect = false;
  ByteBuffer image;
  string md5 = myGameProperties.get(PropType::Cart_MD5);
  size_t size = 0;

  // try to load the image for auto detection
  if(!instance().hasConsole())
  {
    const FilesystemNode& node = FilesystemNode(instance().launcher().selectedRom());

    autoDetect = node.exists() && !node.isDirectory() && (image = instance().openROM(node, md5, size)) != nullptr;
  }
  string label = "";
  Controller::Type type = Controller::getType(myLeftPort->getSelectedTag().toString());

  if(type == Controller::Type::Unknown)
  {
    if(instance().hasConsole())
    {
      label = (!swapPorts ? instance().console().leftController().name()
               : instance().console().rightController().name() + " detected");
      if(BSPF::startsWithIgnoreCase(label, "QUADTARI"))
        label = "QuadTari detected"; // remove plugged-in controller names
    }
    else if(autoDetect)
      label = ControllerDetector::detectName(image, size, type,
                                             !swapPorts ? Controller::Jack::Left : Controller::Jack::Right,
                                             instance().settings()) + " detected";
  }
  myLeftPortDetected->setLabel(label);

  label = "";
  type = Controller::getType(myRightPort->getSelectedTag().toString());

  if(type == Controller::Type::Unknown)
  {
    if(instance().hasConsole())
    {
      label = (!swapPorts ? instance().console().rightController().name()
               : instance().console().leftController().name()) + " detected";
      if(BSPF::startsWithIgnoreCase(label, "QUADTARI"))
        label = "QuadTari detected"; // remove plugged-in controller names
    }
    else if(autoDetect)
      label = ControllerDetector::detectName(image, size, type,
                                             !swapPorts ? Controller::Jack::Right : Controller::Jack::Left,
                                             instance().settings()) + " detected";
  }
  myRightPortDetected->setLabel(label);

  const string& contrLeft = myLeftPort->getSelectedTag().toString();
  const string& contrRight = myRightPort->getSelectedTag().toString();
  bool enableEEEraseButton = false;

  // Compumate bankswitching scheme doesn't allow to select controllers
  bool enableSelectControl = myBSType->getSelectedTag() != "CM";
  // Enable Swap Paddles checkbox only for paddle games
  bool enablePaddles = BSPF::startsWithIgnoreCase(contrLeft, "PADDLES") ||
    BSPF::startsWithIgnoreCase(contrRight, "PADDLES") ||
    BSPF::startsWithIgnoreCase(myLeftPortDetected->getLabel(), "Paddles") ||
    BSPF::startsWithIgnoreCase(myRightPortDetected->getLabel(), "Paddles");

  if(instance().hasConsole())
  {
    const Controller& lport = instance().console().leftController();
    const Controller& rport = instance().console().rightController();

    // we only enable the button if we have a valid previous and new controller.
    bool enableBtnForLeft =
      (contrLeft == "AUTO" || contrLeft == "SAVEKEY" || contrLeft == "ATARIVOX") &&
      (lport.type() == Controller::Type::SaveKey || lport.type() == Controller::Type::AtariVox);
    bool enableBtnForRight =
      (contrRight == "AUTO" || contrRight == "SAVEKEY" || contrRight == "ATARIVOX") &&
      (rport.type() == Controller::Type::SaveKey || rport.type() == Controller::Type::AtariVox);
    enableEEEraseButton = enableBtnForLeft || enableBtnForRight;
  }

  myLeftPortLabel->setEnabled(enableSelectControl);
  myRightPortLabel->setEnabled(enableSelectControl);
  myLeftPort->setEnabled(enableSelectControl);
  myRightPort->setEnabled(enableSelectControl);
  myQuadTariButton->setEnabled(BSPF::startsWithIgnoreCase(contrLeft, "QUADTARI") ||
                               BSPF::startsWithIgnoreCase(contrRight, "QUADTARI") ||
                               BSPF::startsWithIgnoreCase(myLeftPortDetected->getLabel(), "QUADTARI") ||
                               BSPF::startsWithIgnoreCase(myRightPortDetected->getLabel(), "QUADTARI"));

  mySwapPorts->setEnabled(enableSelectControl);
  mySwapPaddles->setEnabled(enablePaddles);

  myEraseEEPROMLabel->setEnabled(enableEEEraseButton);
  myEraseEEPROMButton->setEnabled(enableEEEraseButton);
  myEraseEEPROMInfo->setEnabled(enableEEEraseButton);

  myPaddlesCenter->setEnabled(enablePaddles);
  myPaddleXCenter->setEnabled(enablePaddles);
  myPaddleYCenter->setEnabled(enablePaddles);

  bool enableMouse = enablePaddles ||
    BSPF::startsWithIgnoreCase(contrLeft, "Driving") ||
    BSPF::startsWithIgnoreCase(contrRight, "Driving") ||
    BSPF::startsWithIgnoreCase(contrLeft, "MindLink") ||
    BSPF::startsWithIgnoreCase(contrRight, "MindLink");

  myMouseControl->setEnabled(enableMouse);
  myMouseX->setEnabled(enableMouse);
  myMouseY->setEnabled(enableMouse);
  myMouseRange->setEnabled(enableMouse);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::eraseEEPROM()
{
  Controller& lport = instance().console().leftController();
  Controller& rport = instance().console().rightController();

  if(lport.type() == Controller::Type::SaveKey ||
     lport.type() == Controller::Type::AtariVox)
  {
    SaveKey& skey = static_cast<SaveKey&>(lport);
    skey.eraseCurrent();
  }

  if(rport.type() == Controller::Type::SaveKey ||
     rport.type() == Controller::Type::AtariVox)
  {
    SaveKey& skey = static_cast<SaveKey&>(rport);
    skey.eraseCurrent();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::saveCurrentPropertiesToDisk()
{
  saveProperties();
  stringstream out;
  out << myGameProperties;

  try
  {
    FilesystemNode propfile = instance().defaultSaveDir();
    propfile /= myGameFile.getNameWithExt(".pro");

    propfile.write(out);
    instance().frameBuffer().showMessage("Properties saved to " +
                                         propfile.getShortPath());
  }
  catch(...)
  {
    instance().frameBuffer().showMessage("Error saving properties");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::handleCommand(CommandSender* sender, int cmd,
                                   int data, int id)
{
  switch(cmd)
  {
    case GuiObject::kOKCmd:
      saveConfig();
      close();
      break;

    case GuiObject::kDefaultsCmd:
      setDefaults();
      break;

    case kSavePressed:
      saveCurrentPropertiesToDisk();
      break;

    case TabWidget::kTabChangedCmd:
      if(data == 2)  // 'Controllers' tab selected
        updateControllerStates();

      // The underlying dialog still needs access to this command
      Dialog::handleCommand(sender, cmd, data, 0);
      break;

    case kLeftCChanged:
    case kRightCChanged:
      updateControllerStates();
      break;

    case kQuadTariPressed:
    {
      bool enableLeft =
        BSPF::startsWithIgnoreCase(myLeftPort->getSelectedTag().toString(), "QUADTARI") ||
        BSPF::startsWithIgnoreCase(myLeftPortDetected->getLabel(), "QUADTARI");
      bool enableRight =
        BSPF::startsWithIgnoreCase(myRightPort->getSelectedTag().toString(), "QUADTARI") ||
        BSPF::startsWithIgnoreCase(myRightPortDetected->getLabel(), "QUADTARI");

      if(!myQuadTariDialog)
        myQuadTariDialog = make_unique<QuadTariDialog>
          (this, _font, _font.getMaxCharWidth() * 37, _font.getFontHeight() * 8,
           myGameProperties);
      myQuadTariDialog->show(enableLeft, enableRight);
      break;
    }
    case kEEButtonPressed:
      eraseEEPROM();
      break;

    case kPhosphorChanged:
    {
      bool status = myPhosphor->getState();
      myPPBlend->setEnabled(status);
      break;
    }

    case kPPBlendChanged:
      if(myPPBlend->getValue() == 0)
      {
        myPPBlend->setValueLabel("Off");
        myPPBlend->setValueUnit("");
      }
      else
        myPPBlend->setValueUnit("%");
      break;

    case kVCenterChanged:
      if (myVCenter->getValue() == 0)
      {
        myVCenter->setValueLabel("Default");
        myVCenter->setValueUnit("");
      }
      else
        myVCenter->setValueUnit("px");
      break;

    case kPXCenterChanged:
      myPaddleXCenter->setValueLabel(myPaddleXCenter->getValue() * 5);
      break;

    case kPYCenterChanged:
      myPaddleYCenter->setValueLabel(myPaddleYCenter->getValue() * 5);
      break;

    case kMCtrlChanged:
    {
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
