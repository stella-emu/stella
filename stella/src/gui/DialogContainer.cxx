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
// $Id: DialogContainer.cxx,v 1.27 2006-01-08 13:55:03 stephena Exp $
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
  memset(&ourJoyMouse, 0, sizeof(JoyMouse));
  ourJoyMouse.delay_time = 25;

  ourEnableJoyMouseFlag = myOSystem->settings().getBool("joymouse");
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

  if(myCurrentKeyDown.keycode != 0 && myKeyRepeatTime < myTime)
  {
    activeDialog->handleKeyDown(myCurrentKeyDown.ascii, myCurrentKeyDown.keycode,
                                myCurrentKeyDown.flags);
    myKeyRepeatTime = myTime + kKeyRepeatSustainDelay;
  }

  if(myCurrentMouseDown.button != -1 && myClickRepeatTime < myTime)
  {
    activeDialog->handleMouseDown(myCurrentMouseDown.x - activeDialog->_x,
                                  myCurrentMouseDown.y - activeDialog->_y,
                                  myCurrentMouseDown.button, 1);
    myClickRepeatTime = myTime + kClickRepeatSustainDelay;
  }

  if(myCurrentAxisDown.stick != -1 && myAxisRepeatTime < myTime)
  {
    // The longer an axis event is enabled, the faster it should change
    // We do this by decreasing the amount of time between consecutive axis events
    // After a certain threshold, send 10 events at a time (this is necessary
    // since at some point, we'd like to process more eventss than the
    // current framerate allows)
    myCurrentAxisDown.count++;
    int interval = myCurrentAxisDown.count / 40 + 1;
    myAxisRepeatTime = myTime + kAxisRepeatSustainDelay / interval;
    if(interval > 3)
    {
      for(int i = 0; i < 10; ++i)
        activeDialog->handleJoyAxis(myCurrentAxisDown.stick, myCurrentAxisDown.axis,
                                    myCurrentAxisDown.value);
      myAxisRepeatTime = myTime + kAxisRepeatSustainDelay / 3;
    }
    else
    {
      activeDialog->handleJoyAxis(myCurrentAxisDown.stick, myCurrentAxisDown.axis,
                                  myCurrentAxisDown.value);
      myAxisRepeatTime = myTime + kAxisRepeatSustainDelay / interval;
    }
  }

  // Update joy to mouse events
  if(ourEnableJoyMouseFlag)
    handleJoyMouse(time);
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
void DialogContainer::handleKeyEvent(int unicode, int key, int mod, uInt8 state)
{
  if(myDialogStack.empty())
    return;

  // Send the event to the dialog box on the top of the stack
  Dialog* activeDialog = myDialogStack.top();
  if(state == 1)
  {
    myCurrentKeyDown.ascii   = unicode;
    myCurrentKeyDown.keycode = key;
    myCurrentKeyDown.flags   = mod;
    myKeyRepeatTime = myTime + kKeyRepeatInitialDelay;

    activeDialog->handleKeyDown(unicode, key, mod);
  }
  else
  {
    activeDialog->handleKeyUp(unicode, key, mod);

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
      myClickRepeatTime = myTime + kClickRepeatInitialDelay;

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

  // Only preprocess button events if the dialog absolutely doesn't want them
  if(!activeDialog->wantsAllEvents())
  {
    // Some buttons act as directions.  In those cases, translate them
    // to axis events instead of mouse button events
    int value = state > 0 ? 32767 : 0;
    bool handled = true;
    switch(button)
    {
      case kJDirUp:
        handleJoyAxisEvent(stick, 1, -value);  // axis 1, -value ==> UP
        break;
      case kJDirLeft:
        handleJoyAxisEvent(stick, 0, -value);  // axis 0, -value ==> LEFT
        break;
      case kJDirDown:
        handleJoyAxisEvent(stick, 1, value);   // axis 1, +value ==> DOWN
        break;
      case kJDirRight:
        handleJoyAxisEvent(stick, 0, value);   // axis 0, +value ==> RIGHT
        break;
      default:
        handled = false;
    }
    if(handled)
      return;
  }

  if(activeDialog->wantsEvents())
  {
    if(state == 1)
      activeDialog->handleJoyDown(stick, button);
    else
      activeDialog->handleJoyUp(stick, button);
  }
  else
    myOSystem->eventHandler().createMouseButtonEvent(
      ourJoyMouse.x, ourJoyMouse.y, state);
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

  if(activeDialog->wantsEvents())
  {
    // Only stop firing events if it's the current key
    if(myCurrentAxisDown.stick == stick && value == 0)
    {
      myCurrentAxisDown.stick = myCurrentAxisDown.axis = -1;
      myCurrentAxisDown.count = 0;
    }
    else
    {
      // Now account for repeated axis events (press and hold)
      myCurrentAxisDown.stick = stick;
      myCurrentAxisDown.axis  = axis;
      myCurrentAxisDown.value = value;
      myAxisRepeatTime = myTime + kAxisRepeatInitialDelay;
    }
    activeDialog->handleJoyAxis(stick, axis, value);
  }
  else
  {
    if(axis % 2 == 0)  // x-direction
    {
      if(value != 0)
      {
        ourJoyMouse.x_vel = (value > 0) ? 1 : -1;
        ourJoyMouse.x_down_count = 1;
      }
      else
      {
        ourJoyMouse.x_vel = 0;
        ourJoyMouse.x_down_count = 0;
      }
    }
    else   // y-direction
    {
      value = -value;

      if(value != 0)
      {
        ourJoyMouse.y_vel = (-value > 0) ? 1 : -1;
        ourJoyMouse.y_down_count = 1;
      }
      else
      {
        ourJoyMouse.y_vel = 0;
        ourJoyMouse.y_down_count = 0;
      }
    }
    myCurrentAxisDown.stick = myCurrentAxisDown.axis = -1;
    myCurrentAxisDown.count = 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::handleJoyMouse(uInt32 time)
{
  bool mouseAccel = true;//false;  // TODO - make this a commandline option
  int oldX = ourJoyMouse.x, oldY = ourJoyMouse.y;

  if(time >= ourJoyMouse.last_time + ourJoyMouse.delay_time)
  {
    ourJoyMouse.last_time = time;
    if(ourJoyMouse.x_down_count == 1)
    {
      ourJoyMouse.x_down_time = time;
      ourJoyMouse.x_down_count = 2;
    }
    if(ourJoyMouse.y_down_count == 1)
    {
      ourJoyMouse.y_down_time = time;
      ourJoyMouse.y_down_count = 2;
    }

    if(ourJoyMouse.x_vel || ourJoyMouse.y_vel)
    {
      if(ourJoyMouse.x_down_count)
      {
        if(mouseAccel && time > ourJoyMouse.x_down_time + ourJoyMouse.delay_time * 12)
        {
          if(ourJoyMouse.x_vel > 0)
            ourJoyMouse.x_vel++;
          else
            ourJoyMouse.x_vel--;
        }
        else if(time > ourJoyMouse.x_down_time + ourJoyMouse.delay_time * 8)
        {
          if(ourJoyMouse.x_vel > 0)
            ourJoyMouse.x_vel = ourJoyMouse.amt;
          else
            ourJoyMouse.x_vel = -ourJoyMouse.amt;
        }
      }
      if(ourJoyMouse.y_down_count)
      {
        if(mouseAccel && time > ourJoyMouse.y_down_time + ourJoyMouse.delay_time * 12)
        {
          if(ourJoyMouse.y_vel > 0)
            ourJoyMouse.y_vel++;
          else
            ourJoyMouse.y_vel--;
        }
        else if(time > ourJoyMouse.y_down_time + ourJoyMouse.delay_time * 8)
        {
          if(ourJoyMouse.y_vel > 0)
            ourJoyMouse.y_vel = ourJoyMouse.amt;
          else
            ourJoyMouse.y_vel = -ourJoyMouse.amt;
        }
      }

      ourJoyMouse.x += ourJoyMouse.x_vel;
      ourJoyMouse.y += ourJoyMouse.y_vel;

      if(ourJoyMouse.x < 0)
      {
        ourJoyMouse.x = 0;
        ourJoyMouse.x_vel = -1;
        ourJoyMouse.x_down_count = 1;
      }
      else if(ourJoyMouse.x > ourJoyMouse.x_max)
      {
        ourJoyMouse.x = ourJoyMouse.x_max;
        ourJoyMouse.x_vel = 1;
        ourJoyMouse.x_down_count = 1;
      }

      if(ourJoyMouse.y < 0)
      {
        ourJoyMouse.y = 0;
        ourJoyMouse.y_vel = -1;
        ourJoyMouse.y_down_count = 1;
      }
      else if(ourJoyMouse.y > ourJoyMouse.y_max)
      {
        ourJoyMouse.y = ourJoyMouse.y_max;
        ourJoyMouse.y_vel = 1;
        ourJoyMouse.y_down_count = 1;
      }

      if(oldX != ourJoyMouse.x || oldY != ourJoyMouse.y)
        myOSystem->eventHandler().createMouseMotionEvent(ourJoyMouse.x, ourJoyMouse.y);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::reset()
{
  myCurrentKeyDown.keycode = 0;
  myCurrentMouseDown.button = -1;
  myLastClick.x = myLastClick.y = 0;
  myLastClick.time = 0;
  myLastClick.count = 0;

  myCurrentAxisDown.stick = myCurrentAxisDown.axis = -1;
  myCurrentAxisDown.count = 0;

  int oldX = ourJoyMouse.x, oldY = ourJoyMouse.y;
  if(ourJoyMouse.x > ourJoyMouse.x_max)
    ourJoyMouse.x = ourJoyMouse.x_max;
  if(ourJoyMouse.y > ourJoyMouse.y_max)
    ourJoyMouse.y = ourJoyMouse.y_max;

  if(oldX != ourJoyMouse.x || oldY != ourJoyMouse.y)
    myOSystem->eventHandler().createMouseMotionEvent(ourJoyMouse.x, ourJoyMouse.y);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DialogContainer::ourEnableJoyMouseFlag;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
JoyMouse DialogContainer::ourJoyMouse;
