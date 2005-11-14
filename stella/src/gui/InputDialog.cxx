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
// $Id: InputDialog.cxx,v 1.2 2005-11-14 17:01:19 stephena Exp $
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
  kPaddleChanged = 'PDch',
  kSenseChanged  = 'PSch'
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputDialog::InputDialog(
      OSystem* osystem, DialogContainer* parent,
      int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h)
{
  const int vBorder = 4;
  int xpos, ypos, tabID;

  // The tab widget
  xpos = 2; ypos = vBorder;
  myTab = new TabWidget(this, xpos, ypos, _w - 2*xpos, _h - 24 - 2*ypos);
  addTabWidget(myTab);

  // 1) Event mapper
  tabID = myTab->addTab("Event Mapping");
  myEventMapper = new EventMappingWidget(myTab, 2, 2,
                        myTab->getWidth(),
                        myTab->getHeight() - ypos);
  myTab->setParentWidget(tabID, myEventMapper);
  addToFocusList(myEventMapper->getFocusList(), tabID);

  // 2) Virtual device support
  addVDeviceTab();

  // Activate the first tab
  myTab->setActiveTab(0);

  // Add OK and Cancel buttons
#ifndef MAC_OSX
  addButton(_w - 2 * (kButtonWidth + 7), _h - 24, "OK", kOKCmd, 0);
  addButton(_w - (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd, 0);
#else
  addButton(_w - 2 * (kButtonWidth + 7), _h - 24, "Cancel", kCloseCmd, 0);
  addButton(_w - (kButtonWidth + 10), _h - 24, "OK", kOKCmd, 0);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputDialog::~InputDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::addVDeviceTab()
{
  const GUI::Font& font = instance()->font();
  const int fontHeight = font.getFontHeight(),
            lineHeight = font.getLineHeight();
  const StringList& joynames = instance()->eventHandler().joystickNames();

  WidgetArray wid;
  int xpos, ypos, lwidth, fwidth, tabID;

  // Virtual device/ports
  tabID = myTab->addTab("Virtual Devices");

  // Leftport and rightport commandline arguments
  xpos = 5;  ypos = 5;
  lwidth = font.getStringWidth("Right port: ");
  fwidth = _w - xpos - lwidth - 10;
  new StaticTextWidget(myTab, xpos, ypos+1, lwidth, fontHeight,
                       "Left Port:", kTextAlignLeft);
  myLeftPort = new PopUpWidget(myTab, xpos+lwidth, ypos,
                               fwidth, lineHeight, "", 0, 0);
  myLeftPort->appendEntry("None", 0);
  for(unsigned int i = 0; i < joynames.size(); ++i)
    myLeftPort->appendEntry(joynames[i], i+1);
  wid.push_back(myLeftPort);

  ypos += lineHeight + 3;
  new StaticTextWidget(myTab, xpos, ypos+1, lwidth, fontHeight,
                       "Right Port:", kTextAlignLeft);
  myRightPort = new PopUpWidget(myTab, xpos+lwidth, ypos,
                                fwidth, lineHeight, "", 0, 0);
  myRightPort->appendEntry("None", 0);
  for(unsigned int i = 0; i < joynames.size(); ++i)
    myRightPort->appendEntry(joynames[i], i+1);
  wid.push_back(myRightPort);

  // Add 'mouse to paddle' mapping
  ypos += 2*lineHeight + 3;
  lwidth = font.getStringWidth("Mouse sensitivity: ");
  myPaddleMode = new SliderWidget(myTab, xpos, ypos, lwidth + 30, lineHeight,
                                  "Mouse is paddle: ",
                                  lwidth, kPaddleChanged);
  myPaddleMode->setMinValue(0); myPaddleMode->setMaxValue(3);
  xpos += myPaddleMode->getWidth() + 5;
  myPaddleLabel = new StaticTextWidget(myTab, xpos, ypos+1, 24, lineHeight,
                                       "", kTextAlignLeft);
  myPaddleLabel->setFlags(WIDGET_CLEARBG);
  wid.push_back(myPaddleMode);

  // Add mouse sensitivity
  xpos = 5;  ypos += lineHeight + 3;
  myPaddleSense = new SliderWidget(myTab, xpos, ypos, lwidth + 30, lineHeight,
                                   "Mouse sensitivity: ",
                                   lwidth, kSenseChanged);
  myPaddleSense->setMinValue(1); myPaddleSense->setMaxValue(100);
  xpos += myPaddleSense->getWidth() + 5;
  mySenseLabel = new StaticTextWidget(myTab, xpos, ypos+1, 24, lineHeight,
                                      "", kTextAlignLeft);
  mySenseLabel->setFlags(WIDGET_CLEARBG);
  wid.push_back(myPaddleSense);

  // Add items for virtual device ports
  addToFocusList(wid, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::loadConfig()
{
  // Left & right ports
  int lport = instance()->settings().getInt("leftport") + 1;
  myLeftPort->setSelectedTag(lport);
  int rport = instance()->settings().getInt("rightport") + 1;
  myRightPort->setSelectedTag(rport);

  // Paddle mode
  myPaddleMode->setValue(instance()->settings().getInt("paddle"));
  myPaddleLabel->setLabel(instance()->settings().getString("paddle"));

/*  FIXME - add this to eventhandler core
  // Paddle sensitivity
  myPaddleSense->setValue(instance()->settings().getInt("paddle"));
  mySenseLabel->setLabel(instance()->settings().getString("paddle"));
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::saveConfig()
{
  // Left & right ports
  int lport = myLeftPort->getSelectedTag() - 1;
  int rport = myRightPort->getSelectedTag() - 1;
  instance()->eventHandler().mapJoysticks(lport, rport);

  // Paddle mode
  int mode = myPaddleMode->getValue();
  instance()->eventHandler().setPaddleMode(mode);

/*  FIXME - add this to eventhandler core
  // Paddle sensitivity
  int sense = myPaddleSense->getValue();
  instance()->eventHandler().setPaddleSensee(sense);
*/
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
      myPaddleLabel->setValue(myPaddleMode->getValue());
      break;

    case kSenseChanged:
      mySenseLabel->setValue(myPaddleSense->getValue());
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
