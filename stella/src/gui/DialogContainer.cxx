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
// Copyright (c) 1995-2006 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: DialogContainer.cxx,v 1.34 2006-12-08 16:49:33 stephena Exp $
//============================================================================

#include "OSystem.hxx"
#include "Dialog.hxx"
#include "Stack.hxx"
#include "EventHandler.hxx"
#include "bspf.hxx"
#include "DialogContainer.hxx"

#define JOY_DEADZONE 3200

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DialogContainer::DialogContainer(OSystem* osystem)
  : myOSystem(osystem),
    myBaseDialog(NULL),
    myTime(0),
    myRefreshFlag(false)
{
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DialogContainer::~DialogContainer()
{
  if(myBaseDialog)
    delete myBaseDialog;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::updateTime(uInt32 time)
{
  // We only need millisecond precision
  myTime = time / 1000;

  if(myDialogStack.empty())
    return;

  // Check for pending continuous events and send them to the active dialog box
  Dialog* activeDialog = myDialogStack.top();

  // Key still pressed
  if(myCurrentKeyDown.keycode != 0 && myKeyRepeatTime < myTime)
  {
    activeDialog->handleKeyDown(myCurrentKeyDown.ascii, myCurrentKeyDown.keycode,
                                myCurrentKeyDown.flags);
    myKeyRepeatTime = myTime + kRepeatSustainDelay;
  }

  // Mouse button still pressed
  if(myCurrentMouseDown.button != -1 && myClickRepeatTime < myTime)
  {
    activeDialog->handleMouseDown(myCurrentMouseDown.x - activeDialog->_x,
                                  myCurrentMouseDown.y - activeDialog->_y,
                                  myCurrentMouseDown.button, 1);
    myClickRepeatTime = myTime + kRepeatSustainDelay;
  }

  // Joystick button still pressed
  if(myCurrentButtonDown.stick != -1 && myButtonRepeatTime < myTime)
  {
    activeDialog->handleJoyDown(myCurrentButtonDown.stick, myCurrentButtonDown.button);
    myButtonRepeatTime = myTime + kRepeatSustainDelay;
  }

  // Joystick axis still pressed
  if(myCurrentAxisDown.stick != -1 && myAxisRepeatTime < myTime)
  {
    activeDialog->handleJoyAxis(myCurrentAxisDown.stick, myCurrentAxisDown.axis,
                                myCurrentAxisDown.value);
    myAxisRepeatTime = myTime + kRepeatSustainDelay;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::draw()
{
  // Draw all the dialogs on the stack when we want a full refresh
  if(myRefreshFlag)
  {
    for(int i = 0; i < myDialogStack.size(); i++)
    {
      myDialogStack[i]->setDirty();
      myDialogStack[i]->drawDialog();
    }
    myRefreshFlag = false;
  }
  else if(!myDialogStack.empty())
  {
    myDialogStack.top()->drawDialog();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::addDialog(Dialog* d)
{
  myDialogStack.push(d);

  d->open();
  d->setDirty();  // Next update() will take care of drawing
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::removeDialog()
{
  if(!myDialogStack.empty())
  {
    myDialogStack.pop();

    // We need to redraw the entire screen contents, since we don't know
    // what was obscured
    myOSystem->eventHandler().refreshDisplay();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::reStack()
{
  // Pop all items from the stack, and then add the base menu
  while(!myDialogStack.empty())
    myDialogStack.pop();
  addDialog(myBaseDialog);

  // Erase any previous messages
  myOSystem->frameBuffer().hideMessage();

  // Reset all continuous events
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::handleKeyEvent(int ascii, int key, int mod, uInt8 state)
{
  if(myDialogStack.empty())
    return;

  // Send the event to the dialog box on the top of the stack
  Dialog* activeDialog = myDialogStack.top();
  if(state == 1)
  {
    myCurrentKeyDown.ascii   = ascii;
    myCurrentKeyDown.keycode = key;
    myCurrentKeyDown.flags   = mod;
    myKeyRepeatTime = myTime + kRepeatInitialDelay;

    activeDialog->handleKeyDown(ascii, key, mod);
  }
  else
  {
    activeDialog->handleKeyUp(ascii, key, mod);

    // Only stop firing events if it's the current key
    if (key == myCurrentKeyDown.keycode)
      myCurrentKeyDown.keycode = 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::handleMouseMotionEvent(int x, int y, int button)
{
  if(myDialogStack.empty())
    return;

  // Send the event to the dialog box on the top of the stack
  Dialog* activeDialog = myDialogStack.top();
  activeDialog->handleMouseMoved(x - activeDialog->_x,
                                 y - activeDialog->_y,
                                 button);

  // Turn off continuous click events as soon as the mouse moves
  myCurrentMouseDown.button = -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::handleMouseButtonEvent(MouseButton b, int x, int y, uInt8 state)
{
  if(myDialogStack.empty())
    return;

  // Send the event to the dialog box on the top of the stack
  Dialog* activeDialog = myDialogStack.top();

  int button = (b == EVENT_LBUTTONDOWN || b == EVENT_LBUTTONUP) ? 1 : 2;
  switch(b)
  {
    case EVENT_LBUTTONDOWN:
    case EVENT_RBUTTONDOWN:
      // If more than two clicks have been recorded, we start over
      if(myLastClick.count == 2)
      {
        myLastClick.x = myLastClick.y = 0;
        myLastClick.time = 0;
        myLastClick.count = 0;
      }

      if(myLastClick.count && (myTime < myLastClick.time + kDoubleClickDelay)
         && ABS(myLastClick.x - x) < 3
         && ABS(myLastClick.y - y) < 3)
      {
        myLastClick.count++;
      }
      else
      {
        myLastClick.x = x;
        myLastClick.y = y;
        myLastClick.count = 1;
      }
      myLastClick.time = myTime;

      // Now account for repeated mouse events (click and hold)
      myCurrentMouseDown.x = x;
      myCurrentMouseDown.y = y;
      myCurrentMouseDown.button = button;
      myClickRepeatTime = myTime + kRepeatInitialDelay;

      activeDialog->handleMouseDown(x - activeDialog->_x, y - activeDialog->_y,
                                    button, myLastClick.count);
      break;

    case EVENT_LBUTTONUP:
    case EVENT_RBUTTONUP:
      activeDialog->handleMouseUp(x - activeDialog->_x, y - activeDialog->_y,
                                  button, myLastClick.count);

      if(button == myCurrentMouseDown.button)
        myCurrentMouseDown.button = -1;
      break;

    case EVENT_WHEELUP:
      activeDialog->handleMouseWheel(x - activeDialog->_x, y - activeDialog->_y, -1);
      break;

    case EVENT_WHEELDOWN:
      activeDialog->handleMouseWheel(x - activeDialog->_x, y - activeDialog->_y, 1);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::handleJoyEvent(int stick, int button, uInt8 state)
{
  if(myDialogStack.empty())
    return;

  // Send the event to the dialog box on the top of the stack
  Dialog* activeDialog = myDialogStack.top();

  if(state == 1)
  {
    myCurrentButtonDown.stick  = stick;
    myCurrentButtonDown.button = button;
    myButtonRepeatTime = myTime + kRepeatInitialDelay;

    activeDialog->handleJoyDown(stick, button);
  }
  else
  {
    // Only stop firing events if it's the current button
    if(stick == myCurrentButtonDown.stick)
      myCurrentButtonDown.stick = myCurrentButtonDown.button = -1;

    activeDialog->handleJoyUp(stick, button);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::handleJoyAxisEvent(int stick, int axis, int value)
{
  if(myDialogStack.empty())
    return;

  // Send the event to the dialog box on the top of the stack
  Dialog* activeDialog = myDialogStack.top();

  if(value > JOY_DEADZONE)
    value -= JOY_DEADZONE;
  else if(value < -JOY_DEADZONE )
    value += JOY_DEADZONE;
  else
    value = 0;

  // Only stop firing events if it's the current stick
  if(myCurrentAxisDown.stick == stick && value == 0)
  {
    myCurrentAxisDown.stick = myCurrentAxisDown.axis = -1;
  }
  else
  {
    // Now account for repeated axis events (press and hold)
    myCurrentAxisDown.stick = stick;
    myCurrentAxisDown.axis  = axis;
    myCurrentAxisDown.value = value;
    myAxisRepeatTime = myTime + kRepeatInitialDelay;
  }
  activeDialog->handleJoyAxis(stick, axis, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::handleJoyHatEvent(int stick, int hat, int value)
{
  if(myDialogStack.empty())
    return;

  // Send the event to the dialog box on the top of the stack
  Dialog* activeDialog = myDialogStack.top();

  // FIXME - add repeat processing, similar to axis/button events
  activeDialog->handleJoyHat(stick, hat, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::reset()
{
  myCurrentKeyDown.keycode = 0;
  myCurrentMouseDown.button = -1;
  myLastClick.x = myLastClick.y = 0;
  myLastClick.time = 0;
  myLastClick.count = 0;

  myCurrentButtonDown.stick = myCurrentButtonDown.button = -1;
  myCurrentAxisDown.stick = myCurrentAxisDown.axis = -1;
  myCurrentHatDown.stick = myCurrentHatDown.hat = -1;
}
