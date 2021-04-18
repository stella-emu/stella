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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "bspf.hxx"
#include "OSystem.hxx"
#include "Console.hxx"
#include "EventHandler.hxx"
#include "Joystick.hxx"
#include "Paddles.hxx"
#include "PointingDevice.hxx"
#include "Driving.hxx"
#include "SaveKey.hxx"
#include "AtariVox.hxx"
#include "Settings.hxx"
#include "EventMappingWidget.hxx"
#include "EditTextWidget.hxx"
#include "JoystickDialog.hxx"
#include "PopUpWidget.hxx"
#include "TabWidget.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "MessageBox.hxx"
#include "MediaFactory.hxx"
#include "InputDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputDialog::InputDialog(OSystem& osystem, DialogContainer& parent,
                         const GUI::Font& font, int max_w, int max_h)
  : Dialog(osystem, parent, font, "Input settings"),
    myMaxWidth{max_w},
    myMaxHeight{max_h}
{
  const int lineHeight   = Dialog::lineHeight(),
            fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();
  int xpos, ypos, tabID;

  // Set real dimensions
  setSize(48 * fontWidth + PopUpWidget::dropDownWidth(_font) + HBORDER * 2,
          _th + VGAP * 3 + lineHeight + 13 * (lineHeight + VGAP) + VGAP * 8 + buttonHeight + VBORDER * 3,
          max_w, max_h);

  // The tab widget
  xpos = 2; ypos = VGAP + _th;
  myTab = new TabWidget(this, _font, xpos, ypos,
                        _w - 2*xpos,
                        _h -_th - VGAP - buttonHeight - VBORDER * 2);
  addTabWidget(myTab);

  // 1) Event mapper for emulation actions
  tabID = myTab->addTab(" Emul. Events ", TabWidget::AUTO_WIDTH);
  myEmulEventMapper = new EventMappingWidget(myTab, _font, 2, 2,
                                             myTab->getWidth(),
                                             myTab->getHeight() - VGAP,
                                             EventMode::kEmulationMode);
  myTab->setParentWidget(tabID, myEmulEventMapper);
  addToFocusList(myEmulEventMapper->getFocusList(), myTab, tabID);
  myTab->parentWidget(tabID)->setHelpAnchor("Remapping");

  // 2) Event mapper for UI actions
  tabID = myTab->addTab(" UI Events ", TabWidget::AUTO_WIDTH);
  myMenuEventMapper = new EventMappingWidget(myTab, _font, 2, 2,
                                             myTab->getWidth(),
                                             myTab->getHeight() - VGAP,
                                             EventMode::kMenuMode);
  myTab->setParentWidget(tabID, myMenuEventMapper);
  addToFocusList(myMenuEventMapper->getFocusList(), myTab, tabID);
  myTab->parentWidget(tabID)->setHelpAnchor("Remapping");

  // 3) Devices & ports
  addDevicePortTab();

  // 4) Mouse
  addMouseTab();

  // Finalize the tabs, and activate the first tab
  myTab->activateTabs();
  myTab->setActiveTab(0);

  // Add Defaults, OK and Cancel buttons
  WidgetArray wid;
  addDefaultsOKCancelBGroup(wid, _font);
  addBGroupToFocusList(wid);

  setHelpAnchor("Remapping");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputDialog::~InputDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::addDevicePortTab()
{
  const int lineHeight   = Dialog::lineHeight(),
            fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();
  int xpos, ypos, lwidth, tabID;
  WidgetArray wid;

  // Devices/ports
  tabID = myTab->addTab("Devices & Ports", TabWidget::AUTO_WIDTH);

  ypos = VBORDER;
  lwidth = _font.getStringWidth("Digital paddle sensitivity ");

  // Add joystick deadzone setting
  myDeadzone = new SliderWidget(myTab, _font, HBORDER, ypos - 1, 13 * fontWidth, lineHeight,
                                "Joystick deadzone size", lwidth, kDeadzoneChanged, 5 * fontWidth);
  myDeadzone->setMinValue(Joystick::DEAD_ZONE_MIN);
  myDeadzone->setMaxValue(Joystick::DEAD_ZONE_MAX);
  myDeadzone->setTickmarkIntervals(4);
  wid.push_back(myDeadzone);

  xpos = HBORDER; ypos += lineHeight + VGAP * 3;
  new StaticTextWidget(myTab, _font, xpos, ypos+1, "Analog paddle:");
  xpos += fontWidth * 2;

  // Add analog paddle sensitivity
  ypos += lineHeight;
  myPaddleSpeed = new SliderWidget(myTab, _font, xpos, ypos - 1, 13 * fontWidth, lineHeight,
                                   "Sensitivity",
                                   lwidth - fontWidth * 2, kPSpeedChanged, 4 * fontWidth, "%");
  myPaddleSpeed->setMinValue(0); myPaddleSpeed->setMaxValue(Paddles::MAX_ANALOG_SENSE);
  myPaddleSpeed->setTickmarkIntervals(3);
  wid.push_back(myPaddleSpeed);


  // Add dejitter (analog paddles)
  ypos += lineHeight + VGAP;
  myDejitterBase = new SliderWidget(myTab, _font, xpos, ypos - 1, 13 * fontWidth, lineHeight,
                                    "Dejitter averaging", lwidth - fontWidth * 2,
                                    kDejitterAvChanged, 3 * fontWidth);
  myDejitterBase->setMinValue(Paddles::MIN_DEJITTER);
  myDejitterBase->setMaxValue(Paddles::MAX_DEJITTER);
  myDejitterBase->setTickmarkIntervals(5);
  myDejitterBase->setToolTip("Adjust paddle input averaging.\n"
                             "Note: Already implemented in 2600-daptor");
  //xpos += myDejitterBase->getWidth() + fontWidth - 4;
  wid.push_back(myDejitterBase);

  ypos += lineHeight + VGAP;
  myDejitterDiff = new SliderWidget(myTab, _font, xpos, ypos - 1, 13 * fontWidth, lineHeight,
                                    "Dejitter reaction", lwidth - fontWidth * 2,
                                    kDejitterReChanged, 3 * fontWidth);
  myDejitterDiff->setMinValue(Paddles::MIN_DEJITTER);
  myDejitterDiff->setMaxValue(Paddles::MAX_DEJITTER);
  myDejitterDiff->setTickmarkIntervals(5);
  myDejitterDiff->setToolTip("Adjust paddle reaction to fast movements.");
  wid.push_back(myDejitterDiff);

  // Add paddle speed (digital emulation)
  ypos += lineHeight + VGAP * 3;
  myDPaddleSpeed = new SliderWidget(myTab, _font, HBORDER, ypos - 1, 13 * fontWidth, lineHeight,
                                    "Digital paddle sensitivity",
                                    lwidth, kDPSpeedChanged, 4 * fontWidth, "%");
  myDPaddleSpeed->setMinValue(1); myDPaddleSpeed->setMaxValue(20);
  myDPaddleSpeed->setTickmarkIntervals(4);
  wid.push_back(myDPaddleSpeed);

  ypos += lineHeight + VGAP * 3;
  myAutoFireRate = new SliderWidget(myTab, _font, HBORDER, ypos - 1, 13 * fontWidth, lineHeight,
                                    "Autofire rate",
                                    lwidth, kAutoFireChanged, 5 * fontWidth, "Hz");
  myAutoFireRate->setMinValue(0); myAutoFireRate->setMaxValue(30);
  myAutoFireRate->setTickmarkIntervals(6);
  wid.push_back(myAutoFireRate);

  // Add 'allow all 4 directions' for joystick
  ypos += lineHeight + VGAP * 4;
  myAllowAll4 = new CheckboxWidget(myTab, _font, HBORDER, ypos,
                  "Allow all 4 directions on joystick");
  wid.push_back(myAllowAll4);

  // Enable/disable modifier key-combos
  ypos += lineHeight + VGAP;
  myModCombo = new CheckboxWidget(myTab, _font, HBORDER, ypos,
                  "Use modifier key combos");
  wid.push_back(myModCombo);
  ypos += lineHeight + VGAP;

  // Stelladaptor mappings
  mySAPort = new CheckboxWidget(myTab, _font, HBORDER, ypos,
                                "Swap Stelladaptor ports");
  wid.push_back(mySAPort);

  int fwidth;

  // Add EEPROM erase (part 1/2)
  ypos += VGAP * 3;
  fwidth = _font.getStringWidth("AtariVox/SaveKey");
  new StaticTextWidget(myTab, _font, _w - HBORDER - 2 - fwidth, ypos,
                       "AtariVox/SaveKey");

  // Show joystick database
  ypos += lineHeight;
  lwidth = Dialog::buttonWidth("Joystick Database" + ELLIPSIS);
  myJoyDlgButton = new ButtonWidget(myTab, _font, HBORDER, ypos, lwidth, buttonHeight,
                                    "Joystick Database" + ELLIPSIS, kDBButtonPressed);
  wid.push_back(myJoyDlgButton);

  // Add EEPROM erase (part 1/2)
  myEraseEEPROMButton = new ButtonWidget(myTab, _font, _w - HBORDER - 2 - fwidth, ypos,
                                         fwidth, buttonHeight,
                                        "Erase EEPROM", kEEButtonPressed);
  wid.push_back(myEraseEEPROMButton);

  // Add AtariVox serial port
  ypos += lineHeight + VGAP * 3;
  lwidth = _font.getStringWidth("AtariVox serial port ");
  fwidth = _w - HBORDER * 2 - 2 - lwidth - PopUpWidget::dropDownWidth(_font);
  myAVoxPort = new PopUpWidget(myTab, _font, HBORDER, ypos, fwidth, lineHeight, EmptyVarList,
                               "AtariVox serial port ", lwidth, kCursorStateChanged);
  myAVoxPort->setEditable(true);
  wid.push_back(myAVoxPort);

  // Add items for virtual device ports
  addToFocusList(wid, myTab, tabID);

  myTab->parentWidget(tabID)->setHelpAnchor("DevicesPorts");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::addMouseTab()
{
  const int lineHeight = Dialog::lineHeight(),
            fontWidth  = Dialog::fontWidth(),
            VBORDER    = Dialog::vBorder(),
            HBORDER    = Dialog::hBorder(),
            VGAP       = Dialog::vGap(),
            INDENT     = Dialog::indent();
  int xpos = HBORDER, ypos, lwidth, pwidth, tabID;
  WidgetArray wid;
  VariantList items;

  // Mouse
  tabID = myTab->addTab(" Mouse ", TabWidget::AUTO_WIDTH);

  ypos = VBORDER;
  lwidth = _font.getStringWidth("Use mouse as a controller ");
  pwidth = _font.getStringWidth("-UI, -Emulation");

  // Use mouse as controller
  VarList::push_back(items, "Always", "always");
  VarList::push_back(items, "Analog devices", "analog");
  VarList::push_back(items, "Never", "never");
  myMouseControl = new PopUpWidget(myTab, _font, xpos, ypos, pwidth, lineHeight, items,
                                   "Use mouse as a controller ", lwidth, kMouseCtrlChanged);
  wid.push_back(myMouseControl);

  ypos += lineHeight + VGAP;
  myMouseSensitivity = new StaticTextWidget(myTab, _font, xpos, ypos + 1, "Sensitivity:");

  // Add paddle speed (mouse emulation)
  xpos += INDENT;  ypos += lineHeight + VGAP;
  lwidth -= INDENT;
  myMPaddleSpeed = new SliderWidget(myTab, _font, xpos, ypos - 1, 13 * fontWidth, lineHeight,
                                    "Paddle",
                                    lwidth, kMPSpeedChanged, 4 * fontWidth, "%");
  myMPaddleSpeed->setMinValue(1); myMPaddleSpeed->setMaxValue(20);
  myMPaddleSpeed->setTickmarkIntervals(4);
  wid.push_back(myMPaddleSpeed);

  // Add trackball speed
  ypos += lineHeight + VGAP;
  myTrackBallSpeed = new SliderWidget(myTab, _font, xpos, ypos - 1, 13 * fontWidth, lineHeight,
                                      "Trackball",
                                      lwidth, kTBSpeedChanged, 4 * fontWidth, "%");
  myTrackBallSpeed->setMinValue(1); myTrackBallSpeed->setMaxValue(20);
  myTrackBallSpeed->setTickmarkIntervals(4);
  wid.push_back(myTrackBallSpeed);

  // Add driving controller speed
  ypos += lineHeight + VGAP;
  myDrivingSpeed = new SliderWidget(myTab, _font, xpos, ypos - 1, 13 * fontWidth, lineHeight,
                                    "Driving controller",
                                    lwidth, kDCSpeedChanged, 4 * fontWidth, "%");
  myDrivingSpeed->setMinValue(1); myDrivingSpeed->setMaxValue(20);
  myDrivingSpeed->setTickmarkIntervals(4);
  myDrivingSpeed->setToolTip("Adjust driving controller sensitivity for digital and mouse input.");
  wid.push_back(myDrivingSpeed);

  // Mouse cursor state
  lwidth += INDENT;
  ypos += lineHeight + VGAP * 4;
  items.clear();
  VarList::push_back(items, "-UI, -Emulation", "0");
  VarList::push_back(items, "-UI, +Emulation", "1");
  VarList::push_back(items, "+UI, -Emulation", "2");
  VarList::push_back(items, "+UI, +Emulation", "3");
  myCursorState = new PopUpWidget(myTab, _font, HBORDER, ypos, pwidth, lineHeight, items,
                                  "Mouse cursor visibility ", lwidth, kCursorStateChanged);
  wid.push_back(myCursorState);
#ifndef WINDOWED_SUPPORT
  myCursorState->clearFlags(Widget::FLAG_ENABLED);
#endif

  // Grab mouse (in windowed mode)
  ypos += lineHeight + VGAP;
  myGrabMouse = new CheckboxWidget(myTab, _font, HBORDER, ypos,
                                   "Grab mouse in emulation mode");
  wid.push_back(myGrabMouse);
#ifndef WINDOWED_SUPPORT
  myGrabMouse->clearFlags(Widget::FLAG_ENABLED);
#endif

  // Add items for mouse
  addToFocusList(wid, myTab, tabID);

  myTab->parentWidget(tabID)->setHelpAnchor("Mouse");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::loadConfig()
{
  Settings& settings = instance().settings();

  // Left & right ports
  mySAPort->setState(settings.getString("saport") == "rl");

  // Use mouse as a controller
  myMouseControl->setSelected(
    settings.getString("usemouse"), "analog");
  handleMouseControlState();

  // Mouse cursor state
  myCursorState->setSelected(settings.getString("cursor"), "2");
  handleCursorState();

  // Joystick deadzone
  myDeadzone->setValue(settings.getInt("joydeadzone"));

  // Paddle speed (analog)
  myPaddleSpeed->setValue(settings.getInt("psense"));
  // Paddle dejitter (analog)
  myDejitterBase->setValue(settings.getInt("dejitter.base"));
  myDejitterDiff->setValue(settings.getInt("dejitter.diff"));

  // Paddle speed (digital and mouse)
  myDPaddleSpeed->setValue(settings.getInt("dsense"));
  myMPaddleSpeed->setValue(settings.getInt("msense"));

  // Trackball speed
  myTrackBallSpeed->setValue(settings.getInt("tsense"));
  // Driving controller speed
  myDrivingSpeed->setValue(settings.getInt("dcsense"));

  // Autofire rate
  myAutoFireRate->setValue(settings.getInt("autofirerate"));

  // AtariVox serial port
  const string& avoxport = settings.getString("avoxport");
  const StringList ports = MediaFactory::createSerialPort()->portNames();
  VariantList items;

  for(const auto& port: ports)
    VarList::push_back(items, port, port);
  if(avoxport != EmptyString && !BSPF::contains(ports, avoxport))
    VarList::push_back(items, avoxport, avoxport);
  if(items.size() == 0)
    VarList::push_back(items, "None detected");

  myAVoxPort->addItems(items);
  myAVoxPort->setSelected(avoxport);

  // EEPROM erase (only enable in emulation mode and for valid controllers)
  if(instance().hasConsole())
  {
    Controller& lport = instance().console().leftController();
    Controller& rport = instance().console().rightController();

    myEraseEEPROMButton->setEnabled(lport.type() == Controller::Type::SaveKey || lport.type() == Controller::Type::AtariVox ||
                                    rport.type() == Controller::Type::SaveKey || rport.type() == Controller::Type::AtariVox);
  }
  else
    myEraseEEPROMButton->setEnabled(false);

  // Allow all 4 joystick directions
  myAllowAll4->setState(settings.getBool("joyallow4"));

  // Grab mouse
  myGrabMouse->setState(settings.getBool("grabmouse"));

  // Enable/disable modifier key-combos
  myModCombo->setState(settings.getBool("modcombo"));

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::saveConfig()
{
  Settings& settings = instance().settings();

  // *** Device & Ports ***
  // Joystick deadzone
  int deadzone = myDeadzone->getValue();
  settings.setValue("joydeadzone", deadzone);
  Joystick::setDeadZone(deadzone);

  // Paddle speed (analog)
  int sensitivity = myPaddleSpeed->getValue();
  settings.setValue("psense", sensitivity);
  Paddles::setAnalogSensitivity(sensitivity);

  // Paddle speed (digital and mouse)
  int dejitter = myDejitterBase->getValue();
  settings.setValue("dejitter.base", dejitter);
  Paddles::setDejitterBase(dejitter);
  dejitter = myDejitterDiff->getValue();
  settings.setValue("dejitter.diff", dejitter);
  Paddles::setDejitterDiff(dejitter);

  sensitivity = myDPaddleSpeed->getValue();
  settings.setValue("dsense", sensitivity);
  Paddles::setDigitalSensitivity(sensitivity);

  // Autofire rate
  int rate = myAutoFireRate->getValue();
  settings.setValue("autofirerate", rate);
  Controller::setAutoFireRate(rate);

  // Allow all 4 joystick directions
  bool allowall4 = myAllowAll4->getState();
  settings.setValue("joyallow4", allowall4);
  instance().eventHandler().allowAllDirections(allowall4);

  // Enable/disable modifier key-combos
  settings.setValue("modcombo", myModCombo->getState());

  // Left & right ports
  instance().eventHandler().mapStelladaptors(mySAPort->getState() ? "rl" : "lr");

  // AtariVox serial port
  settings.setValue("avoxport", myAVoxPort->getText());

  // *** Mouse ***
  // Use mouse as a controller
  const string& usemouse = myMouseControl->getSelectedTag().toString();
  settings.setValue("usemouse", usemouse);
  instance().eventHandler().setMouseControllerMode(usemouse);

  sensitivity = myMPaddleSpeed->getValue();
  settings.setValue("msense", sensitivity);
  Paddles::setMouseSensitivity(sensitivity);

  // Trackball speed
  sensitivity = myTrackBallSpeed->getValue();
  settings.setValue("tsense", sensitivity);
  PointingDevice::setSensitivity(sensitivity);

  // Driving controller speed
  sensitivity = myDrivingSpeed->getValue();
  settings.setValue("dcsense", sensitivity);
  Driving::setSensitivity(sensitivity);

  // Grab mouse and hide cursor
  const string& cursor = myCursorState->getSelectedTag().toString();
  settings.setValue("cursor", cursor);

  // only allow grab mouse if cursor is hidden in emulation
  int state = myCursorState->getSelected();
  bool enableGrab = state != 1 && state != 3;
  bool grab = enableGrab ? myGrabMouse->getState() : false;
  settings.setValue("grabmouse", grab);
  instance().frameBuffer().enableGrabMouse(grab);

  instance().eventHandler().saveKeyMapping();
  instance().eventHandler().saveJoyMapping();
//  instance().saveConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::setDefaults()
{
  switch(myTab->getActiveTab())
  {
    case 0:  // Emulation events
      myEmulEventMapper->setDefaults();
      break;

    case 1:  // UI events
      myMenuEventMapper->setDefaults();
      break;

    case 2:  // Devices & Ports
      // Joystick deadzone
      myDeadzone->setValue(0);

      // Paddle speed (analog)
      myPaddleSpeed->setValue(20);
    #if defined(RETRON77)
      myDejitterBase->setValue(2);
      myDejitterDiff->setValue(6);
    #else
      myDejitterBase->setValue(0);
      myDejitterDiff->setValue(0);
    #endif

      // Paddle speed (digital)
      myDPaddleSpeed->setValue(10);

      // Autofire rate
      myAutoFireRate->setValue(0);

      // Allow all 4 joystick directions
      myAllowAll4->setState(false);

      // Enable/disable modifier key-combos
      myModCombo->setState(true);

      // Left & right ports
      mySAPort->setState(false);

      // AtariVox serial port
      myAVoxPort->setSelectedIndex(0);
      break;

    case 3:  // Mouse
      // Use mouse as a controller
      myMouseControl->setSelected("analog");

      // Paddle speed (mouse)
      myMPaddleSpeed->setValue(10);
      myTrackBallSpeed->setValue(10);
      myDrivingSpeed->setValue(10);

      // Mouse cursor state
      myCursorState->setSelected("2");

      // Grab mouse
      myGrabMouse->setState(true);

      handleMouseControlState();
      handleCursorState();
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool InputDialog::repeatEnabled()
{
  return !myEmulEventMapper->isRemapping() && !myMenuEventMapper->isRemapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleKeyDown(StellaKey key, StellaMod mod, bool repeated)
{
  // Remap key events in remap mode, otherwise pass to parent dialog
  if (myEmulEventMapper->remapMode())
    myEmulEventMapper->handleKeyDown(key, mod);
  else if (myMenuEventMapper->remapMode())
    myMenuEventMapper->handleKeyDown(key, mod);
  else
    Dialog::handleKeyDown(key, mod);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleKeyUp(StellaKey key, StellaMod mod)
{
  // Remap key events in remap mode, otherwise pass to parent dialog
  if (myEmulEventMapper->remapMode())
    myEmulEventMapper->handleKeyUp(key, mod);
  else if (myMenuEventMapper->remapMode())
    myMenuEventMapper->handleKeyUp(key, mod);
  else
    Dialog::handleKeyUp(key, mod);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleJoyDown(int stick, int button, bool longPress)
{
  // Remap joystick buttons in remap mode, otherwise pass to parent dialog
  if(myEmulEventMapper->remapMode())
    myEmulEventMapper->handleJoyDown(stick, button);
  else if(myMenuEventMapper->remapMode())
    myMenuEventMapper->handleJoyDown(stick, button);
  else
    Dialog::handleJoyDown(stick, button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleJoyUp(int stick, int button)
{
  // Remap joystick buttons in remap mode, otherwise pass to parent dialog
  if (myEmulEventMapper->remapMode())
    myEmulEventMapper->handleJoyUp(stick, button);
  else if (myMenuEventMapper->remapMode())
    myMenuEventMapper->handleJoyUp(stick, button);
  else
    Dialog::handleJoyUp(stick, button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleJoyAxis(int stick, JoyAxis axis, JoyDir adir, int button)
{
  // Remap joystick axis in remap mode, otherwise pass to parent dialog
  if(myEmulEventMapper->remapMode())
    myEmulEventMapper->handleJoyAxis(stick, axis, adir, button);
  else if(myMenuEventMapper->remapMode())
    myMenuEventMapper->handleJoyAxis(stick, axis, adir, button);
  else
    Dialog::handleJoyAxis(stick, axis, adir, button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool InputDialog::handleJoyHat(int stick, int hat, JoyHatDir hdir, int button)
{
  // Remap joystick hat in remap mode, otherwise pass to parent dialog
  if(myEmulEventMapper->remapMode())
    return myEmulEventMapper->handleJoyHat(stick, hat, hdir, button);
  else if(myMenuEventMapper->remapMode())
    return myMenuEventMapper->handleJoyHat(stick, hat, hdir, button);
  else
    return Dialog::handleJoyHat(stick, hat, hdir, button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::eraseEEPROM()
{
  // This method will only be callable if a console exists, so we don't
  // need to check again here
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
void InputDialog::handleCommand(CommandSender* sender, int cmd,
                                int data, int id)
{
  switch(cmd)
  {
    case GuiObject::kOKCmd:
      saveConfig();
      close();
      break;

    case GuiObject::kCloseCmd:
      // Revert changes made to event mapping
      close();
      break;

    case GuiObject::kDefaultsCmd:
      setDefaults();
      break;

    case kDeadzoneChanged:
      myDeadzone->setValueLabel(Joystick::deadZoneValue(myDeadzone->getValue()));
      break;

    case kPSpeedChanged:
      myPaddleSpeed->setValueLabel(std::round(Paddles::setAnalogSensitivity(
            myPaddleSpeed->getValue()) * 100.F));
      break;

    case kDejitterAvChanged:
      updateDejitterAveraging();
      break;

    case kDejitterReChanged:
      updateDejitterReaction();
      break;

    case kDPSpeedChanged:
      myDPaddleSpeed->setValueLabel(myDPaddleSpeed->getValue() * 10);
      break;

    case kDCSpeedChanged:
      myDrivingSpeed->setValueLabel(myDrivingSpeed->getValue() * 10);
      break;

    case kTBSpeedChanged:
      myTrackBallSpeed->setValueLabel(myTrackBallSpeed->getValue() * 10);
      break;

    case kAutoFireChanged:
      updateAutoFireRate();
      break;

    case kDBButtonPressed:
      if(!myJoyDialog)
      {
        const GUI::Font& font = instance().frameBuffer().font();
        myJoyDialog = make_unique<JoystickDialog>
          (this, font, fontWidth() * 56 + 20, fontHeight() * 18 + 20);
      }
      myJoyDialog->show();
      break;

    case kEEButtonPressed:
      if(!myConfirmMsg)
      {
        StringList msg;
        msg.push_back("This operation cannot be undone.");
        msg.push_back("All data stored on your AtariVox");
        msg.push_back("or SaveKey will be erased!");
        msg.push_back("");
        msg.push_back("If you are sure you want to erase");
        msg.push_back("the data, click 'OK', otherwise ");
        msg.push_back("click 'Cancel'.");
        myConfirmMsg = make_unique<GUI::MessageBox>
          (this, instance().frameBuffer().font(), msg,
           myMaxWidth, myMaxHeight, kConfirmEEEraseCmd,
           "OK", "Cancel", "Erase EEPROM", false);
      }
      myConfirmMsg->show();
      break;

    case kConfirmEEEraseCmd:
      eraseEEPROM();
      break;

    case kMouseCtrlChanged:
      handleMouseControlState();
      handleCursorState();
      break;

    case kCursorStateChanged:
      handleCursorState();
      break;

    case kMPSpeedChanged:
      myMPaddleSpeed->setValueLabel(myMPaddleSpeed->getValue() * 10);
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::updateDejitterAveraging()
{
  int strength = myDejitterBase->getValue();

  myDejitterBase->setValueLabel(strength ? std::to_string(strength) : "Off");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::updateDejitterReaction()
{
  int strength = myDejitterDiff->getValue();

  myDejitterDiff->setValueLabel(strength ? std::to_string(strength) : "Off");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::updateAutoFireRate()
{
  int rate = myAutoFireRate->getValue();

  myAutoFireRate->setValueLabel(rate ? std::to_string(rate) : "Off");
  myAutoFireRate->setValueUnit(rate ? " Hz" : "");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleMouseControlState()
{
  bool enable = myMouseControl->getSelected() != 2;

  myMPaddleSpeed->setEnabled(enable);
  myTrackBallSpeed->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleCursorState()
{
  int state = myCursorState->getSelected();
  bool enableGrab = state != 1 && state != 3 && myMouseControl->getSelected() != 2;

  myGrabMouse->setEnabled(enableGrab);
}
