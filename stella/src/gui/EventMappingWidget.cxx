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
// $Id: EventMappingWidget.cxx,v 1.3 2005-12-20 19:05:16 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "Widget.hxx"
#include "StringListWidget.hxx"
#include "GuiUtils.hxx"
#include "Event.hxx"
#include "EventHandler.hxx"
#include "EventMappingWidget.hxx"

#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventMappingWidget::EventMappingWidget(GuiObject* boss, int x, int y, int w, int h)
  : Widget(boss, x, y, w, h),
    CommandSender(boss),
    myActionSelected(-1),
    myRemapStatus(false)
{
  const GUI::Font& font = instance()->font();
  const int fontHeight = font.getFontHeight(),
            lineHeight = font.getLineHeight();
  int xpos = 5, ypos = 5;

  myActionsList = new StringListWidget(boss, font, xpos, ypos,
                                       _w - 70, _h - 3*lineHeight);
  myActionsList->setTarget(this);
  myActionsList->setNumberingMode(kListNumberingOff);
  myActionsList->setEditable(false);
  myActionsList->setFlags(WIDGET_NODRAW_FOCUS);
  addFocusWidget(myActionsList);

  // Add remap, erase, cancel and default buttons
  xpos += myActionsList->getWidth() + 15;  ypos += 5;
  myMapButton = new ButtonWidget(boss, xpos, ypos, 50, 16,
                                 "Map", kStartMapCmd);
  myMapButton->setTarget(this);
  ypos += 20;
  myEraseButton = new ButtonWidget(boss, xpos, ypos, 50, 16,
                                   "Erase", kEraseCmd);
  myEraseButton->setTarget(this);
  ypos += 20;
  myCancelMapButton = new ButtonWidget(boss, xpos, ypos, 50, 16,
                                       "Cancel", kStopMapCmd);
  myCancelMapButton->setTarget(this);
  myCancelMapButton->clearFlags(WIDGET_ENABLED);
  ypos += 30;
  myDefaultsButton = new ButtonWidget(boss, xpos, ypos, 50, 16,
                                      "Defaults", kDefaultsCmd);
  myDefaultsButton->setTarget(this);

  // Show message for currently selected event
  xpos = 10;  ypos = 5 + myActionsList->getHeight() + 3;
  myKeyMapping  = new StaticTextWidget(boss, xpos, ypos, _w - 20, fontHeight,
                                       "Action: ", kTextAlignLeft);
  myKeyMapping->setFlags(WIDGET_CLEARBG);

  // Get actions names
  StringList l;

  for(int i = 0; i < kActionListSize; ++i)
    l.push_back(EventHandler::ourActionList[i].action);

  myActionsList->setList(l);
  myActionSelected = myActionsList->getSelected();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventMappingWidget::~EventMappingWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::loadConfig()
{
  // Make sure remapping is turned off, just in case the user didn't properly
  // exit last time
  stopRemapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::saveConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::startRemapping()
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
  myCancelMapButton->setEnabled(true);

  // And show a message indicating which key is being remapped
  string buf = "Select action for '" +
               EventHandler::ourActionList[ myActionSelected ].action +
               "' event";	 	
  myKeyMapping->setColor(kTextColorEm);
  myKeyMapping->setLabel(buf);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::eraseRemapping()
{
  if(myActionSelected < 0)
    return;

  Event::Type event = EventHandler::ourActionList[ myActionSelected ].event;
  instance()->eventHandler().eraseMapping(event);

  drawKeyMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::stopRemapping()
{
  // Turn off remap mode
  myRemapStatus = false;

  // And re-enable all the widgets
  myActionsList->setEnabled(true);
  myMapButton->setEnabled(false);
  myEraseButton->setEnabled(false);
  myDefaultsButton->setEnabled(true);
  myCancelMapButton->setEnabled(false);

  // Make sure the list widget is in a known state
cerr << "myActionSelected = " << myActionSelected << endl;
  if(myActionSelected >= 0)
  {
    drawKeyMapping();
    myMapButton->setEnabled(true);
    myEraseButton->setEnabled(true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::drawKeyMapping()
{
  if(myActionSelected >= 0)
  {
    string buf = "Action: " + EventHandler::ourActionList[ myActionSelected ].key;
    myKeyMapping->setColor(kTextColor);
    myKeyMapping->setLabel(buf);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventMappingWidget::handleKeyDown(int ascii, int keycode, int modifiers)
{
  // Remap keys in remap mode
  if(myRemapStatus && myActionSelected >= 0)
  {
    Event::Type event = EventHandler::ourActionList[ myActionSelected ].event;
    instance()->eventHandler().addKeyMapping(event, keycode);

    stopRemapping();
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::handleJoyDown(int stick, int button)
{
  // Remap joystick buttons in remap mode
  if(myRemapStatus && myActionSelected >= 0)
  {
    Event::Type event = EventHandler::ourActionList[ myActionSelected ].event;
    instance()->eventHandler().addJoyMapping(event, stick, button);

    stopRemapping();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::handleJoyAxis(int stick, int axis, int value)
{
  // Remap joystick buttons in remap mode
  if(myRemapStatus && myActionSelected >= 0)
  {
    Event::Type event = EventHandler::ourActionList[ myActionSelected ].event;
    instance()->eventHandler().addJoyAxisMapping(event, stick, axis, value);

    stopRemapping();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::handleCommand(CommandSender* sender, int cmd,
                                       int data, int id)
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
  }
}
