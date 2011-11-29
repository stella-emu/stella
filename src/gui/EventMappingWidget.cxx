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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <sstream>

#include "bspf.hxx"

#include "EventHandler.hxx"
#include "Event.hxx"
#include "OSystem.hxx"
#include "EditTextWidget.hxx"
#include "StringListWidget.hxx"
#include "Widget.hxx"
#include "ComboDialog.hxx"

#include "EventMappingWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventMappingWidget::EventMappingWidget(GuiObject* boss, const GUI::Font& font,
                                       int x, int y, int w, int h,
                                       const StringList& actions, EventMode mode)
  : Widget(boss, font, x, y, w, h),
    CommandSender(boss),
    myEventMode(mode),
    myActionSelected(-1),
    myRemapStatus(false),
    myFirstTime(true)
{
  const int fontHeight   = font.getFontHeight(),
            lineHeight   = font.getLineHeight(),
            buttonWidth  = font.getStringWidth("Defaults") + 10,
            buttonHeight = font.getLineHeight() + 4;
  int xpos = 5, ypos = 5;

  myActionsList = new StringListWidget(boss, font, xpos, ypos,
                                       _w - buttonWidth - 20, _h - 3*lineHeight);
  myActionsList->setTarget(this);
  myActionsList->setNumberingMode(kListNumberingOff);
  myActionsList->setEditable(false);
  myActionsList->setList(actions);
  addFocusWidget(myActionsList);

  // Add remap, erase, cancel and default buttons
  xpos += myActionsList->getWidth() + 5;  ypos += 5;
  myMapButton = new ButtonWidget(boss, font, xpos, ypos,
                                 buttonWidth, buttonHeight,
                                 "Map", kStartMapCmd);
  myMapButton->setTarget(this);
  addFocusWidget(myMapButton);

  ypos += lineHeight + 10;
  myCancelMapButton = new ButtonWidget(boss, font, xpos, ypos,
                                       buttonWidth, buttonHeight,
                                       "Cancel", kStopMapCmd);
  myCancelMapButton->setTarget(this);
  myCancelMapButton->clearFlags(WIDGET_ENABLED);
  addFocusWidget(myCancelMapButton);

  ypos += lineHeight + 20;
  myEraseButton = new ButtonWidget(boss, font, xpos, ypos,
                                   buttonWidth, buttonHeight,
                                   "Erase", kEraseCmd);
  myEraseButton->setTarget(this);
  addFocusWidget(myEraseButton);

  ypos += lineHeight + 10;
  myResetButton = new ButtonWidget(boss, font, xpos, ypos,
                                   buttonWidth, buttonHeight,
                                   "Reset", kResetCmd);
  myResetButton->setTarget(this);
  addFocusWidget(myResetButton);

  if(mode == kEmulationMode)
  {
    ypos += lineHeight + 20;
    myComboButton = new ButtonWidget(boss, font, xpos, ypos,
                                     buttonWidth, buttonHeight,
                                     "Combo", kComboCmd);
    myComboButton->setTarget(this);
    addFocusWidget(myComboButton);

    StringMap combolist;
    instance().eventHandler().getComboList(mode, combolist);
    myComboDialog = new ComboDialog(boss, font, combolist);
  }
  else
    myComboButton = NULL;

  // Show message for currently selected event
  xpos = 10;  ypos = 5 + myActionsList->getHeight() + 5;
  StaticTextWidget* t;
  t = new StaticTextWidget(boss, font, xpos, ypos, font.getStringWidth("Action:"),
                           fontHeight, "Action:", kTextAlignLeft);
  t->setFlags(WIDGET_CLEARBG);

  myKeyMapping = new EditTextWidget(boss, font, xpos + t->getWidth() + 5, ypos,
                                    _w - xpos - t->getWidth() - 15, lineHeight, "");
  myKeyMapping->setEditable(false);
  myKeyMapping->clearFlags(WIDGET_RETAIN_FOCUS);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventMappingWidget::~EventMappingWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::loadConfig()
{
  if(myFirstTime)
  {
    myActionsList->setSelected(0);
    myFirstTime = false;
  }

  // Make sure remapping is turned off, just in case the user didn't properly
  // exit last time
  if(myRemapStatus)
    stopRemapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::saveConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::setDefaults()
{
  instance().eventHandler().setDefaultMapping(Event::NoType, myEventMode);
  drawKeyMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::startRemapping()
{
  if(myActionSelected < 0 || myRemapStatus)
    return;

  // Set the flags for the next event that arrives
  myRemapStatus = true;

  // Reset all previous events for determining correct axis/hat values
  myLastStick = myLastAxis = myLastHat = myLastValue = -1;

  // Disable all other widgets while in remap mode, except enable 'Cancel'
  enableButtons(false);

  // And show a message indicating which key is being remapped
  ostringstream buf;
  buf << "Select action for '"
      << instance().eventHandler().actionAtIndex(myActionSelected, myEventMode)
      << "' event";
  myKeyMapping->setTextColor(kTextColorEm);
  myKeyMapping->setEditString(buf.str());

  // Make sure that this widget receives all raw data, before any
  // pre-processing occurs
  myActionsList->setFlags(WIDGET_WANTS_RAWDATA);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::eraseRemapping()
{
  if(myActionSelected < 0)
    return;

  Event::Type event =
    instance().eventHandler().eventAtIndex(myActionSelected, myEventMode);
  instance().eventHandler().eraseMapping(event, myEventMode);

  drawKeyMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::resetRemapping()
{
  if(myActionSelected < 0)
    return;

  Event::Type event =
    instance().eventHandler().eventAtIndex(myActionSelected, myEventMode);
  instance().eventHandler().setDefaultMapping(event, myEventMode);

  drawKeyMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::stopRemapping()
{
  // Turn off remap mode
  myRemapStatus = false;

  // Reset all previous events for determining correct axis/hat values
  myLastStick = myLastAxis = myLastHat = myLastValue = -1;

  // And re-enable all the widgets
  enableButtons(true);

  // Make sure the list widget is in a known state
  drawKeyMapping();

  // Widget is now free to process events normally
  myActionsList->clearFlags(WIDGET_WANTS_RAWDATA);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::drawKeyMapping()
{
  if(myActionSelected >= 0)
  {
    myKeyMapping->setTextColor(kTextColor);
    myKeyMapping->setEditString(instance().eventHandler().keyAtIndex(myActionSelected, myEventMode));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::enableButtons(bool state)
{
  myActionsList->setEnabled(state);
  myMapButton->setEnabled(state);
  myCancelMapButton->setEnabled(!state);
  myEraseButton->setEnabled(state);
  myResetButton->setEnabled(state);
  if(myComboButton)
  {
    Event::Type e =
      instance().eventHandler().eventAtIndex(myActionSelected, myEventMode);

    myComboButton->setEnabled(state && e >= Event::Combo1 && e <= Event::Combo16);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventMappingWidget::handleKeyDown(int ascii, int keycode, int modifiers)
{
  // Remap keys in remap mode
  if(myRemapStatus && myActionSelected >= 0)
  {
    Event::Type event =
      instance().eventHandler().eventAtIndex(myActionSelected, myEventMode);
    if(instance().eventHandler().addKeyMapping(event, myEventMode, keycode))
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
    Event::Type event =
      instance().eventHandler().eventAtIndex(myActionSelected, myEventMode);
    if(instance().eventHandler().addJoyButtonMapping(event, myEventMode, stick, button))
      stopRemapping();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::handleJoyAxis(int stick, int axis, int value)
{
  // Remap joystick axes in remap mode
  // There are two phases to detection:
  //   First, detect an axis 'on' event
  //   Then, detect the same axis 'off' event
  if(myRemapStatus && myActionSelected >= 0)
  {
    // Detect the first axis event that represents 'on'
    if(myLastStick == -1 && myLastAxis == -1 && value != 0)
    {
      myLastStick = stick;
      myLastAxis = axis;
      myLastValue = value;
    }
    // Detect the first axis event that matches a previously set
    // stick and axis, but turns the axis 'off'
    else if(myLastStick == stick && axis == myLastAxis && value == 0)
    {
      value = myLastValue;

      Event::Type event =
        instance().eventHandler().eventAtIndex(myActionSelected, myEventMode);
      if(instance().eventHandler().addJoyAxisMapping(event, myEventMode,
                                                     stick, axis, value))
        stopRemapping();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventMappingWidget::handleJoyHat(int stick, int hat, int value)
{
  // Remap joystick hats in remap mode
  // There are two phases to detection:
  //   First, detect a hat direction event
  //   Then, detect the same hat 'center' event
  if(myRemapStatus && myActionSelected >= 0)
  {
    // Detect the first hat event that represents a valid direction
    if(myLastStick == -1 && myLastHat == -1 && value != EVENT_HATCENTER)
    {
      myLastStick = stick;
      myLastHat = hat;
      myLastValue = value;

      return true;
    }
    // Detect the first hat event that matches a previously set
    // stick and hat, but centers the hat
    else if(myLastStick == stick && hat == myLastHat && value == EVENT_HATCENTER)
    {
      value = myLastValue;

      Event::Type event =
        instance().eventHandler().eventAtIndex(myActionSelected, myEventMode);
      if(instance().eventHandler().addJoyHatMapping(event, myEventMode,
                                                    stick, hat, value))
      {
        stopRemapping();
        return true;
      }
    }
  }

  return false;
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
        enableButtons(true);
      }
      break;

/*
    case kListItemDoubleClickedCmd:
      if(myActionsList->getSelected() >= 0)
      {
        myActionSelected = myActionsList->getSelected();
        startRemapping();
      }
      break;
*/

    case kStartMapCmd:
      startRemapping();
      break;

    case kStopMapCmd:
      stopRemapping();
      break;

    case kEraseCmd:
      eraseRemapping();
      break;

    case kResetCmd:
      resetRemapping();
      break;

    case kComboCmd:
      if(myComboDialog)
        myComboDialog->show(
          instance().eventHandler().eventAtIndex(myActionSelected, myEventMode),
          instance().eventHandler().actionAtIndex(myActionSelected, myEventMode));
      break;
  }
}
