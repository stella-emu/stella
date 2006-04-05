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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: InputDialog.cxx,v 1.13 2006-04-05 12:28:39 stephena Exp $
//============================================================================

#include "OSystem.hxx"
#include "Widget.hxx"
#include "Array.hxx"
#include "TabWidget.hxx"
#include "EventMappingWidget.hxx"
#include "InputDialog.hxx"
#include "PopUpWidget.hxx"

#include "bspf.hxx"

enum {
  kPaddleChanged       = 'PDch',
  kPaddleThreshChanged = 'PDth',
  kP0SpeedID = 100,
  kP1SpeedID = 101,
  kP2SpeedID = 102,
  kP3SpeedID = 103
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputDialog::InputDialog(OSystem* osystem, DialogContainer* parent,
                         const GUI::Font& font, int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h)
{
  const int vBorder = 4;
  int xpos, ypos, tabID;

  // The tab widget
  xpos = 2; ypos = vBorder;
  myTab = new TabWidget(this, font, xpos, ypos, _w - 2*xpos, _h - 24 - 2*ypos);
  addTabWidget(myTab);

  // 1) Event mapper
  tabID = myTab->addTab("Event Mapping");
  myEventMapper = new EventMappingWidget(myTab, font, 2, 2,
                                         myTab->getWidth(),
                                         myTab->getHeight() - ypos);
  myTab->setParentWidget(tabID, myEventMapper);
  addToFocusList(myEventMapper->getFocusList(), tabID);

  // 2) Virtual device support
  addVDeviceTab(font);

  // Finalize the tabs, and activate the first tab
  myTab->activateTabs();
  myTab->setActiveTab(0);

  // Add OK and Cancel buttons
#ifndef MAC_OSX
  addButton(font, _w - 2 * (kButtonWidth + 7), _h - 24, "OK", kOKCmd, 0);
  addButton(font, _w - (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd, 0);
#else
  addButton(font, _w - 2 * (kButtonWidth + 7), _h - 24, "Cancel", kCloseCmd, 0);
  addButton(font, _w - (kButtonWidth + 10), _h - 24, "OK", kOKCmd, 0);
#endif
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
  tabID = myTab->addTab("Virtual Devices");

  // Stelladaptor mappings
  xpos = 5;  ypos = 5;
  lwidth = font.getStringWidth("Stelladaptor 2 is: ");
  pwidth = font.getStringWidth("right virtual port");

  myLeftPort = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                               "Stelladaptor 1 is: ", lwidth);
  myLeftPort->appendEntry("left virtual port", 1);
  myLeftPort->appendEntry("right virtual port", 2);
  wid.push_back(myLeftPort);

  ypos += lineHeight + 3;
  myRightPort = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                "Stelladaptor 2 is: ", lwidth);
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

  // Add 'paddle threshhold' setting
  xpos = 5;  ypos += lineHeight + 3;
  myPaddleThreshold = new SliderWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                       "Paddle threshold: ",
                                       lwidth, kPaddleThreshChanged);
  myPaddleThreshold->setMinValue(400); myPaddleThreshold->setMaxValue(800);
  xpos += myPaddleThreshold->getWidth() + 5;
  myPaddleThresholdLabel = new StaticTextWidget(myTab, font, xpos, ypos+1,
                                                24, lineHeight,
                                                "", kTextAlignLeft);
  myPaddleThresholdLabel->setFlags(WIDGET_CLEARBG);
  wid.push_back(myPaddleThreshold);

  // Add paddle 0 speed
  xpos = 5;  ypos += lineHeight + 3;
  myPaddleSpeed[0] = new SliderWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                      "Paddle 1 speed: ",
                                      lwidth, kP0SpeedID);
  myPaddleSpeed[0]->setMinValue(1); myPaddleSpeed[0]->setMaxValue(100);
  xpos += myPaddleSpeed[0]->getWidth() + 5;
  myPaddleLabel[0] = new StaticTextWidget(myTab, font, xpos, ypos+1, 24, lineHeight,
                                          "", kTextAlignLeft);
  myPaddleLabel[0]->setFlags(WIDGET_CLEARBG);
  wid.push_back(myPaddleSpeed[0]);

  // Add paddle 1 speed
  xpos = 5;  ypos += lineHeight + 3;
  myPaddleSpeed[1] = new SliderWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                      "Paddle 2 speed: ",
                                      lwidth, kP1SpeedID);
  myPaddleSpeed[1]->setMinValue(1); myPaddleSpeed[1]->setMaxValue(100);
  xpos += myPaddleSpeed[1]->getWidth() + 5;
  myPaddleLabel[1] = new StaticTextWidget(myTab, font, xpos, ypos+1, 24, lineHeight,
                                          "", kTextAlignLeft);
  myPaddleLabel[1]->setFlags(WIDGET_CLEARBG);
  wid.push_back(myPaddleSpeed[1]);

  // Add paddle 2 speed
  xpos = 5;  ypos += lineHeight + 3;
  myPaddleSpeed[2] = new SliderWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                      "Paddle 3 speed: ",
                                      lwidth, kP2SpeedID);
  myPaddleSpeed[2]->setMinValue(1); myPaddleSpeed[2]->setMaxValue(100);
  xpos += myPaddleSpeed[2]->getWidth() + 5;
  myPaddleLabel[2] = new StaticTextWidget(myTab, font, xpos, ypos+1, 24, lineHeight,
                                        "", kTextAlignLeft);
  myPaddleLabel[2]->setFlags(WIDGET_CLEARBG);
  wid.push_back(myPaddleSpeed[2]);

  // Add paddle 3 speed
  xpos = 5;  ypos += lineHeight + 3;
  myPaddleSpeed[3] = new SliderWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                      "Paddle 4 speed: ",
                                      lwidth, kP3SpeedID);
  myPaddleSpeed[3]->setMinValue(1); myPaddleSpeed[3]->setMaxValue(100);
  xpos += myPaddleSpeed[3]->getWidth() + 5;
  myPaddleLabel[3] = new StaticTextWidget(myTab, font, xpos, ypos+1, 24, lineHeight,
                                        "", kTextAlignLeft);
  myPaddleLabel[3]->setFlags(WIDGET_CLEARBG);
  wid.push_back(myPaddleSpeed[3]);

  // Add items for virtual device ports
  addToFocusList(wid, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::loadConfig()
{
  myEventMapper->loadConfig();

  // Left & right ports
  const string& sa1 = instance()->settings().getString("sa1");
  int lport = sa1 == "right" ? 2 : 1;
  myLeftPort->setSelectedTag(lport);
  const string& sa2 = instance()->settings().getString("sa2");
  int rport = sa2 == "right" ? 2 : 1;
  myRightPort->setSelectedTag(rport);

  // Paddle mode
  myPaddleMode->setValue(instance()->settings().getInt("paddle"));
  myPaddleModeLabel->setLabel(instance()->settings().getString("paddle"));

  // Paddle threshold
  myPaddleThreshold->setValue(instance()->settings().getInt("pthresh"));
  myPaddleThresholdLabel->setLabel(instance()->settings().getString("pthresh"));

  // Paddle speed settings
  myPaddleSpeed[0]->setValue(instance()->settings().getInt("p1speed"));
  myPaddleLabel[0]->setLabel(instance()->settings().getString("p1speed"));
  myPaddleSpeed[1]->setValue(instance()->settings().getInt("p2speed"));
  myPaddleLabel[1]->setLabel(instance()->settings().getString("p2speed"));
  myPaddleSpeed[2]->setValue(instance()->settings().getInt("p3speed"));
  myPaddleLabel[2]->setLabel(instance()->settings().getString("p3speed"));
  myPaddleSpeed[3]->setValue(instance()->settings().getInt("p4speed"));
  myPaddleLabel[3]->setLabel(instance()->settings().getString("p4speed"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::saveConfig()
{
  // Left & right ports
  string sa1 = myLeftPort->getSelectedTag() == 2 ? "right" : "left";
  string sa2 = myRightPort->getSelectedTag() == 2 ? "right" : "left";
  instance()->eventHandler().mapStelladaptors(sa1, sa2);

  // Paddle mode
  int mode = myPaddleMode->getValue();
  instance()->eventHandler().setPaddleMode(mode);

  // Paddle threshold
  int threshold = myPaddleThreshold->getValue();
  instance()->eventHandler().setPaddleThreshold(threshold);

  // Paddle speed settings
  for(int i = 0; i < 4; ++i)
    instance()->eventHandler().setPaddleSpeed(i, myPaddleSpeed[i]->getValue());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleKeyDown(int ascii, int keycode, int modifiers)
{
  // Remap key events in remap mode, otherwise pass to listwidget
  if(myEventMapper->remapMode())
    myEventMapper->handleKeyDown(ascii, keycode, modifiers);
  else
    Dialog::handleKeyDown(ascii, keycode, modifiers);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleJoyDown(int stick, int button)
{
  // Remap joystick buttons in remap mode, otherwise pass to listwidget
  if(myEventMapper->remapMode())
    myEventMapper->handleJoyDown(stick, button);
  else
    Dialog::handleJoyDown(stick, button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleJoyAxis(int stick, int axis, int value)
{
  // Remap joystick axis in remap mode, otherwise pass to listwidget
  if(myEventMapper->remapMode())
    myEventMapper->handleJoyAxis(stick, axis, value);
  else
    Dialog::handleJoyAxis(stick, axis, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool InputDialog::handleJoyHat(int stick, int hat, int value)
{
  // Remap joystick hat in remap mode, otherwise pass to listwidget
  if(myEventMapper->remapMode())
    return myEventMapper->handleJoyHat(stick, hat, value);
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

    case kPaddleChanged:
      myPaddleModeLabel->setValue(myPaddleMode->getValue());
      break;

    case kPaddleThreshChanged:
      myPaddleThresholdLabel->setValue(myPaddleThreshold->getValue());
      break;

    case kP0SpeedID:
    case kP1SpeedID:
    case kP2SpeedID:
    case kP3SpeedID:
      myPaddleLabel[cmd-100]->setValue(myPaddleSpeed[cmd-100]->getValue());
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
