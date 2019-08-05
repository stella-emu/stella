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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <sstream>

#include "bspf.hxx"

#include "OSystem.hxx"
#include "GuiObject.hxx"
#include "FrameBuffer.hxx"
#include "EventHandler.hxx"
#include "Event.hxx"
#include "OSystem.hxx"
#include "EditTextWidget.hxx"
#include "StringListWidget.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "ComboDialog.hxx"
#include "Variant.hxx"
#include "EventMappingWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventMappingWidget::EventMappingWidget(GuiObject* boss, const GUI::Font& font,
                                       int x, int y, int w, int h,
                                       const StringList& actions, EventMode mode)
  : Widget(boss, font, x, y, w, h),
    CommandSender(boss),
    myComboDialog(nullptr),
    myEventMode(mode),
    myActionSelected(-1),
    myRemapStatus(false),
    myLastStick(0),
    myLastAxis(0),
    myLastHat(0),
    myLastValue(0),
    myLastButton(JOY_CTRL_NONE),
    myFirstTime(true)
{
  const int fontHeight   = font.getFontHeight(),
            lineHeight   = font.getLineHeight(),
            buttonWidth  = font.getStringWidth("Defaults") + 10,
            buttonHeight = font.getLineHeight() + 4;
  const int HBORDER = 8;
  const int VBORDER = 8;
  int xpos = HBORDER, ypos = VBORDER;

  myActionsList = new StringListWidget(boss, font, xpos, ypos,
                                       _w - buttonWidth - HBORDER * 2 - 8, _h - 3*lineHeight - VBORDER);
  myActionsList->setTarget(this);
  myActionsList->setEditable(false);
  myActionsList->setList(actions);
  addFocusWidget(myActionsList);

  // Add remap, erase, cancel and default buttons
  xpos = _w - HBORDER - buttonWidth;
  myMapButton = new ButtonWidget(boss, font, xpos, ypos,
                                 buttonWidth, buttonHeight,
                                 "Map" + ELLIPSIS, kStartMapCmd);
  myMapButton->setTarget(this);
  addFocusWidget(myMapButton);

  ypos += lineHeight + 10;
  myCancelMapButton = new ButtonWidget(boss, font, xpos, ypos,
                                       buttonWidth, buttonHeight,
                                       "Cancel", kStopMapCmd);
  myCancelMapButton->setTarget(this);
  myCancelMapButton->clearFlags(Widget::FLAG_ENABLED);
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
                                     "Combo" + ELLIPSIS, kComboCmd);
    myComboButton->setTarget(this);
    addFocusWidget(myComboButton);

    VariantList combolist = instance().eventHandler().getComboList(mode);
    myComboDialog = new ComboDialog(boss, font, combolist);
  }
  else
    myComboButton = nullptr;

  // Show message for currently selected event
  xpos = HBORDER;  ypos = VBORDER + myActionsList->getHeight() + 8;
  StaticTextWidget* t;
  t = new StaticTextWidget(boss, font, xpos, ypos+2, font.getStringWidth("Action"),
                           fontHeight, "Action", TextAlign::Left);

  myKeyMapping = new EditTextWidget(boss, font, xpos + t->getWidth() + 8, ypos,
                                    _w - xpos - t->getWidth() - 8 - HBORDER, lineHeight, "");
  myKeyMapping->setEditable(false, true);
  myKeyMapping->clearFlags(Widget::FLAG_RETAIN_FOCUS);
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
  cerr << "startRemapping" << endl;
  if(myActionSelected < 0 || myRemapStatus)
    return;

  // Set the flags for the next event that arrives
  myRemapStatus = true;

  // Reset all previous events for determining correct axis/hat values
  myLastStick = myLastAxis = myLastHat = myLastValue = -1;
  myLastButton = JOY_CTRL_NONE;

  // Reset the previously aggregated key mappings
  myMod = myLastKey = 0;

  // Disable all other widgets while in remap mode, except enable 'Cancel'
  enableButtons(false);

  // And show a message indicating which key is being remapped
  ostringstream buf;
  buf << "Select action for '"
      << instance().eventHandler().actionAtIndex(myActionSelected, myEventMode)
      << "' event";
  myKeyMapping->setTextColor(kTextColorEm);
  myKeyMapping->setText(buf.str());

  // Make sure that this widget receives all raw data, before any
  // pre-processing occurs
  myActionsList->setFlags(Widget::FLAG_WANTS_RAWDATA);
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
  cerr << "stopRemapping " << myRemapStatus << endl;

  // Reset all previous events for determining correct axis/hat values
  myLastStick = myLastAxis = myLastHat = myLastValue = -1;
  myLastButton = JOY_CTRL_NONE;

  // And re-enable all the widgets
  enableButtons(true);

  // Make sure the list widget is in a known state
  drawKeyMapping();

  // Widget is now free to process events normally
  myActionsList->clearFlags(Widget::FLAG_WANTS_RAWDATA);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::drawKeyMapping()
{
  if(myActionSelected >= 0)
  {
    myKeyMapping->setTextColor(kTextColor);
    myKeyMapping->setText(instance().eventHandler().keyAtIndex(myActionSelected, myEventMode));
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
bool EventMappingWidget::handleKeyDown(StellaKey key, StellaMod mod)
{
  // Remap keys in remap mode
  if (myRemapStatus && myActionSelected >= 0)
  {
    myLastKey = key;
    myMod |= mod;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventMappingWidget::handleKeyUp(StellaKey key, StellaMod mod)
{
  // Remap keys in remap mode
  if (myRemapStatus && myActionSelected >= 0
    && (mod & (KBDM_CTRL | KBDM_SHIFT | KBDM_ALT | KBDM_GUI)) == 0)
  {
    Event::Type event =
      instance().eventHandler().eventAtIndex(myActionSelected, myEventMode);
    if (instance().eventHandler().addKeyMapping(event, myEventMode, StellaKey(myLastKey), StellaMod(myMod)))
      stopRemapping();
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::handleJoyDown(int stick, int button)
{
  cerr << "handleJoyDown" << endl;
  // Remap joystick buttons in remap mode
  if(myRemapStatus && myActionSelected >= 0)
  {
    cerr << "remap button start " << myRemapStatus << endl;
    myLastStick = stick;
    myLastButton = button;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::handleJoyUp(int stick, int button)
{
  cerr << "handleJoyUp" << endl;
  // Remap joystick buttons in remap mode
  if (myRemapStatus && myActionSelected >= 0)
  {
    if (myLastStick == stick && myLastButton == button)
    {
      EventHandler& eh = instance().eventHandler();
      Event::Type event = eh.eventAtIndex(myActionSelected, myEventMode);

      cerr << "remap button stop" << endl;
      // This maps solo button presses only
      if (eh.addJoyMapping(event, myEventMode, stick, button))
        stopRemapping();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingWidget::handleJoyAxis(int stick, int axis, int value, int button)
{
  cerr << "handleJoyAxis:" << axis << ", " << value << ", (" << stick << ", " << myLastStick << "), (" << axis << ", " << myLastAxis << ")" << endl;
  // Remap joystick axes in remap mode
  // There are two phases to detection:
  //   First, detect an axis 'on' event
  //   Then, detect the same axis 'off' event
  if(myRemapStatus && myActionSelected >= 0)
  {
    // Detect the first axis event that represents 'on'
    if((myLastStick == -1 || myLastStick == stick) && myLastAxis == -1 && value != 0)
    {
      cerr << "remap start" << endl;
      myLastStick = stick;
      myLastAxis = axis;
      myLastValue = value;
    }
    // Detect the first axis event that matches a previously set
    // stick and axis, but turns the axis 'off'
    else if(myLastStick == stick && axis == myLastAxis && value == 0)
    {
      EventHandler& eh = instance().eventHandler();
      Event::Type event = eh.eventAtIndex(myActionSelected, myEventMode);

      cerr << "remap stop" << endl;
      if (eh.addJoyMapping(event, myEventMode, stick, myLastButton, JoyAxis(axis), myLastValue))
        stopRemapping();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventMappingWidget::handleJoyHat(int stick, int hat, JoyHat value, int button)
{
  // Remap joystick hats in remap mode
  // There are two phases to detection:
  //   First, detect a hat direction event
  //   Then, detect the same hat 'center' event
  if(myRemapStatus && myActionSelected >= 0)
  {
    // Detect the first hat event that represents a valid direction
    if((myLastStick == -1 || myLastStick == stick) && myLastHat == -1 && value != JoyHat::CENTER)
    {
      myLastStick = stick;
      myLastHat = hat;
      myLastValue = int(value);

      return true;
    }
    // Detect the first hat event that matches a previously set
    // stick and hat, but centers the hat
    else if(myLastStick == stick && hat == myLastHat && value == JoyHat::CENTER)
    {
      EventHandler& eh = instance().eventHandler();
      Event::Type event = eh.eventAtIndex(myActionSelected, myEventMode);

      if (eh.addJoyHatMapping(event, myEventMode, stick, myLastButton, hat, JoyHat(myLastValue)))
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
    case ListWidget::kSelectionChangedCmd:
      if(myActionsList->getSelected() >= 0)
      {
        myActionSelected = myActionsList->getSelected();
        drawKeyMapping();
        enableButtons(true);
      }
      break;

    case ListWidget::kDoubleClickedCmd:
      if(myActionsList->getSelected() >= 0)
      {
        myActionSelected = myActionsList->getSelected();
        startRemapping();
      }
      break;

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
