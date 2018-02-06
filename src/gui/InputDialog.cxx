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

#include "bspf.hxx"
#include "OSystem.hxx"
#include "Console.hxx"
#include "EventHandler.hxx"
#include "Joystick.hxx"
#include "Paddles.hxx"
#include "PointingDevice.hxx"
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
#include "InputDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputDialog::InputDialog(OSystem& osystem, DialogContainer& parent,
                         const GUI::Font& font, int max_w, int max_h)
  : Dialog(osystem, parent, font, "Input settings"),
    myConfirmMsg(nullptr),
    myMaxWidth(max_w),
    myMaxHeight(max_h)
{
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            buttonHeight = font.getLineHeight() + 4;
  const int vBorder = 4;
  int xpos, ypos, tabID;
  StringList actions;

  // Set real dimensions
  _w = std::min(50 * fontWidth + 10, max_w);
  _h = std::min(16 * (lineHeight + 4) + 16 + _th, max_h);

  // The tab widget
  xpos = 2; ypos = vBorder + _th;
  myTab = new TabWidget(this, font, xpos, ypos, _w - 2*xpos, _h -_th - buttonHeight - 20);
  addTabWidget(myTab);

  // 1) Event mapper for emulation actions
  tabID = myTab->addTab("Emul. Events");
  actions = instance().eventHandler().getActionList(kEmulationMode);
  myEmulEventMapper = new EventMappingWidget(myTab, font, 2, 2,
                                             myTab->getWidth(),
                                             myTab->getHeight() - 4,
                                             actions, kEmulationMode);
  myTab->setParentWidget(tabID, myEmulEventMapper);
  addToFocusList(myEmulEventMapper->getFocusList(), myTab, tabID);

  // 2) Event mapper for UI actions
  tabID = myTab->addTab("UI Events");
  actions = instance().eventHandler().getActionList(kMenuMode);
  myMenuEventMapper = new EventMappingWidget(myTab, font, 2, 2,
                                             myTab->getWidth(),
                                             myTab->getHeight() - 4,
                                             actions, kMenuMode);
  myTab->setParentWidget(tabID, myMenuEventMapper);
  addToFocusList(myMenuEventMapper->getFocusList(), myTab, tabID);

  // 3) Devices & ports
  addDevicePortTab(font);

  // Finalize the tabs, and activate the first tab
  myTab->activateTabs();
  myTab->setActiveTab(0);

  // Add Defaults, OK and Cancel buttons
  WidgetArray wid;
  addDefaultsOKCancelBGroup(wid, font);
  addBGroupToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputDialog::~InputDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::addDevicePortTab(const GUI::Font& font)
{
  const int lineHeight = font.getLineHeight(),
            fontWidth  = font.getMaxCharWidth(),
            fontHeight = font.getFontHeight();
  int xpos, ypos, lwidth, pwidth, tabID;
  WidgetArray wid;
  VariantList items;
  const int VGAP = 4;
  const int VBORDER = 9;
  const int HBORDER = 8;

  // Devices/ports
  tabID = myTab->addTab("Devices & Ports");

  ypos = VBORDER;
  lwidth = font.getStringWidth("Digital paddle sensitivity "); // was: "Use mouse as a controller "
  pwidth = font.getStringWidth("-UI, -Emulation");

  // Use mouse as controller
  items.clear();
  VarList::push_back(items, "Always", "always");
  VarList::push_back(items, "Analog devices", "analog");
  VarList::push_back(items, "Never", "never");
  myMouseControl = new PopUpWidget(myTab, font, HBORDER, ypos, pwidth, lineHeight, items,
                                   "Use mouse as a controller ", lwidth);
  wid.push_back(myMouseControl);

  // Mouse cursor state
  ypos += lineHeight + VGAP;
  items.clear();
  VarList::push_back(items, "-UI, -Emulation", "0");
  VarList::push_back(items, "-UI, +Emulation", "1");
  VarList::push_back(items, "+UI, -Emulation", "2");
  VarList::push_back(items, "+UI, +Emulation", "3");
  myCursorState = new PopUpWidget(myTab, font, HBORDER, ypos, pwidth, lineHeight, items,
                                  "Mouse cursor visibility ", lwidth);
  wid.push_back(myCursorState);
#ifndef WINDOWED_SUPPORT
  myCursorState->clearFlags(WIDGET_ENABLED);
#endif

  lwidth = font.getStringWidth("Digital paddle sensitivity ");
  pwidth = font.getMaxCharWidth() * 8;

  // Add joystick deadzone setting
  ypos += lineHeight + VGAP*3;
  myDeadzone = new SliderWidget(myTab, font, HBORDER, ypos,
                                "Joystick deadzone size ", lwidth, kDeadzoneChanged);
  myDeadzone->setMinValue(0); myDeadzone->setMaxValue(29);
  xpos = HBORDER + myDeadzone->getWidth() + 5;
  myDeadzoneLabel = new StaticTextWidget(myTab, font, xpos, ypos+1, 5*fontWidth, lineHeight, "");
  wid.push_back(myDeadzone);

  // Add paddle speed (digital emulation)
  ypos += lineHeight + VGAP;
  myDPaddleSpeed = new SliderWidget(myTab, font, HBORDER, ypos,
                                    "Digital paddle sensitivity ",
                                    lwidth, kDPSpeedChanged);
  myDPaddleSpeed->setMinValue(1); myDPaddleSpeed->setMaxValue(20);
  xpos = HBORDER + myDPaddleSpeed->getWidth() + 5;
  myDPaddleLabel = new StaticTextWidget(myTab, font, xpos, ypos+1, 24, lineHeight, "");
  wid.push_back(myDPaddleSpeed);

  // Add paddle speed (mouse emulation)
  ypos += lineHeight + VGAP;
  myMPaddleSpeed = new SliderWidget(myTab, font, HBORDER, ypos,
                                    "Mouse paddle sensitivity ",
                                    lwidth, kMPSpeedChanged);
  myMPaddleSpeed->setMinValue(1); myMPaddleSpeed->setMaxValue(20);
  xpos = HBORDER + myMPaddleSpeed->getWidth() + 5;
  myMPaddleLabel = new StaticTextWidget(myTab, font, xpos, ypos+1, 24, lineHeight, "");
  wid.push_back(myMPaddleSpeed);

  // Add trackball speed
  ypos += lineHeight + VGAP;
  myTrackBallSpeed = new SliderWidget(myTab, font, HBORDER, ypos,
                                      "Trackball sensitivity ",
                                      lwidth, kTBSpeedChanged);
  myTrackBallSpeed->setMinValue(1); myTrackBallSpeed->setMaxValue(20);
  xpos = HBORDER + myTrackBallSpeed->getWidth() + 5;
  myTrackBallLabel = new StaticTextWidget(myTab, font, xpos, ypos+1, 24, lineHeight, "");
  wid.push_back(myTrackBallSpeed);

  // Add 'allow all 4 directions' for joystick
  ypos += lineHeight + VGAP*3;
  myAllowAll4 = new CheckboxWidget(myTab, font, HBORDER, ypos,
                  "Allow all 4 directions on joystick");
  wid.push_back(myAllowAll4);

  // Grab mouse (in windowed mode)
  ypos += lineHeight + VGAP;
  myGrabMouse = new CheckboxWidget(myTab, font, HBORDER, ypos,
	                "Grab mouse in emulation mode");
  wid.push_back(myGrabMouse);
#ifndef WINDOWED_SUPPORT
  myGrabMouse->clearFlags(WIDGET_ENABLED);
#endif

  // Enable/disable control key-combos
  ypos += lineHeight + VGAP;
  myCtrlCombo = new CheckboxWidget(myTab, font, HBORDER, ypos,
	                "Use Control key combos");
  wid.push_back(myCtrlCombo);
  ypos += lineHeight + VGAP;

  // Stelladaptor mappings
  mySAPort = new CheckboxWidget(myTab, font, HBORDER, ypos,
                                "Swap Stelladaptor ports");
  wid.push_back(mySAPort);

  int fwidth;

  // Add EEPROM erase (part 1/2)
  ypos += VGAP*4;
  fwidth = font.getStringWidth("AtariVox/SaveKey");
  lwidth = font.getStringWidth("AtariVox/SaveKey");
  new StaticTextWidget(myTab, font, _w - HBORDER - 4 - (fwidth + lwidth) / 2, ypos,
                       "AtariVox/SaveKey");

  // Show joystick database
  ypos += lineHeight;
  myJoyDlgButton = new ButtonWidget(myTab, font, HBORDER, ypos, 20,
    "Joystick Database" + ELLIPSIS, kDBButtonPressed);
  wid.push_back(myJoyDlgButton);

  // Add EEPROM erase (part 1/2)
  myEraseEEPROMButton = new ButtonWidget(myTab, font, _w - HBORDER - 4 - fwidth, ypos,
                                         fwidth, lineHeight+4,
                                        "Erase EEPROM", kEEButtonPressed);
  wid.push_back(myEraseEEPROMButton);

  // Add AtariVox serial port
  ypos += lineHeight + VGAP*2;
  lwidth = font.getStringWidth("AVox serial port ");
  fwidth = _w - HBORDER * 2 - 4 - lwidth;
  new StaticTextWidget(myTab, font, HBORDER, ypos + 2, "AVox serial port ");
  myAVoxPort = new EditTextWidget(myTab, font, HBORDER + lwidth, ypos,
                                  fwidth, fontHeight);

  wid.push_back(myAVoxPort);

  // Add items for virtual device ports
  addToFocusList(wid, myTab, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::loadConfig()
{
  // Left & right ports
  mySAPort->setState(instance().settings().getString("saport") == "rl");

  // Use mouse as a controller
  myMouseControl->setSelected(
    instance().settings().getString("usemouse"), "analog");

  // Mouse cursor state
  myCursorState->setSelected(instance().settings().getString("cursor"), "2");

  // Joystick deadzone
  myDeadzone->setValue(instance().settings().getInt("joydeadzone"));
  myDeadzoneLabel->setValue(Joystick::deadzone());

  // Paddle speed (digital and mouse)
  myDPaddleSpeed->setValue(instance().settings().getInt("dsense"));
  myDPaddleLabel->setLabel(instance().settings().getString("dsense"));
  myMPaddleSpeed->setValue(instance().settings().getInt("msense"));
  myMPaddleLabel->setLabel(instance().settings().getString("msense"));

  // Trackball speed
  myTrackBallSpeed->setValue(instance().settings().getInt("tsense"));
  myTrackBallLabel->setLabel(instance().settings().getString("tsense"));

  // AtariVox serial port
  myAVoxPort->setText(instance().settings().getString("avoxport"));

  // EEPROM erase (only enable in emulation mode and for valid controllers)
  if(instance().hasConsole())
  {
    Controller& lport = instance().console().leftController();
    Controller& rport = instance().console().rightController();

    myEraseEEPROMButton->setEnabled(lport.type() == Controller::SaveKey || lport.type() == Controller::AtariVox ||
                                    rport.type() == Controller::SaveKey || rport.type() == Controller::AtariVox);
  }
  else
    myEraseEEPROMButton->setEnabled(false);

  // Allow all 4 joystick directions
  myAllowAll4->setState(instance().settings().getBool("joyallow4"));

  // Grab mouse
  myGrabMouse->setState(instance().settings().getBool("grabmouse"));

  // Enable/disable control key-combos
  myCtrlCombo->setState(instance().settings().getBool("ctrlcombo"));

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::saveConfig()
{
  // Left & right ports
  instance().eventHandler().mapStelladaptors(mySAPort->getState() ? "rl": "lr");

  // Use mouse as a controller
  const string& usemouse = myMouseControl->getSelectedTag().toString();
  instance().settings().setValue("usemouse", usemouse);
  instance().eventHandler().setMouseControllerMode(usemouse);

  // Joystick deadzone
  int deadzone = myDeadzone->getValue();
  instance().settings().setValue("joydeadzone", deadzone);
  Joystick::setDeadZone(deadzone);

  // Paddle speed (digital and mouse)
  int sensitivity = myDPaddleSpeed->getValue();
  instance().settings().setValue("dsense", sensitivity);
  Paddles::setDigitalSensitivity(sensitivity);
  sensitivity = myMPaddleSpeed->getValue();
  instance().settings().setValue("msense", sensitivity);
  Paddles::setMouseSensitivity(sensitivity);

  // Trackball speed
  sensitivity = myTrackBallSpeed->getValue();
  instance().settings().setValue("tsense", sensitivity);
  PointingDevice::setSensitivity(sensitivity);

  // AtariVox serial port
  instance().settings().setValue("avoxport", myAVoxPort->getText());

  // Allow all 4 joystick directions
  bool allowall4 = myAllowAll4->getState();
  instance().settings().setValue("joyallow4", allowall4);
  instance().eventHandler().allowAllDirections(allowall4);

  // Grab mouse and hide cursor
  const string& cursor = myCursorState->getSelectedTag().toString();
  instance().settings().setValue("cursor", cursor);
  instance().settings().setValue("grabmouse", myGrabMouse->getState());
  instance().frameBuffer().enableGrabMouse(myGrabMouse->getState());

  // Enable/disable control key-combos
  instance().settings().setValue("ctrlcombo", myCtrlCombo->getState());
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

    case 2:  // Virtual devices
    {
      // Left & right ports
      mySAPort->setState(false);

      // Use mouse as a controller
      myMouseControl->setSelected("analog");

      // Mouse cursor state
      myCursorState->setSelected("2");

      // Joystick deadzone
      myDeadzone->setValue(0);
      myDeadzoneLabel->setValue(3200);

      // Paddle speed (digital and mouse)
      myDPaddleSpeed->setValue(10);
      myDPaddleLabel->setLabel("10");
      myMPaddleSpeed->setValue(10);
      myMPaddleLabel->setLabel("10");
      myTrackBallSpeed->setValue(10);
      myTrackBallLabel->setLabel("10");

      // AtariVox serial port
      myAVoxPort->setText("");

      // Allow all 4 joystick directions
      myAllowAll4->setState(false);

      // Grab mouse
      myGrabMouse->setState(true);

      // Enable/disable control key-combos
      myCtrlCombo->setState(true);

      break;
    }

    default:
      break;
  }

  _dirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleKeyDown(StellaKey key, StellaMod mod)
{
  // Remap key events in remap mode, otherwise pass to parent dialog
  if(myEmulEventMapper->remapMode())
    myEmulEventMapper->handleKeyDown(key, mod);
  else if(myMenuEventMapper->remapMode())
    myMenuEventMapper->handleKeyDown(key, mod);
  else
    Dialog::handleKeyDown(key, mod);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleJoyDown(int stick, int button)
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
void InputDialog::handleJoyAxis(int stick, int axis, int value)
{
  // Remap joystick axis in remap mode, otherwise pass to parent dialog
  if(myEmulEventMapper->remapMode())
    myEmulEventMapper->handleJoyAxis(stick, axis, value);
  else if(myMenuEventMapper->remapMode())
    myMenuEventMapper->handleJoyAxis(stick, axis, value);
  else
    Dialog::handleJoyAxis(stick, axis, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool InputDialog::handleJoyHat(int stick, int hat, JoyHat value)
{
  // Remap joystick hat in remap mode, otherwise pass to parent dialog
  if(myEmulEventMapper->remapMode())
    return myEmulEventMapper->handleJoyHat(stick, hat, value);
  else if(myMenuEventMapper->remapMode())
    return myMenuEventMapper->handleJoyHat(stick, hat, value);
  else
    return Dialog::handleJoyHat(stick, hat, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::eraseEEPROM()
{
  // This method will only be callable if a console exists, so we don't
  // need to check again here
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
      myDeadzoneLabel->setValue(3200 + 1000*myDeadzone->getValue());
      break;

    case kDPSpeedChanged:
      myDPaddleLabel->setValue(myDPaddleSpeed->getValue());
      break;

    case kMPSpeedChanged:
      myMPaddleLabel->setValue(myMPaddleSpeed->getValue());
      break;

    case kTBSpeedChanged:
      myTrackBallLabel->setValue(myTrackBallSpeed->getValue());
      break;

    case kDBButtonPressed:
      if(!myJoyDialog)
      {
        const GUI::Font& font = instance().frameBuffer().font();
        myJoyDialog = make_unique<JoystickDialog>
          (this, font, font.getMaxCharWidth() * 56 + 20, font.getFontHeight() * 18 + 20);
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

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
