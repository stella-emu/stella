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
// $Id: InputDialog.cxx,v 1.30 2008-03-23 16:22:46 stephena Exp $
//============================================================================

#include "bspf.hxx"

#include "Array.hxx"
#include "EventMappingWidget.hxx"
#include "OSystem.hxx"
#include "Paddles.hxx"
#include "PopUpWidget.hxx"
#include "Settings.hxx"
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
  const StringList& eactions = instance()->eventHandler().getActionList(kEmulationMode);
  myEmulEventMapper = new EventMappingWidget(myTab, font, 2, 2,
                                             myTab->getWidth(),
                                             myTab->getHeight() - ypos,
                                             eactions, kEmulationMode);
  myTab->setParentWidget(tabID, myEmulEventMapper);
  addToFocusList(myEmulEventMapper->getFocusList(), tabID);

  // 2) Event mapper for UI actions
  tabID = myTab->addTab("UI Events");
  const StringList& mactions = instance()->eventHandler().getActionList(kMenuMode);
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
  const int lineHeight = font.getLineHeight();
  int xpos, ypos, lwidth, pwidth, tabID;
  WidgetArray wid;

  // Virtual device/ports
  tabID = myTab->addTab("Virtual Devs");

  // Stelladaptor mappings
  xpos = 5;  ypos = 5;
  lwidth = font.getStringWidth("Stelladaptor 2 is: ");
  pwidth = font.getStringWidth("right virtual port");

  myLeftPort = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                               "Stelladaptor 1 is: ", lwidth, kLeftChanged);
  myLeftPort->appendEntry("left virtual port", 1);
  myLeftPort->appendEntry("right virtual port", 2);
  wid.push_back(myLeftPort);

  ypos += lineHeight + 5;
  myRightPort = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                "Stelladaptor 2 is: ", lwidth, kRightChanged);
  myRightPort->appendEntry("left virtual port", 1);
  myRightPort->appendEntry("right virtual port", 2);
  wid.push_back(myRightPort);

  // Add 'mouse to paddle' mapping
  ypos += 2*lineHeight;
  lwidth = font.getStringWidth("Paddle threshold: ");
  pwidth = font.getMaxCharWidth() * 5;
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

  // Add items for virtual device ports
  addToFocusList(wid, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::loadConfig()
{
  // Left & right ports
  const string& sa1 = instance()->settings().getString("sa1");
  int lport = sa1 == "right" ? 2 : 1;
  myLeftPort->setSelectedTag(lport);
  const string& sa2 = instance()->settings().getString("sa2");
  int rport = sa2 == "right" ? 2 : 1;
  myRightPort->setSelectedTag(rport);

  // Paddle mode
  myPaddleMode->setValue(0);
  myPaddleModeLabel->setLabel("0");

  // Paddle speed
  myPaddleSpeed->setValue(instance()->settings().getInt("pspeed"));
  myPaddleLabel->setLabel(instance()->settings().getString("pspeed"));

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::saveConfig()
{
  // Left & right ports
  string sa1 = myLeftPort->getSelectedTag() == 2 ? "right" : "left";
  string sa2 = myRightPort->getSelectedTag() == 2 ? "right" : "left";
  instance()->eventHandler().mapStelladaptors(sa1, sa2);

  // Paddle mode
  Paddles::setMouseIsPaddle(myPaddleMode->getValue());

  // Paddle speed
  int speed = myPaddleSpeed->getValue();
  instance()->settings().setInt("pspeed", speed);
  Paddles::setDigitalSpeed(speed);
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
      myRightPort->setSelectedTag(
        myLeftPort->getSelectedTag() == 2 ? 1 : 2);
      break;

    case kRightChanged:
      myLeftPort->setSelectedTag(
        myRightPort->getSelectedTag() == 2 ? 1 : 2);
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
