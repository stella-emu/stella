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
#include "EditTextWidget.hxx"
#include "RadioButtonWidget.hxx"
#include "Launcher.hxx"
#include "OSystem.hxx"
#include "CartDetector.hxx"
#include "ControllerDetector.hxx"
#include "PopUpWidget.hxx"
#include "PropsSet.hxx"
#include "TabWidget.hxx"
#include "TIAConstants.hxx"
#include "Widget.hxx"
#include "Font.hxx"

#include "FrameBuffer.hxx"
#include "TIASurface.hxx"
#include "TIA.hxx"
#include "Switches.hxx"
#include "AudioSettings.hxx"
#include "bspf.hxx"

#include "GameInfoDialog.hxx"

using namespace std;
using namespace BSPF;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GameInfoDialog::GameInfoDialog(
      OSystem& osystem, DialogContainer& parent, const GUI::Font& font,
      GuiObject* boss, int max_w, int max_h)
  : Dialog(osystem, parent, font, "Game properties"),
    CommandSender(boss)
{
  const int VBORDER = 8;
  const int HBORDER = 10;
  const int VGAP = 4;
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int lineHeight   = _font.getLineHeight(),
            fontWidth    = _font.getMaxCharWidth(),
            fontHeight   = _font.getFontHeight(),
            infoLineHeight = ifont.getLineHeight();

  WidgetArray wid;

  // Set real dimensions
  setSize(HBORDER * 2 + 53 * fontWidth,
          8 * (lineHeight + VGAP) + 1 * (infoLineHeight + VGAP) + VBORDER * 2 + _th +
          buttonHeight(font) + fontHeight + ifont.getLineHeight() + 20,
          max_w, max_h);

  // The tab widget
  myTab = new TabWidget(this, font, 2, 4 + _th, _w - 2 * 2,
                        _h - (_th + buttonHeight(font) + 20));
  addTabWidget(myTab);

  //////////////////////////////////////////////////////////////////////////////
  addEmulationTab();
  addConsoleTab();
  addControllersTab();
  addCartridgeTab();
  addHighScoresTab();
  //////////////////////////////////////////////////////////////////////////////

  // Activate the first tab
  myTab->setActiveTab(0);

  // Add Defaults, OK and Cancel buttons
  addDefaultsOKCancelBGroup(wid, font);
  addBGroupToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::addEmulationTab()
{
  const int VBORDER = 8;
  const int HBORDER = 10;
  const int VGAP = 4;
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int lineHeight   = _font.getLineHeight(),
            fontWidth    = _font.getMaxCharWidth(),
            fontHeight   = _font.getFontHeight();

  int ypos, pwidth, tabID;
  WidgetArray wid;
  VariantList items;
  StaticTextWidget* t;

  // 1) Emulation properties
  tabID = myTab->addTab("Emulation", TabWidget::AUTO_WIDTH);

  ypos = VBORDER;

  t = new StaticTextWidget(myTab, _font, HBORDER, ypos + 1, "Type (*)      ");
  pwidth = _font.getStringWidth("CM (SpectraVideo CompuMate)");
  items.clear();
  for(uInt32 i = 0; i < uInt32(Bankswitch::Type::NumSchemes); ++i)
    VarList::push_back(items, Bankswitch::BSList[i].desc, Bankswitch::BSList[i].name);
  myBSType = new PopUpWidget(myTab, _font, t->getRight() + 8, ypos,
                             pwidth, lineHeight, items, "");
  wid.push_back(myBSType);
  ypos += lineHeight + VGAP;

  myTypeDetected = new StaticTextWidget(myTab, ifont, t->getRight() + 8, ypos,
                                        "CM (SpectraVideo CompuMate) detected");
  ypos += ifont.getLineHeight() + VGAP;

  // Start bank
  myStartBankLabel = new StaticTextWidget(myTab, _font, HBORDER, ypos + 1, "Start bank (*) ");
  items.clear();
  myStartBank = new PopUpWidget(myTab, _font, myStartBankLabel->getRight(), ypos,
                                _font.getStringWidth("AUTO"), lineHeight, items, "", 0, 0);
  wid.push_back(myStartBank);
  ypos += lineHeight + VGAP * 4;

  pwidth = _font.getStringWidth("Auto-detect");
  t = new StaticTextWidget(myTab, _font, HBORDER, ypos + 1, "TV format      ");
  items.clear();
  VarList::push_back(items, "Auto-detect", "AUTO");
  VarList::push_back(items, "NTSC", "NTSC");
  VarList::push_back(items, "PAL", "PAL");
  VarList::push_back(items, "SECAM", "SECAM");
  VarList::push_back(items, "NTSC50", "NTSC50");
  VarList::push_back(items, "PAL60", "PAL60");
  VarList::push_back(items, "SECAM60", "SECAM60");
  myFormat = new PopUpWidget(myTab, _font, t->getRight(), ypos,
                             pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(myFormat);

  myFormatDetected = new StaticTextWidget(myTab, ifont, myFormat->getRight() + 8, ypos + 4, "SECAM60 detected");

  // Phosphor
  ypos += lineHeight + VGAP;
  myPhosphor = new CheckboxWidget(myTab, _font, HBORDER, ypos + 1, "Phosphor (enabled for all ROMs)", kPhosphorChanged);
  wid.push_back(myPhosphor);

  ypos += lineHeight + VGAP * 0;
  myPPBlend = new SliderWidget(myTab, _font,
                               HBORDER + 20, ypos,
                               "Blend  ", 0, kPPBlendChanged, 4 * fontWidth, "%");
  myPPBlend->setMinValue(0); myPPBlend->setMaxValue(100);
  myPPBlend->setTickmarkIntervals(2);
  wid.push_back(myPPBlend);

  ypos += lineHeight + VGAP;
  t = new StaticTextWidget(myTab, _font, HBORDER, ypos + 1, "V-Center ");
  myVCenter = new SliderWidget(myTab, _font, t->getRight() + 2, ypos, "",
                               0, kVCenterChanged, 7 * fontWidth, "px", 0, true);

  myVCenter->setMinValue(TIAConstants::minVcenter);
  myVCenter->setMaxValue(TIAConstants::maxVcenter);
  myVCenter->setTickmarkIntervals(4);
  wid.push_back(myVCenter);

  ypos += lineHeight + VGAP * 3;
  mySound = new CheckboxWidget(myTab, _font, HBORDER, ypos + 1, "Stereo sound");
  wid.push_back(mySound);

  // Add message concerning usage
  ypos = myTab->getHeight() - 5 - fontHeight - ifont.getFontHeight() - 10;
  new StaticTextWidget(myTab, ifont, HBORDER, ypos,
                       "(*) Changes require a ROM reload");

  // Add items for tab 0
  addToFocusList(wid, myTab, tabID);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::addConsoleTab()
{
  // 2) Console properties
  const int VBORDER = 8;
  const int HBORDER = 10;
  const int VGAP = 4;
  const int lineHeight = _font.getLineHeight(),
            fontHeight = _font.getFontHeight();

  int xpos, ypos, lwidth, tabID;
  WidgetArray wid;
  VariantList items;

  tabID = myTab->addTab(" Console ", TabWidget::AUTO_WIDTH);

  xpos = HBORDER; ypos = VBORDER;
  lwidth = _font.getStringWidth(GUI::RIGHT_DIFFICULTY + " ");

  new StaticTextWidget(myTab, _font, xpos, ypos + 1, "TV type");
  myTVTypeGroup = new RadioButtonGroup();
  RadioButtonWidget* r = new RadioButtonWidget(myTab, _font, xpos + lwidth, ypos + 1,
                                               "Color", myTVTypeGroup);
  wid.push_back(r);
  ypos += lineHeight;
  r = new RadioButtonWidget(myTab, _font, xpos + lwidth, ypos + 1,
                            "B/W", myTVTypeGroup);
  wid.push_back(r);
  ypos += lineHeight + VGAP * 2;

  new StaticTextWidget(myTab, _font, xpos, ypos+1, GUI::LEFT_DIFFICULTY);
  myLeftDiffGroup = new RadioButtonGroup();
  r = new RadioButtonWidget(myTab, _font, xpos + lwidth, ypos + 1,
                            "A (Expert)", myLeftDiffGroup);
  wid.push_back(r);
  ypos += lineHeight;
  r = new RadioButtonWidget(myTab, _font, xpos + lwidth, ypos + 1,
                            "B (Novice)", myLeftDiffGroup);
  wid.push_back(r);
  ypos += lineHeight + VGAP * 2;

  new StaticTextWidget(myTab, _font, xpos, ypos+1, GUI::RIGHT_DIFFICULTY);
  myRightDiffGroup = new RadioButtonGroup();
  r = new RadioButtonWidget(myTab, _font, xpos + lwidth, ypos + 1,
                            "A (Expert)", myRightDiffGroup);
  wid.push_back(r);
  ypos += lineHeight;
  r = new RadioButtonWidget(myTab, _font, xpos + lwidth, ypos + 1,
                            "B (Novice)", myRightDiffGroup);
  wid.push_back(r);

  // Add items for tab 1
  addToFocusList(wid, myTab, tabID);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::addControllersTab()
{
  // 3) Controller properties
  const int VBORDER = 8;
  const int HBORDER = 10;
  const int VGAP = 4;
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int lineHeight = _font.getLineHeight(),
    fontWidth = _font.getMaxCharWidth(),
    fontHeight = _font.getFontHeight();

  int xpos, ypos, pwidth, tabID;
  WidgetArray wid;
  VariantList items, ports, ctrls;;

  tabID = myTab->addTab("Controllers", TabWidget::AUTO_WIDTH);

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

  xpos = HBORDER; ypos = VBORDER;
  pwidth = _font.getStringWidth("Paddles_IAxis");
  myLeftPortLabel = new StaticTextWidget(myTab, _font, HBORDER, ypos+1, "Left port        ");
  myLeftPort = new PopUpWidget(myTab, _font, myLeftPortLabel->getRight(),
                               myLeftPortLabel->getTop()-1,
                               pwidth, lineHeight, ctrls, "", 0, kLeftCChanged);
  wid.push_back(myLeftPort);
  ypos += lineHeight + VGAP;

  myLeftPortDetected = new StaticTextWidget(myTab, ifont, myLeftPort->getLeft(), ypos,
                                            "Sega Genesis detected");
  ypos += ifont.getLineHeight() + VGAP;

  myRightPortLabel = new StaticTextWidget(myTab, _font, HBORDER, ypos+1, "Right port       ");
  myRightPort = new PopUpWidget(myTab, _font, myRightPortLabel->getRight(),
                                myRightPortLabel->getTop()-1,
                                pwidth, lineHeight, ctrls, "", 0, kRightCChanged);
  wid.push_back(myRightPort);
  ypos += lineHeight + VGAP;
  myRightPortDetected = new StaticTextWidget(myTab, ifont, myRightPort->getLeft(), ypos,
                                             "Sega Genesis detected");
  ypos += ifont.getLineHeight() + VGAP + 4;

  mySwapPorts = new CheckboxWidget(myTab, _font, myLeftPort->getRight() + fontWidth*4,
                                   myLeftPort->getTop()+1, "Swap ports");
  wid.push_back(mySwapPorts);
  mySwapPaddles = new CheckboxWidget(myTab, _font, myRightPort->getRight() + fontWidth*4,
                                     myRightPort->getTop()+1, "Swap paddles");
  wid.push_back(mySwapPaddles);

  // EEPROM erase button for left/right controller
  //ypos += lineHeight + VGAP + 4;
  pwidth = myRightPort->getWidth();   //_font.getStringWidth("Erase EEPROM ") + 23;
  myEraseEEPROMLabel = new StaticTextWidget(myTab, _font, HBORDER, ypos, "AtariVox/SaveKey ");
  myEraseEEPROMButton = new ButtonWidget(myTab, _font, myEraseEEPROMLabel->getRight(), ypos - 4,
                                         pwidth, buttonHeight(_font), "Erase EEPROM",
                                         kEEButtonPressed);
  wid.push_back(myEraseEEPROMButton);
  myEraseEEPROMInfo = new StaticTextWidget(myTab, ifont, myEraseEEPROMButton->getRight() + 4,
                                           myEraseEEPROMLabel->getTop() + 3,
                                           "(for this game only)");

  ypos += lineHeight + VGAP * 4;
  myMouseControl = new CheckboxWidget(myTab, _font, xpos, ypos + 1, "Specific mouse axes",
                                      kMCtrlChanged);
  wid.push_back(myMouseControl);

  // Mouse controller specific axis
  pwidth = _font.getStringWidth("MindLink 0");
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

  xpos += 20;
  ypos += lineHeight + VGAP;
  myMouseX = new PopUpWidget(myTab, _font, xpos, ypos, pwidth, lineHeight, items,
                             "X-Axis is ");
  wid.push_back(myMouseX);

  ypos += lineHeight + VGAP;
  myMouseY = new PopUpWidget(myTab, _font, myMouseX->getLeft(), ypos, pwidth, lineHeight, items,
                             "Y-Axis is ");
  wid.push_back(myMouseY);

  xpos = HBORDER; ypos += lineHeight + VGAP;
  myMouseRange = new SliderWidget(myTab, _font, HBORDER, ypos,
                                  "Mouse axes range ", 0, 0, fontWidth * 4, "%");
  myMouseRange->setMinValue(1); myMouseRange->setMaxValue(100);
  myMouseRange->setTickmarkIntervals(4);
  wid.push_back(myMouseRange);

  // Add items for tab 2
  addToFocusList(wid, myTab, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::addCartridgeTab()
{
  // 4) Cartridge properties
  const int VBORDER = 8;
  const int HBORDER = 10;
  const int VGAP = 4;
  const int lineHeight = _font.getLineHeight(),
            fontHeight = _font.getFontHeight();

  int xpos, ypos, lwidth, fwidth, tabID;
  WidgetArray wid;
  VariantList items;

  wid.clear();
  tabID = myTab->addTab("Cartridge", TabWidget::AUTO_WIDTH);

  xpos = HBORDER; ypos = VBORDER;
  lwidth = _font.getStringWidth("Manufacturer ");
  fwidth = _w - lwidth - HBORDER * 2 - 2;
  new StaticTextWidget(myTab, _font, xpos, ypos + 1, lwidth, fontHeight, "Name");
  myName = new EditTextWidget(myTab, _font, xpos + lwidth, ypos - 1,
                              fwidth, lineHeight, "");
  wid.push_back(myName);

  ypos += lineHeight + VGAP;
  new StaticTextWidget(myTab, _font, xpos, ypos + 1, lwidth, fontHeight, "MD5");
  myMD5 = new EditTextWidget(myTab, _font, xpos + lwidth, ypos - 1,
                             fwidth, lineHeight, "");
  myMD5->setEditable(false);

  ypos += lineHeight + VGAP;
  new StaticTextWidget(myTab, _font, xpos, ypos + 1, lwidth, fontHeight, "Manufacturer");
  myManufacturer = new EditTextWidget(myTab, _font, xpos + lwidth, ypos - 1,
                                      fwidth, lineHeight, "");
  wid.push_back(myManufacturer);

  ypos += lineHeight + VGAP;
  new StaticTextWidget(myTab, _font, xpos, ypos + 1, lwidth, fontHeight,
                       "Model", TextAlign::Left);
  myModelNo = new EditTextWidget(myTab, _font, xpos + lwidth, ypos - 1,
                                 fwidth, lineHeight, "");
  wid.push_back(myModelNo);

  ypos += lineHeight + VGAP;
  new StaticTextWidget(myTab, _font, xpos, ypos + 1, lwidth, fontHeight, "Rarity");
  myRarity = new EditTextWidget(myTab, _font, xpos + lwidth, ypos - 1,
                                fwidth, lineHeight, "");
  wid.push_back(myRarity);

  ypos += lineHeight + VGAP;
  new StaticTextWidget(myTab, _font, xpos, ypos + 1, lwidth, fontHeight, "Note");
  myNote = new EditTextWidget(myTab, _font, xpos + lwidth, ypos - 1,
                              fwidth, lineHeight, "");
  wid.push_back(myNote);

  // Add items for tab 3
  addToFocusList(wid, myTab, tabID);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::addHighScoresTab()
{
  // 4) High Scores properties
  const int VBORDER = 8;
  const int HBORDER = 10;
  const int VGAP = 4;
  const int lineHeight = _font.getLineHeight(),
            fontHeight = _font.getFontHeight();

  int xpos, ypos, lwidth, fwidth, pwidth, tabID;
  WidgetArray wid;
  VariantList items;

  tabID = myTab->addTab("High Scores", TabWidget::AUTO_WIDTH);

  EditableWidget::TextFilter fAddr = [](char c) {
    return (c >= 'a' && c <= 'f') || (c >= '0' && c <= '9');
  };
  EditableWidget::TextFilter fVars = [](char c) {
    return (c >= '0' && c <= '9');
  };
  EditableWidget::TextFilter fSpecial = [](char c) {
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.'|| c == '-';
  };

  xpos = HBORDER; ypos = VBORDER;
  lwidth = _font.getStringWidth("Variations ");

  myHighScores = new CheckboxWidget(myTab, _font, xpos, ypos + 1, "Enable High Scores", kHiScoresChanged);

  xpos += 20; ypos += lineHeight + VGAP;

  items.clear();
  for (int i = 1; i <= HSM::MAX_PLAYERS; ++i)
    VarList::push_back(items, std::to_string(i), std::to_string(i));
  pwidth = _font.getStringWidth("4");

  myPlayersLabel = new StaticTextWidget(myTab, _font, xpos, ypos + 1, lwidth, fontHeight, "Players");
  myPlayers = new PopUpWidget(myTab, _font, xpos + lwidth, ypos, pwidth, lineHeight, items, "", 0, kHiScoresChanged);
  wid.push_back(myPlayers);

  int awidth = _font.getMaxCharWidth() * HSM::MAX_ADDR_CHARS + 4;
  int vwidth = _font.getStringWidth("123") + 4;

  myPlayersAddressLabel = new StaticTextWidget(myTab, _font, myPlayers->getRight() + 16, ypos + 1, "Address ");
  myPlayersAddress = new EditTextWidget(myTab, _font, myPlayersAddressLabel->getRight(), ypos - 1, awidth, lineHeight);
  myPlayersAddress->setTextFilter(fAddr);
  wid.push_back(myPlayersAddress);
  myPlayersAddressVal = new EditTextWidget(myTab, _font, myPlayersAddress->getRight() + 2, ypos - 1, vwidth, lineHeight);
  myPlayersAddressVal->setEditable(false);

  ypos += lineHeight + VGAP;

  fwidth = _font.getStringWidth("255") + 5;
  vwidth = _font.getStringWidth("123") + 4;
  int swidth = _font.getStringWidth("abcde") + 4;
  myVariationsLabel = new StaticTextWidget(myTab, _font, xpos, ypos + 1, lwidth, fontHeight, "Variations");
  myVariations = new EditTextWidget(myTab, _font, xpos + lwidth, ypos - 1, fwidth, lineHeight);
  myVariations->setTextFilter(fVars);
  wid.push_back(myVariations);

  myVarAddressLabel = new StaticTextWidget(myTab, _font, myPlayersAddressLabel->getLeft(), ypos + 1, "Address ");
  myVarAddress = new EditTextWidget(myTab, _font, myVarAddressLabel->getRight(), ypos - 1, awidth, lineHeight);
  myVarAddress->setTextFilter(fAddr);
  wid.push_back(myVarAddress);
  myVarAddressVal = new EditTextWidget(myTab, _font, myVarAddress->getRight() + 2, ypos - 1, vwidth, lineHeight);
  myVarAddressVal->setEditable(false);

  myVarsBCD = new CheckboxWidget(myTab, _font, myVarAddressVal->getRight() + 16, ypos + 1, "BCD", kHiScoresChanged);
  wid.push_back(myVarsBCD);

  myVarsZeroBased = new CheckboxWidget(myTab, _font, myVarsBCD->getRight() + 16, ypos + 1, "0-based", kHiScoresChanged);
  wid.push_back(myVarsZeroBased);

  ypos += lineHeight + VGAP;

  mySpecialLabel = new StaticTextWidget(myTab, _font, xpos, ypos + 1, "Special  ");
  mySpecialName = new EditTextWidget(myTab, _font, mySpecialLabel->getRight(), ypos - 1, swidth, lineHeight);
  mySpecialName->setTextFilter(fSpecial);
  wid.push_back(mySpecialName);

  mySpecialAddressLabel = new StaticTextWidget(myTab, _font, myPlayersAddressLabel->getLeft(), ypos + 1, "Address ");
  mySpecialAddress = new EditTextWidget(myTab, _font, mySpecialAddressLabel->getRight(), ypos - 1, awidth, lineHeight);
  mySpecialAddress->setTextFilter(fAddr);
  wid.push_back(mySpecialAddress);
  mySpecialAddressVal = new EditTextWidget(myTab, _font, mySpecialAddress->getRight() + 2, ypos - 1, vwidth, lineHeight);
  mySpecialAddressVal->setEditable(false);

  mySpecialBCD = new CheckboxWidget(myTab, _font, mySpecialAddressVal->getRight() + 16, ypos + 1, "BCD", kHiScoresChanged);
  wid.push_back(mySpecialBCD);

  mySpecialZeroBased = new CheckboxWidget(myTab, _font, mySpecialBCD->getRight() + 16, ypos + 1, "0-based", kHiScoresChanged);
  wid.push_back(mySpecialZeroBased);

  //xpos += 20;
  ypos += lineHeight + VGAP * 2;

  vwidth = _font.getStringWidth("AB") + 4;
  items.clear();
  for (int i = 1; i <= HSM::MAX_SCORE_DIGITS; ++i)
    VarList::push_back(items, std::to_string(i), std::to_string(i));

  myScoreDigitsLabel = new StaticTextWidget(myTab, _font, xpos, ypos + 1, "Score digits ");
  myScoreDigits = new PopUpWidget(myTab, _font, myScoreDigitsLabel->getRight(), ypos, pwidth, lineHeight,
                                  items, "", 0, kHiScoresChanged);
  wid.push_back(myScoreDigits);

  items.clear();
  for (int i = 0; i <= HSM::MAX_SCORE_DIGITS - 3; ++i)
    VarList::push_back(items, std::to_string(i), std::to_string(i));
  pwidth = _font.getStringWidth("0");

  myTrailingZeroesLabel = new StaticTextWidget(myTab, _font, myScoreDigits->getRight() + 30, ypos + 1, "0-digits ");
  myTrailingZeroes = new PopUpWidget(myTab, _font, myTrailingZeroesLabel->getRight(), ypos, pwidth, lineHeight,
                                     items, "", 0, kHiScoresChanged);
  wid.push_back(myTrailingZeroes);

  myScoreBCD = new CheckboxWidget(myTab, _font, myVarsBCD->getLeft(), ypos + 1, "BCD", kHiScoresChanged);
  wid.push_back(myScoreBCD);

  myScoreInvert = new CheckboxWidget(myTab, _font, myScoreBCD->getRight() + 16, ypos + 1, "Invert");
  wid.push_back(myScoreInvert);

  for (uInt32 p = 0; p < HSM::MAX_PLAYERS; ++p)
  {
    uInt32 s_xpos = xpos;
    ypos += lineHeight + VGAP;

    myScoreAddressesLabel[p] = new StaticTextWidget(myTab, _font, s_xpos, ypos + 1,
                                                    "P" + to_string(p + 1) + " Addresses ");
    s_xpos += myScoreAddressesLabel[p]->getWidth();
    for (uInt32 a = 0; a < HSM::MAX_SCORE_ADDR; ++a)
    {
      myScoreAddress[p][a] = new EditTextWidget(myTab, _font, s_xpos, ypos - 1, awidth, lineHeight);
      myScoreAddress[p][a]->setTextFilter(fAddr);
      wid.push_back(myScoreAddress[p][a]);
      s_xpos += myScoreAddress[p][a]->getWidth() + 2;

      myScoreAddressVal[p][a] = new EditTextWidget(myTab, _font, myScoreAddress[p][a]->getRight() + 2, ypos - 1, vwidth, lineHeight);
      myScoreAddressVal[p][a]->setEditable(false);
      s_xpos += myScoreAddressVal[p][a]->getWidth() + 16;
    }
    myCurrentScore[p] = new StaticTextWidget(myTab, _font, s_xpos, ypos + 1, "= 123456");
  }
  //myCurrentScoreLabel = new StaticTextWidget(myTab, _font, myCurrentScore[0]->getLeft(), myScoreBCD->getTop(), "Current");

  // Add items for tab 4
  addToFocusList(wid, myTab, tabID);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadConfig()
{
  if(instance().hasConsole())
  {
    myGameProperties = instance().console().properties();
  }
  else
  {
    const string& md5 = instance().launcher().selectedRomMD5();
    instance().propSet().getMD5(md5, myGameProperties);
  }

  loadEmulationProperties(myGameProperties);
  loadConsoleProperties(myGameProperties);
  loadControllerProperties(myGameProperties);
  loadCartridgeProperties(myGameProperties);
  loadHighScoresProperties(myGameProperties);

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
    uInt16 numBanks = instance().console().cartridge().bankCount();

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
  myPPBlend->setValue(stringToInt(blend));

  // set vertical center
  Int32 vcenter = stringToInt(props.get(PropType::Display_VCenter));
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

  // MouseAxis property (potentially contains 'range' information)
  istringstream m_axis(props.get(PropType::Controller_MouseAxis));
  string m_control, m_range;
  m_axis >> m_control;
  bool autoAxis = equalsIgnoreCase(m_control, "AUTO");
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
    myMouseRange->setValue(stringToInt(m_range));
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
void GameInfoDialog::loadHighScoresProperties(const Properties& props)
{
  HSM::ScoresInfo info;
  uInt32 numPlayers, numVariations;
  bool enable = instance().highScores().get(props, numPlayers, numVariations, info);

  myHighScores->setState(enable);

  myPlayers->setSelected(numPlayers);
  myVariations->setText(to_string(numVariations));

  ostringstream ss;

  myScoreDigits->setSelected(info.numDigits);
  myTrailingZeroes->setSelected(info.trailingZeroes);
  myScoreBCD->setState(info.scoreBCD);
  myScoreInvert->setState(info.scoreInvert);
  myVarsBCD->setState(info.varsBCD);
  myVarsZeroBased->setState(info.varsZeroBased);
  mySpecialName->setText(info.special);
  mySpecialBCD->setState(info.specialBCD);
  mySpecialZeroBased->setState(info.specialZeroBased);

  ss.str("");
  ss << hex << right //<< setw(HSM::MAX_ADDR_CHARS) << setfill(' ')
    << uppercase << info.playersAddr;
  myPlayersAddress->setText(ss.str());

  ss.str("");
  ss << hex << right //<< setw(HSM::MAX_ADDR_CHARS) << setfill(' ')
    << uppercase << info.varsAddr;
  myVarAddress->setText(ss.str());

  ss.str("");
  ss << hex << right //<< setw(HSM::MAX_ADDR_CHARS) << setfill(' ')
    << uppercase << info.specialAddr;
  mySpecialAddress->setText(ss.str());


  for (uInt32 p = 0; p < HSM::MAX_PLAYERS; ++p)
  {
    for (uInt32 a = 0; a < instance().highScores().numAddrBytes(info.numDigits, info.trailingZeroes); ++a)
    {
      if (p < numPlayers)
      {
        ss.str("");
        ss << hex << right //<< setw(HSM::MAX_ADDR_CHARS) << setfill(' ')
          << uppercase << info.scoresAddr[p][a];
        myScoreAddress[p][a]->setText(ss.str());
      }
      else
        myScoreAddress[p][a]->setText("");
    }
  }
  updateHighScoresWidgets();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::saveConfig()
{
  // Emulation properties
  myGameProperties.set(PropType::Cart_Type, myBSType->getSelectedTag().toString());
  myGameProperties.set(PropType::Cart_StartBank, myStartBank->getSelectedTag().toString());
  myGameProperties.set(PropType::Display_Format, myFormat->getSelectedTag().toString());
  myGameProperties.set(PropType::Display_Phosphor, myPhosphor->getState() ? "YES" : "NO");
  myGameProperties.set(PropType::Display_PPBlend, myPPBlend->getValueLabel() == "Off" ? "0" :
                       myPPBlend->getValueLabel());
  Int32 vcenter = myVCenter->getValue();

  myGameProperties.set(PropType::Display_VCenter, to_string(vcenter));
  myGameProperties.set(PropType::Cart_Sound, mySound->getState() ? "STEREO" : "MONO");

  // Console properties
  myGameProperties.set(PropType::Console_TVType, myTVTypeGroup->getSelected() ? "BW" : "COLOR");
  myGameProperties.set(PropType::Console_LeftDiff, myLeftDiffGroup->getSelected() ? "B" : "A");
  myGameProperties.set(PropType::Console_RightDiff, myRightDiffGroup->getSelected() ? "B" : "A");

  // Controller properties
  myGameProperties.set(PropType::Controller_Left, myLeftPort->getSelectedTag().toString());
  myGameProperties.set(PropType::Controller_Right, myRightPort->getSelectedTag().toString());
  myGameProperties.set(PropType::Console_SwapPorts, (mySwapPorts->isEnabled() && mySwapPorts->getState()) ? "YES" : "NO");
  myGameProperties.set(PropType::Controller_SwapPaddles, (/*mySwapPaddles->isEnabled() &&*/ mySwapPaddles->getState()) ? "YES" : "NO");

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

  saveHighScoresProperties();

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
    instance().console().updateVcenter(vcenter);
    instance().console().initializeAudio();

    // update 'Console' tab settings immediately
    instance().console().switches().setTvColor(myTVTypeGroup->getSelected() == 0);
    instance().console().switches().setLeftDifficultyA(myLeftDiffGroup->getSelected() == 0);
    instance().console().switches().setRightDifficultyA(myRightDiffGroup->getSelected() == 0);

    // update 'Controllers' tab settings immediately
    instance().console().setControllers(myGameProperties.get(PropType::Cart_MD5));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::saveHighScoresProperties()
{
  HSM::ScoresInfo info;

  if (myHighScores->getState())
  {
    string strText;

    // limit variants and special size
    strText = myVariations->getText();
    strText = strText.substr(0, 3);
    myVariations->setText(strText);

    strText = mySpecialName->getText();
    strText = strText.substr(0, HSM::MAX_SPECIAL_NAME);
    mySpecialName->setText(strText);

    // fill format
    info.varsZeroBased = myVarsZeroBased->getState();
    info.varsBCD = myVarsBCD->getState();

    info.special = mySpecialName->getText();
    info.specialZeroBased = mySpecialZeroBased->getState();
    info.specialBCD = mySpecialBCD->getState();

    info.numDigits = myScoreDigits->getSelected() + 1;
    info.trailingZeroes = myTrailingZeroes->getSelected();
    info.scoreBCD = myScoreBCD->getState();
    info.scoreInvert = myScoreInvert->getState();

    // fill addresses
    string strAddr;

    strAddr = myPlayersAddress->getText();
    info.playersAddr = stringToIntBase16(strAddr, HSM::DEFAULT_ADDRESS);
    strAddr = myVarAddress->getText();
    info.varsAddr = stringToIntBase16(strAddr, HSM::DEFAULT_ADDRESS);
    strAddr = mySpecialAddress->getText();
    info.specialAddr = stringToIntBase16(strAddr, HSM::DEFAULT_ADDRESS);

    for (uInt32 p = 0; p < HSM::MAX_PLAYERS; ++p)
    {
      for (uInt32 a = 0; a < HSM::MAX_SCORE_ADDR; ++a)
      {
        strAddr = myScoreAddress[p][a]->getText();
        info.scoresAddr[p][a] = stringToIntBase16(strAddr, HSM::DEFAULT_ADDRESS);
      }
    }

    string strVars = myVariations->getText();

    instance().highScores().set(myGameProperties, myPlayers->getSelected() + 1,
                                stringToInt(strVars, HSM::DEFAULT_VARIATION), info);
  }
  else
  {
    myGameProperties.reset(PropType::Cart_Players);
    myGameProperties.reset(PropType::Cart_Variations);
    myGameProperties.reset(PropType::Cart_Formats);
    myGameProperties.reset(PropType::Cart_Addresses);
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

    case 4: // High Scores properties
      loadHighScoresProperties(defaultProperties);
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
      label = (!swapPorts ? instance().console().leftController().name()
               : instance().console().rightController().name()) + " detected";
    else if(autoDetect)
      label = ControllerDetector::detectName(image.get(), size, type,
                                             !swapPorts ? Controller::Jack::Left : Controller::Jack::Right,
                                             instance().settings()) + " detected";
  }
  myLeftPortDetected->setLabel(label);

  label = "";
  type = Controller::getType(myRightPort->getSelectedTag().toString());

  if(type == Controller::Type::Unknown)
  {
    if(instance().hasConsole())
      label = (!swapPorts ? instance().console().rightController().name()
               : instance().console().leftController().name()) + " detected";
    else if(autoDetect)
      label = ControllerDetector::detectName(image.get(), size, type,
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
  bool enableSwapPaddles = startsWithIgnoreCase(contrLeft, "PADDLES") ||
    startsWithIgnoreCase(contrRight, "PADDLES") ||
    startsWithIgnoreCase(myLeftPortDetected->getLabel(), "Paddles") ||
    startsWithIgnoreCase(myRightPortDetected->getLabel(), "Paddles");

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

  mySwapPorts->setEnabled(enableSelectControl);
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

  if(lport.type() == Controller::Type::SaveKey || lport.type() == Controller::Type::AtariVox)
  {
    SaveKey& skey = static_cast<SaveKey&>(lport);
    skey.eraseCurrent();
  }

  if(rport.type() == Controller::Type::SaveKey || rport.type() == Controller::Type::AtariVox)
  {
    SaveKey& skey = static_cast<SaveKey&>(rport);
    skey.eraseCurrent();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::updateHighScoresWidgets()
{
  bool enable = myHighScores->getState();
  uInt32 players = myPlayers->getSelected() + 1;
  bool enablePlayers = enable && players > 1;
  bool enableVars = enable && myVariations->getText() > "1";
  bool enableSpecial = enable && !mySpecialName->getText().empty();
  uInt32 numAddr = instance().highScores().numAddrBytes(myScoreDigits->getSelected() + 1,
                                                        myTrailingZeroes->getSelected());

  // enable widgets
  myPlayersLabel->setEnabled(enable);
  myPlayers->setEnabled(enable);
  myPlayersAddressLabel->setEnabled(enablePlayers);
  myPlayersAddress->setEnabled(enablePlayers);
  myPlayersAddress->setEditable(enablePlayers);
  myPlayersAddressVal->setEnabled(enablePlayers);

  myVariationsLabel->setEnabled(enable);
  myVariations->setEnabled(enable);
  myVariations->setEditable(enable);
  myVarAddressLabel->setEnabled(enableVars);
  myVarAddress->setEnabled(enableVars);
  myVarAddress->setEditable(enableVars);
  myVarAddressVal->setEnabled(enableVars);
  myVarsBCD->setEnabled(enableVars && stringToInt(myVariations->getText(), 1) >= 10);
  myVarsZeroBased->setEnabled(enableVars);

  mySpecialLabel->setEnabled(enable);
  mySpecialName->setEnabled(enable);
  mySpecialName->setEditable(enable);
  mySpecialAddressLabel->setEnabled(enableSpecial);
  mySpecialAddress->setEnabled(enableSpecial);
  mySpecialAddress->setEditable(enableSpecial);
  mySpecialAddressVal->setEnabled(enableSpecial);
  mySpecialBCD->setEnabled(enableSpecial);
  mySpecialZeroBased->setEnabled(enableSpecial);

  myScoreDigitsLabel->setEnabled(enable);
  myScoreDigits->setEnabled(enable);
  myScoreBCD->setEnabled(enable);
  myTrailingZeroesLabel->setEnabled(enable);
  myTrailingZeroes->setEnabled(enable);
  myScoreInvert->setEnabled(enable);
  //myCurrentScoreLabel->setEnabled(enable);

  for (uInt32 p = 0; p < HSM::MAX_PLAYERS; ++p)
  {
    enable &= players > p;
    myScoreAddressesLabel[p]->setEnabled(enable);

    for (uInt32 a = 0; a < HSM::MAX_SCORE_ADDR; ++a)
    {
      myScoreAddress[p][a]->setEnabled(enable && numAddr > a);
      myScoreAddressVal[p][a]->setEnabled(enable && numAddr > a);
    }
    myCurrentScore[p]->setEnabled(enable);
  }

  // verify and update widget data

  // update players and variations RAM values
  setAddressVal(myPlayersAddress, myPlayersAddressVal);
  setAddressVal(myVarAddress, myVarAddressVal, myVarsBCD->getState(),
                myVarsZeroBased->getState() ? 1 : 0);
  setAddressVal(mySpecialAddress, mySpecialAddressVal, mySpecialBCD->getState(),
                mySpecialZeroBased->getState() ? 1 : 0);

  // update score RAM values and resulting scores
  for (uInt32 p = 0; p < HSM::MAX_PLAYERS; ++p)
  {
    if (p < players)
    {
      HSM::ScoreAddresses scoreAddr;

      for (uInt32 a = 0; a < numAddr; ++a)
      {
        setAddressVal(myScoreAddress[p][a], myScoreAddressVal[p][a]);
        string strAddr = myScoreAddress[p][a]->getText();
        scoreAddr[a] = stringToIntBase16(strAddr, HSM::DEFAULT_ADDRESS);
      }

      Int32 score = instance().highScores().score(p, numAddr, myTrailingZeroes->getSelected(),
                                                  myScoreBCD->getState(), scoreAddr);
      if (score >= 0)
      {
        ostringstream ss;

        ss.str("");
        ss << "= " << right << setw(myScoreDigits->getSelected() + 1) << setfill(' ') << score;
        myCurrentScore[p]->setLabel(ss.str());
      }
      else
        myCurrentScore[p]->setLabel("");
    }
    else
    {
      for (uInt32 a = 0; a < numAddr; ++a)
        myScoreAddressVal[p][a]->setText("");
      myCurrentScore[p]->setLabel("");
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::setAddressVal(EditTextWidget* addressWidget, EditTextWidget* valWidget,
                                   bool isBCD, uInt8 incVal)
{
  string strAddr;

  // limit address size
  strAddr = addressWidget->getText();
  strAddr = strAddr.substr(0, HSM::MAX_ADDR_CHARS);

  if (instance().hasConsole() && valWidget->isEnabled())
  {
    System& system = instance().console().system();
    uInt16 addr;
    uInt8 val;
    ostringstream ss;

    // convert to number and read from memory
    addr = stringToIntBase16(strAddr, HSM::DEFAULT_ADDRESS);
    val = system.peek(addr) + incVal;
    // format output and display in value widget
    if (isBCD)
      ss << hex;
    ss << right //<< setw(2) << setfill(' ')
      << uppercase << uInt16(val);
    valWidget->setText(ss.str());
  }
  else
    valWidget->setText("");
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
      if(data == 2)  // 'Controllers' tab selected
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

    case kMCtrlChanged:
    {
      bool state = myMouseControl->getState();
      myMouseX->setEnabled(state);
      myMouseY->setEnabled(state);
      break;
    }

    case EditTextWidget::kChangedCmd:
    case kHiScoresChanged:
      updateHighScoresWidgets();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
