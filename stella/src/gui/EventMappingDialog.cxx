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
// $Id: EventMappingDialog.cxx,v 1.15 2005-06-20 18:32:12 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "Widget.hxx"
#include "ListWidget.hxx"
#include "PopUpWidget.hxx"
#include "Dialog.hxx"
#include "GuiUtils.hxx"
#include "Event.hxx"
#include "EventHandler.hxx"
#include "EventMappingDialog.hxx"

#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventMappingDialog::EventMappingDialog(OSystem* osystem, DialogContainer* parent,
                                       int x, int y, int w, int h)
    : Dialog(osystem, parent, x, y, w, h),
      myActionSelected(-1),
      myRemapStatus(false)
{
  // Add Default and OK buttons
  myDefaultsButton = addButton(10, h - 24, "Defaults", kDefaultsCmd, 0);
  myOKButton       = addButton(w - (kButtonWidth + 10), h - 24, "OK", kOKCmd, 0);

  new StaticTextWidget(this, 10, 8, 150, 16, "Select an event to remap:", kTextAlignCenter);
  myActionsList = new ListWidget(this, 10, 20, 150, 100);
  myActionsList->setNumberingMode(kListNumberingOff);
  myActionsList->setEditable(false);
  myActionsList->clearFlags(WIDGET_TAB_NAVIGATE);

  myKeyMapping  = new StaticTextWidget(this, 10, 125, w - 20, 16,
                                       "Action: ", kTextAlignLeft);
  myKeyMapping->setFlags(WIDGET_CLEARBG);

  // Add remap and erase buttons
  myMapButton       = addButton(170, 25, "Map", kStartMapCmd, 0);
  myEraseButton     = addButton(170, 45, "Erase", kEraseCmd, 0);
  myCancelMapButton = addButton(170, 65, "Cancel", kStopMapCmd, 0);
  myCancelMapButton->setEnabled(false);

  // Add 'mouse to paddle' mapping
  myPaddleModeText = new StaticTextWidget(this, 168, 93, 50, kLineHeight,
                                          "Mouse is", kTextAlignCenter);
  myPaddleModePopup = new PopUpWidget(this, 160, 105, 60, kLineHeight,
                                     "paddle: ", 40, 0);
  myPaddleModePopup->appendEntry("0", 0);
  myPaddleModePopup->appendEntry("1", 1);
  myPaddleModePopup->appendEntry("2", 2);
  myPaddleModePopup->appendEntry("3", 3);

  // Get actions names
  StringList l;

  for(int i = 0; i < 61; ++i)
    l.push_back(EventHandler::ourActionList[i].action);

  myActionsList->setList(l);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventMappingDialog::~EventMappingDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingDialog::loadConfig()
{
  // Make sure remapping is turned off, just in case the user didn't properly
  // exit from the dialog last time
  stopRemapping();

  // Paddle mode
  int mode = instance()->settings().getInt("paddle");
  myPaddleModePopup->setSelectedTag(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingDialog::saveConfig()
{
  // Paddle mode
  int mode = myPaddleModePopup->getSelectedTag();
  instance()->eventHandler().setPaddleMode(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingDialog::startRemapping()
{
  if(myActionSelected < 0 || myRemapStatus)
    return;

  // Set the flags for the next event that arrives
  myRemapStatus = true;

  // Disable all other widgets while in remap mode, except enable 'Cancel'
  myActionsList->setEnabled(false);
  myMapButton->setEnabled(false);
  myEraseButton->setEnabled(false);
  myDefaultsButton->setEnabled(false);
  myOKButton->setEnabled(false);
  myPaddleModeText->setEnabled(false);
  myPaddleModePopup->setEnabled(false);
  myCancelMapButton->setEnabled(true);

  // And show a message indicating which key is being remapped
  string buf = "Select action for '" +
               EventHandler::ourActionList[ myActionSelected ].action +
               "' event";	 	
  myKeyMapping->setColor(kTextColorEm);
  myKeyMapping->setLabel(buf);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingDialog::eraseRemapping()
{
  if(myActionSelected < 0)
    return;

  Event::Type event = EventHandler::ourActionList[ myActionSelected ].event;
  instance()->eventHandler().eraseMapping(event);

  drawKeyMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingDialog::stopRemapping()
{
  // Turn off remap mode
  myRemapStatus = false;

  // And re-enable all the widgets
  myActionsList->setEnabled(true);
  myMapButton->setEnabled(false);
  myEraseButton->setEnabled(false);
  myDefaultsButton->setEnabled(true);
  myOKButton->setEnabled(true);
  myPaddleModeText->setEnabled(true);
  myPaddleModePopup->setEnabled(true);
  myCancelMapButton->setEnabled(false);

  // Make sure the list widget is in a known state
  if(myActionSelected >= 0)
  {
    drawKeyMapping();
    myMapButton->setEnabled(true);
    myEraseButton->setEnabled(true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingDialog::drawKeyMapping()
{
  if(myActionSelected >= 0)
  {
    string buf = "Action: " + EventHandler::ourActionList[ myActionSelected ].key;
    myKeyMapping->setColor(kTextColor);
    myKeyMapping->setLabel(buf);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingDialog::handleKeyDown(int ascii, int keycode, int modifiers)
{
  // Remap keys in remap mode, otherwise pass to listwidget
  if(myRemapStatus && myActionSelected >= 0)
  {
    Event::Type event = EventHandler::ourActionList[ myActionSelected ].event;
    instance()->eventHandler().addKeyMapping(event, keycode);

    stopRemapping();
  }
  else
    myActionsList->handleKeyDown(ascii, keycode, modifiers);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingDialog::handleJoyDown(int stick, int button)
{
  // Remap joystick buttons in remap mode, otherwise pass to listwidget
  if(myRemapStatus && myActionSelected >= 0)
  {
    Event::Type event = EventHandler::ourActionList[ myActionSelected ].event;
    instance()->eventHandler().addJoyMapping(event, stick, button);

    stopRemapping();
  }
  else
    myActionsList->handleJoyDown(stick, button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingDialog::handleCommand(CommandSender* sender, int cmd, int data)
{
  switch(cmd)
  {
    case kOKCmd:
      saveConfig();
      close();
      break;

    case kListSelectionChangedCmd:
      if(myActionsList->getSelected() >= 0)
      {
        myActionSelected = myActionsList->getSelected();
        drawKeyMapping();
        myMapButton->setEnabled(true);
        myEraseButton->setEnabled(true);
        myCancelMapButton->setEnabled(false);
      }
      break;

    case kStartMapCmd:
      startRemapping();
      break;

    case kEraseCmd:
      eraseRemapping();
      break;

    case kStopMapCmd:
      stopRemapping();
      break;

    case kDefaultsCmd:
      instance()->eventHandler().setDefaultMapping();
      drawKeyMapping();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data);
  }
}
