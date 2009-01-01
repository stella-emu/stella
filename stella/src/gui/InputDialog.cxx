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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: InputDialog.cxx,v 1.35 2009-01-01 18:13:38 stephena Exp $
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
                         const GUI::Font& font, int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h)
{
  const int buttonHeight = font.getLineHeight() + 4;
  const int vBorder = 4;
  int xpos, ypos, tabID;
  WidgetArray wid;

  // Set real dimensions
//  _w = 42 * fontWidth + 10;
//  _h = 12 * (lineHeight + 4) + 10;

  // The tab widget
  xpos = 2; ypos = vBorder;
  myTab = new TabWidget(this, font, xpos, ypos, _w - 2*xpos, _h - buttonHeight - 20);
  addTabWidget(myTab);
  wid.push_back(myTab);
  addToFocusList(wid);

  // 1) Event mapper for emulation actions
  tabID = myTab->addTab("Emul. Events");
  const StringList& eactions = instance().eventHandler().getActionList(kEmulationMode);
  myEmulEventMapper = new EventMappingWidget(myTab, font, 2, 2,
                                             myTab->getWidth(),
                                             myTab->getHeight() - ypos,
                                             eactions, kEmulationMode);
  myTab->setParentWidget(tabID, myEmulEventMapper);
  addToFocusList(myEmulEventMapper->getFocusList(), tabID);

  // 2) Event mapper for UI actions
  tabID = myTab->addTab("UI Events");
  const StringList& mactions = instance().eventHandler().getActionList(kMenuMode);
  myMenuEventMapper = new EventMappingWidget(myTab, font, 2, 2,
                                             myTab->getWidth(),
                                             myTab->getHeight() - ypos,
                                             mactions, kMenuMode);
  myTab->setParentWidget(tabID, myMenuEventMapper);
  addToFocusList(myMenuEventMapper->getFocusList(), tabID);

  // 3) Virtual device support
  addVDeviceTab(font);

  // Finalize the tabs, and activate the first tab
  myTab->activateTabs();
  myTab->setActiveTab(0);

  // Add OK and Cancel buttons
  wid.clear();
  addOKCancelBGroup(wid, font);
  addBGroupToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputDialog::~InputDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::addVDeviceTab(const GUI::Font& font)
{
  const int lineHeight = font.getLineHeight(),
            fontHeight = font.getFontHeight();
  int xpos, ypos, lwidth, pwidth, tabID;
  WidgetArray wid;
  StringMap items;

  // Virtual device/ports
  tabID = myTab->addTab("Virtual Devs");

  // Stelladaptor mappings
  xpos = 5;  ypos = 5;
  lwidth = font.getStringWidth("Stelladaptor 2 is: ");
  pwidth = font.getStringWidth("right virtual port");

  items.clear();
  items.push_back("left virtual port", "left");
  items.push_back("right virtual port", "right");
  myLeftPort = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight, items,
                               "Stelladaptor 1 is: ", lwidth, kLeftChanged);
  wid.push_back(myLeftPort);

  ypos += lineHeight + 5;
  // ... use items from above
  myRightPort = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight, items,
                                "Stelladaptor 2 is: ", lwidth, kRightChanged);
  wid.push_back(myRightPort);

  lwidth = font.getStringWidth("Paddle threshold: ");
  pwidth = font.getMaxCharWidth() * 5;

  // Add joystick deadzone setting
  ypos += 2*lineHeight;
  myDeadzone = new SliderWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                "Joy deadzone: ", lwidth, kDeadzoneChanged);
  myDeadzone->setMinValue(0); myDeadzone->setMaxValue(29);
  xpos += myDeadzone->getWidth() + 5;
  myDeadzoneLabel = new StaticTextWidget(myTab, font, xpos, ypos+1, 24, lineHeight,
                                         "", kTextAlignLeft);
  myDeadzoneLabel->setFlags(WIDGET_CLEARBG);
  wid.push_back(myDeadzone);

  // Add 'mouse to paddle' mapping
  xpos = 5;  ypos += lineHeight + 3;
  myPaddleMode = new SliderWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                  "Mouse is paddle: ", lwidth, kPaddleChanged);
  myPaddleMode->setMinValue(0); myPaddleMode->setMaxValue(3);
  xpos += myPaddleMode->getWidth() + 5;
  myPaddleModeLabel = new StaticTextWidget(myTab, font, xpos, ypos+1, 24, lineHeight,
                                           "", kTextAlignLeft);
  myPaddleModeLabel->setFlags(WIDGET_CLEARBG);
  wid.push_back(myPaddleMode);

  // Add paddle speed
  xpos = 5;  ypos += lineHeight + 3;
  myPaddleSpeed = new SliderWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                   "Paddle speed: ",
                                   lwidth, kPSpeedChanged);
  myPaddleSpeed->setMinValue(1); myPaddleSpeed->setMaxValue(15);
  xpos += myPaddleSpeed->getWidth() + 5;
  myPaddleLabel = new StaticTextWidget(myTab, font, xpos, ypos+1, 24, lineHeight,
                                       "", kTextAlignLeft);
  myPaddleLabel->setFlags(WIDGET_CLEARBG);
  wid.push_back(myPaddleSpeed);

  // Add AtariVox serial port
  xpos = 5;  ypos += 2*lineHeight;
  int fwidth = _w - xpos - lwidth - 20;
  new StaticTextWidget(myTab, font, xpos, ypos, lwidth, fontHeight,
                       "AVox serial port:", kTextAlignLeft);
  myAVoxPort = new EditTextWidget(myTab, font, xpos+lwidth, ypos,
                                  fwidth, fontHeight, "");
  wid.push_back(myAVoxPort);

  // Add items for virtual device ports
  addToFocusList(wid, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::loadConfig()
{
  // Left & right ports
  const string& sa1 = instance().settings().getString("sa1");
  int lport = sa1 == "right" ? 1 : 0;
  myLeftPort->setSelected(lport);
  const string& sa2 = instance().settings().getString("sa2");
  int rport = sa2 == "right" ? 1 : 0;
  myRightPort->setSelected(rport);

  // Joystick deadzone
  myDeadzone->setValue(instance().settings().getInt("joydeadzone"));
  myDeadzoneLabel->setLabel(instance().settings().getString("joydeadzone"));

  // Paddle mode
  myPaddleMode->setValue(0);
  myPaddleModeLabel->setLabel("0");

  // Paddle speed
  myPaddleSpeed->setValue(instance().settings().getInt("pspeed"));
  myPaddleLabel->setLabel(instance().settings().getString("pspeed"));

  // AtariVox serial port
  myAVoxPort->setEditString(instance().settings().getString("avoxport"));

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::saveConfig()
{
  // Left & right ports
  const string& sa1 = myLeftPort->getSelectedTag();
  const string& sa2 = myRightPort->getSelectedTag();
  instance().eventHandler().mapStelladaptors(sa1, sa2);

  // Joystick deadzone
  int deadzone = myDeadzone->getValue();
  instance().settings().setInt("joydeadzone", deadzone);
  Joystick::setDeadZone(deadzone);

  // Paddle mode
  Paddles::setMouseIsPaddle(myPaddleMode->getValue());

  // Paddle speed
  int speed = myPaddleSpeed->getValue();
  instance().settings().setInt("pspeed", speed);
  Paddles::setDigitalSpeed(speed);

  // AtariVox serial port
  instance().settings().setString("avoxport", myAVoxPort->getEditString());
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

    case kLeftChanged:
      myRightPort->setSelected(
        myLeftPort->getSelected() == 1 ? 0 : 1);
      break;

    case kRightChanged:
      myLeftPort->setSelected(
        myRightPort->getSelected() == 1 ? 0 : 1);
      break;

    case kDeadzoneChanged:
      myDeadzoneLabel->setValue(myDeadzone->getValue());
      break;

    case kPaddleChanged:
      myPaddleModeLabel->setValue(myPaddleMode->getValue());
      break;

    case kPSpeedChanged:
      myPaddleLabel->setValue(myPaddleSpeed->getValue());
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
