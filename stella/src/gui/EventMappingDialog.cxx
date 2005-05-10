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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: EventMappingDialog.cxx,v 1.8 2005-05-10 19:20:43 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "Widget.hxx"
#include "ListWidget.hxx"
#include "Dialog.hxx"
#include "GuiUtils.hxx"
#include "Event.hxx"
#include "EventHandler.hxx"
#include "EventMappingDialog.hxx"

#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventMappingDialog::EventMappingDialog(OSystem* osystem, DialogContainer* parent,
                                       uInt16 x, uInt16 y, uInt16 w, uInt16 h)
    : Dialog(osystem, parent, x, y, w, h),
      myActionSelected(-1),
      myRemapStatus(false)
{
  // Add Default and OK buttons
  myDefaultsButton = addButton(10, h - 24, "Defaults", kDefaultsCmd, 0);
  myOKButton       = addButton(w - (kButtonWidth + 10), h - 24, "OK", kCloseCmd, 0);

  new StaticTextWidget(this, 10, 8, 200, 16, "Select an event to remap:", kTextAlignCenter);
  myActionsList = new ListWidget(this, 10, 20, 200, 100);
  myActionsList->setNumberingMode(kListNumberingOff);

  myKeyMapping  = new StaticTextWidget(this, 10, 125, w - 20, 16,
                                       "Key(s) : ", kTextAlignLeft);
  myKeyMapping->setFlags(WIDGET_CLEARBG);

  // Add remap and erase buttons
  myMapButton       = addButton(220, 30, "Map", kStartMapCmd, 0);
  myEraseButton     = addButton(220, 50, "Erase", kEraseCmd, 0);
  myCancelMapButton = addButton(220, 70, "Cancel", kStopMapCmd, 0);
  myCancelMapButton->setEnabled(false);

  // Get actions names
  StringList l;

  for(int i = 0; i < 60; ++i)  // FIXME - create a size() method
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
  myCancelMapButton->setEnabled(true);

  // And show a message indicating which key is being remapped
  string buf = "Select a new event for the '" +
               EventHandler::ourActionList[ myActionSelected ].action +
               "' action";	 	
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
    string buf = "Key(s) : " + EventHandler::ourActionList[ myActionSelected ].key;
    myKeyMapping->setLabel(buf);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingDialog::handleKeyDown(uInt16 ascii, Int32 keycode, Int32 modifiers)
{
  // Remap keys in remap mode, otherwise pass to listwidget
  if(myRemapStatus && myActionSelected >= 0)
  {
    Event::Type event = EventHandler::ourActionList[ myActionSelected ].event;
    instance()->eventHandler().addKeyMapping(event, ascii);

    stopRemapping();
  }
  else
    myActionsList->handleKeyDown(ascii, keycode, modifiers);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingDialog::handleCommand(CommandSender* sender, uInt32 cmd, uInt32 data)
{
  switch(cmd)
  {
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
