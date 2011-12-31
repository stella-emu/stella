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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "bspf.hxx"

#include "Array.hxx"
#include "OSystem.hxx"
#include "Joystick.hxx"
#include "Paddles.hxx"
#include "Settings.hxx"
#include "StringList.hxx"
#include "EventMappingWidget.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "TabWidget.hxx"
#include "Widget.hxx"

#include "InputDialog.hxx"


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputDialog::InputDialog(OSystem* osystem, DialogContainer* parent,
                         const GUI::Font& font, int max_w, int max_h)
  : Dialog(osystem, parent, 0, 0, 0, 0)
{
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            buttonWidth  = font.getStringWidth("Defaults") + 20,
            buttonHeight = font.getLineHeight() + 4;
  const int vBorder = 4;
  int xpos, ypos, tabID;
  WidgetArray wid;
  StringList actions;

  // Set real dimensions
  _w = BSPF_min(50 * fontWidth + 10, max_w);
  _h = BSPF_min(14 * (lineHeight + 4) + 14, max_h);

  // The tab widget
  xpos = 2; ypos = vBorder;
  myTab = new TabWidget(this, font, xpos, ypos, _w - 2*xpos, _h - buttonHeight - 20);
  addTabWidget(myTab);
  wid.push_back(myTab);
  addToFocusList(wid);

  // 1) Event mapper for emulation actions
  tabID = myTab->addTab("Emul. Events");
  instance().eventHandler().getActionList(kEmulationMode, actions);
  myEmulEventMapper = new EventMappingWidget(myTab, font, 2, 2,
                                             myTab->getWidth(),
                                             myTab->getHeight() - ypos,
                                             actions, kEmulationMode);
  myTab->setParentWidget(tabID, myEmulEventMapper);
  addToFocusList(myEmulEventMapper->getFocusList(), tabID);

  // 2) Event mapper for UI actions
  tabID = myTab->addTab("UI Events");
  actions.clear();
  instance().eventHandler().getActionList(kMenuMode, actions);
  myMenuEventMapper = new EventMappingWidget(myTab, font, 2, 2,
                                             myTab->getWidth(),
                                             myTab->getHeight() - ypos,
                                             actions, kMenuMode);
  myTab->setParentWidget(tabID, myMenuEventMapper);
  addToFocusList(myMenuEventMapper->getFocusList(), tabID);

  // 3) Devices & ports
  addDevicePortTab(font);

  // Finalize the tabs, and activate the first tab
  myTab->activateTabs();
  myTab->setActiveTab(0);

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
  StringMap items;

  // Devices/ports
  tabID = myTab->addTab("Devices & Ports");

  // Stelladaptor mappings
  xpos = 5;  ypos = 5;
  lwidth = font.getStringWidth("Stelladaptor port order: ");
  pwidth = font.getStringWidth("left / right");

  items.clear();
  items.push_back("left / right", "leftright");
  items.push_back("right / left", "rightleft");
  mySAPort = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight, items,
                             "Stelladaptor port order: ", lwidth);
  wid.push_back(mySAPort);

  // Add AtariVox serial port
  ypos += lineHeight + 5;
  lwidth = font.getStringWidth("AVox serial port: ");
  int fwidth = _w - xpos - lwidth - 20;
  new StaticTextWidget(myTab, font, xpos, ypos, lwidth, fontHeight,
                       "AVox serial port: ", kTextAlignLeft);
  myAVoxPort = new EditTextWidget(myTab, font, xpos+lwidth, ypos,
                                  fwidth, fontHeight, "");
  wid.push_back(myAVoxPort);

  lwidth = font.getStringWidth("Digital paddle sensitivity: ");
  pwidth = font.getMaxCharWidth() * 8;

  // Add joystick deadzone setting
  ypos += lineHeight + 8;
  myDeadzone = new SliderWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                "Joystick deadzone size: ", lwidth, kDeadzoneChanged);
  myDeadzone->setMinValue(0); myDeadzone->setMaxValue(29);
  xpos += myDeadzone->getWidth() + 5;
  myDeadzoneLabel = new StaticTextWidget(myTab, font, xpos, ypos+1, 5*fontWidth,
                                         lineHeight, "", kTextAlignLeft);
  myDeadzoneLabel->setFlags(WIDGET_CLEARBG);
  wid.push_back(myDeadzone);

  // Add paddle speed (digital emulation)
  xpos = 5;  ypos += lineHeight + 4;
  myDPaddleSpeed = new SliderWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                   "Digital paddle sensitivity: ",
                                   lwidth, kDPSpeedChanged);
  myDPaddleSpeed->setMinValue(1); myDPaddleSpeed->setMaxValue(10);
  xpos += myDPaddleSpeed->getWidth() + 5;
  myDPaddleLabel = new StaticTextWidget(myTab, font, xpos, ypos+1, 24, lineHeight,
                                        "", kTextAlignLeft);
  myDPaddleLabel->setFlags(WIDGET_CLEARBG);
  wid.push_back(myDPaddleSpeed);

  // Add paddle speed (mouse emulation)
  xpos = 5;  ypos += lineHeight + 4;
  myMPaddleSpeed = new SliderWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                   "Mouse paddle sensitivity: ",
                                   lwidth, kMPSpeedChanged);
  myMPaddleSpeed->setMinValue(1); myMPaddleSpeed->setMaxValue(15);
  xpos += myMPaddleSpeed->getWidth() + 5;
  myMPaddleLabel = new StaticTextWidget(myTab, font, xpos, ypos+1, 24, lineHeight,
                                        "", kTextAlignLeft);
  myMPaddleSpeed->setFlags(WIDGET_CLEARBG);
  wid.push_back(myMPaddleSpeed);

  // Add 'allow all 4 directions' for joystick
  xpos = 10;  ypos += lineHeight + 8;
  myAllowAll4 = new CheckboxWidget(myTab, font, xpos, ypos,
                  "Allow all 4 directions on joystick");
  wid.push_back(myAllowAll4);

  // Grab mouse (in windowed mode)	 
  ypos += lineHeight + 4;
  myGrabMouse = new CheckboxWidget(myTab, font, xpos, ypos,
	                "Grab mouse in emulation mode");
  wid.push_back(myGrabMouse);
#ifndef WINDOWED_SUPPORT
  myGrabMouse->clearFlags(WIDGET_ENABLED);
#endif

  // Mouse is controller type
  ypos += lineHeight + 8;
  lwidth = font.getStringWidth("Use mouse as a controller: ");
  pwidth = font.getStringWidth("Specific axis");
  items.clear();
  items.push_back("Automatic", "auto");
  items.push_back("Specific axis", "paddle");
  myMouseControl =
    new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight, items,
                   "Use mouse as a controller: ", lwidth, kMPCtrlChanged);
  wid.push_back(myMouseControl);

  // Mouse controller specific axis
  lwidth = font.getStringWidth("X-Axis is: ");
  pwidth = font.getStringWidth("Paddle 3");
  items.clear();
  items.push_back("not used", BSPF_toString(Controller::NoControl));
  items.push_back("Paddle 0", BSPF_toString(Controller::Paddle0));
  items.push_back("Paddle 1", BSPF_toString(Controller::Paddle1));
  items.push_back("Paddle 2", BSPF_toString(Controller::Paddle2));
  items.push_back("Paddle 3", BSPF_toString(Controller::Paddle3));

  xpos = 45;  ypos += lineHeight + 4;
  myMouseX = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight, items,
               "X-Axis is: ", lwidth);
  wid.push_back(myMouseX);

  ypos += lineHeight + 4;
  myMouseY = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight, items,
               "Y-Axis is: ", lwidth);
  wid.push_back(myMouseY);

  // Add items for virtual device ports
  addToFocusList(wid, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::loadConfig()
{
  // Left & right ports
  const string& sa1 = instance().settings().getString("sa1");
  int portorder = sa1 == "right" ? 1 : 0;
  mySAPort->setSelected(portorder);

  // Joystick deadzone
  myDeadzone->setValue(instance().settings().getInt("joydeadzone"));
  myDeadzoneLabel->setValue(Joystick::deadzone());

  // Grab mouse
  myGrabMouse->setState(instance().settings().getBool("grabmouse"));

  // Paddle speed (digital and mouse)
  myDPaddleSpeed->setValue(instance().settings().getInt("dsense"));
  myDPaddleLabel->setLabel(instance().settings().getString("dsense"));
  myMPaddleSpeed->setValue(instance().settings().getInt("msense"));
  myMPaddleLabel->setLabel(instance().settings().getString("msense"));

  // AtariVox serial port
  myAVoxPort->setEditString(instance().settings().getString("avoxport"));

  // Allow all 4 joystick directions
  myAllowAll4->setState(instance().settings().getBool("joyallow4"));

  // Mouse is controller type
  const string& mcontrol = instance().settings().getString("mcontrol");
  bool autoAxis = mcontrol == "auto";
  if(autoAxis)
  {
    myMouseControl->setSelected(0);
    myMouseX->setSelected(0);
    myMouseY->setSelected(0);
  }
  else
  {
    myMouseControl->setSelected(1);
    myMouseX->setSelected(BSPF_toString(mcontrol[0] - '0'), "");
    myMouseY->setSelected(BSPF_toString(mcontrol[1] - '0'), "");
  }
  myMouseX->setEnabled(!autoAxis);
  myMouseY->setEnabled(!autoAxis);

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::saveConfig()
{
  // Left & right ports
  int sa_order = mySAPort->getSelected();
  if(sa_order == 0)
    instance().eventHandler().mapStelladaptors("left", "right");
  else
    instance().eventHandler().mapStelladaptors("right", "left");

  // Joystick deadzone
  int deadzone = myDeadzone->getValue();
  instance().settings().setInt("joydeadzone", deadzone);
  Joystick::setDeadZone(deadzone);

  // Grab mouse	 
  instance().settings().setBool("grabmouse", myGrabMouse->getState());	 
  instance().frameBuffer().setCursorState();

  // Paddle speed (digital and mouse)
  int sensitivity = myDPaddleSpeed->getValue();
  instance().settings().setInt("dsense", sensitivity);
  Paddles::setDigitalSensitivity(sensitivity);
  sensitivity = myMPaddleSpeed->getValue();
  instance().settings().setInt("msense", sensitivity);
  Paddles::setMouseSensitivity(sensitivity);

  // AtariVox serial port
  instance().settings().setString("avoxport", myAVoxPort->getEditString());

  // Allow all 4 joystick directions
  bool allowall4 = myAllowAll4->getState();
  instance().settings().setBool("joyallow4", allowall4);
  instance().eventHandler().allowAllDirections(allowall4);

  // Mouse is controller type
  string mcontrol = myMouseControl->getSelectedTag();
  if(mcontrol != "auto")
    mcontrol = myMouseX->getSelectedTag() + myMouseY->getSelectedTag();
  instance().settings().setString("mcontrol", mcontrol);
  instance().eventHandler().setMouseControllerMode(mcontrol);
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
      mySAPort->setSelected("leftright", "leftright");

      // Joystick deadzone
      myDeadzone->setValue(0);
      myDeadzoneLabel->setValue(3200);

      // Grab mouse
      myGrabMouse->setState(true);

      // Paddle speed (digital and mouse)
      myDPaddleSpeed->setValue(5);
      myDPaddleLabel->setLabel("5");
      myMPaddleSpeed->setValue(6);
      myMPaddleLabel->setLabel("6");

      // AtariVox serial port
      myAVoxPort->setEditString("");

      // Allow all 4 joystick directions
      myAllowAll4->setState(false);

      // Mouse is controller type
      myMouseControl->setSelected(0);
      myMouseX->setSelected(0);
      myMouseY->setSelected(0);
      myMouseX->setEnabled(false);
      myMouseY->setEnabled(false);
      break;
    }

    default:
      break;
  }

  _dirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleKeyDown(int ascii, int keycode, int modifiers)
{
  // Remap key events in remap mode, otherwise pass to parent dialog
  if(myEmulEventMapper->remapMode())
    myEmulEventMapper->handleKeyDown(ascii, keycode, modifiers);
  else if(myMenuEventMapper->remapMode())
    myMenuEventMapper->handleKeyDown(ascii, keycode, modifiers);
  else
    Dialog::handleKeyDown(ascii, keycode, modifiers);
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
bool InputDialog::handleJoyHat(int stick, int hat, int value)
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
void InputDialog::handleCommand(CommandSender* sender, int cmd,
                                int data, int id)
{
  switch(cmd)
  {
    case kOKCmd:
      saveConfig();
      close();
      break;

    case kCloseCmd:
      // Revert changes made to event mapping
      close();
      break;

    case kDefaultsCmd:
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

    case kMPCtrlChanged:
    {
      bool state = myMouseControl->getSelectedTag() != "auto";
      myMouseX->setEnabled(state);
      myMouseY->setEnabled(state);
      break;
    }

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
