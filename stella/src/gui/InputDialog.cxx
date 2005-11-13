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
// $Id: InputDialog.cxx,v 1.1 2005-11-13 22:25:47 stephena Exp $
//============================================================================

#include "OSystem.hxx"
#include "Widget.hxx"
#include "Array.hxx"
#include "TabWidget.hxx"
#include "EventMappingWidget.hxx"
#include "InputDialog.hxx"
#include "PopUpWidget.hxx"

#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputDialog::InputDialog(
      OSystem* osystem, DialogContainer* parent,
      int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h)
{
  const GUI::Font& font = instance()->font();
  const int fontHeight = font.getFontHeight(),
            lineHeight = font.getLineHeight();

  const int vBorder = 4;
  int xpos, ypos, tabID;
  WidgetArray wid;

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
  wid.clear();
  tabID = myTab->addTab("Virtual Devices");

  // Add 'mouse to paddle' mapping
  myPaddleModeText = new StaticTextWidget(myTab, 168, 93, 50, fontHeight,
                                          "Mouse is", kTextAlignCenter);
  myPaddleModePopup = new PopUpWidget(myTab, 160, 105, 60, lineHeight,
                                     "paddle: ", 40, 0);
  myPaddleModePopup->appendEntry("0", 0);
  myPaddleModePopup->appendEntry("1", 1);
  myPaddleModePopup->appendEntry("2", 2);
  myPaddleModePopup->appendEntry("3", 3);
  wid.push_back(myPaddleModePopup);

  // Add items for virtual device ports
  addToFocusList(wid, tabID);

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
void InputDialog::loadConfig()
{
  // Paddle mode
  int mode = instance()->settings().getInt("paddle");
  myPaddleModePopup->setSelectedTag(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::saveConfig()
{
  // Paddle mode
  int mode = myPaddleModePopup->getSelectedTag();
  instance()->eventHandler().setPaddleMode(mode);
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

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
