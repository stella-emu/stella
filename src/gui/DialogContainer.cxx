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

#include "OSystem.hxx"
#include "Dialog.hxx"
#include "Stack.hxx"
#include "EventHandler.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "Joystick.hxx"
#include "bspf.hxx"
#include "DialogContainer.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DialogContainer::DialogContainer(OSystem& osystem)
  : myOSystem(osystem),
    myTime(0),
    myClickRepeatTime(0),
    myButtonRepeatTime(0),
    myButtonLongPressTime(0),
    myAxisRepeatTime(0),
    myHatRepeatTime(0)
{
  _DOUBLE_CLICK_DELAY = osystem.settings().getInt("mdouble");
  _REPEAT_INITIAL_DELAY = osystem.settings().getInt("ctrldelay");
  setControllerRate(osystem.settings().getInt("ctrlrate"));
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::updateTime(uInt64 time)
{
  if(myDialogStack.empty())
    return;

  // We only need millisecond precision
  myTime = time / 1000;

  // Check for pending continuous events and send them to the active dialog box
  Dialog* activeDialog = myDialogStack.top();

  // Mouse button still pressed
  if(myCurrentMouseDown.b != MouseButton::NONE && myClickRepeatTime < myTime)
  {
    activeDialog->handleMouseDown(myCurrentMouseDown.x - activeDialog->_x,
                                  myCurrentMouseDown.y - activeDialog->_y,
                                  myCurrentMouseDown.b, 1);
    myClickRepeatTime = myTime + _REPEAT_SUSTAIN_DELAY;
  }

  // Joystick button still pressed
  if(myCurrentButtonDown.stick != -1 && myButtonRepeatTime < myTime)
  {
    activeDialog->handleJoyDown(myCurrentButtonDown.stick, myCurrentButtonDown.button);
    myButtonRepeatTime = myTime + _REPEAT_SUSTAIN_DELAY;
  }

  // Joystick has been pressed long
  if(myCurrentButtonDown.stick != -1 && myButtonLongPressTime < myTime)
  {
    activeDialog->handleJoyDown(myCurrentButtonDown.stick, myCurrentButtonDown.button, true);
    myButtonLongPressTime = myButtonRepeatTime = myTime + _REPEAT_NONE;
  }

  // Joystick axis still pressed
  if(myCurrentAxisDown.stick != -1 && myAxisRepeatTime < myTime)
  {
    activeDialog->handleJoyAxis(myCurrentAxisDown.stick, myCurrentAxisDown.axis,
                                myCurrentAxisDown.adir);
    myAxisRepeatTime = myTime + _REPEAT_SUSTAIN_DELAY;
  }

  // Joystick hat still pressed
  if(myCurrentHatDown.stick != -1 && myHatRepeatTime < myTime)
  {
    activeDialog->handleJoyHat(myCurrentHatDown.stick, myCurrentHatDown.hat,
                                myCurrentHatDown.hdir);
    myHatRepeatTime = myTime + _REPEAT_SUSTAIN_DELAY;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DialogContainer::draw(bool full)
{
  if(myDialogStack.empty())
    return false;

  // Make the top dialog dirty if a full redraw is requested
  if(full)
    myDialogStack.top()->setDirty();

  // If the top dialog is dirty, then all below it must be redrawn too
  const bool dirty = needsRedraw();

  myDialogStack.applyAll([&](Dialog*& d){
    if(dirty)
      d->setDirty();
    full |= d->render();
  });

  return full;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DialogContainer::needsRedraw() const
{
  return !myDialogStack.empty() ? myDialogStack.top()->isDirty() : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DialogContainer::baseDialogIsActive() const
{
  return myDialogStack.size() == 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int DialogContainer::addDialog(Dialog* d)
{
  const Common::Rect& r = myOSystem.frameBuffer().imageRect();
  const uInt32 scale = myOSystem.frameBuffer().hidpiScaleFactor();

  if(uInt32(d->getWidth() * scale) > r.w() || uInt32(d->getHeight() * scale) > r.h())
    myOSystem.frameBuffer().showMessage(
        "Unable to show dialog box; FIX THE CODE");
  else
  {
    d->setDirty();
    myDialogStack.push(d);
  }
  return myDialogStack.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::removeDialog()
{
  if(!myDialogStack.empty())
  {
    myDialogStack.pop();
    if(!myDialogStack.empty())
      myDialogStack.top()->setDirty();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::reStack()
{
  // Pop all items from the stack, and then add the base menu
  while(!myDialogStack.empty())
    myDialogStack.top()->close();

  baseDialog()->open();

  // Reset all continuous events
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::handleTextEvent(char text)
{
  if(myDialogStack.empty())
    return;

  // Send the event to the dialog box on the top of the stack
  Dialog* activeDialog = myDialogStack.top();
  activeDialog->handleText(text);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::handleKeyEvent(StellaKey key, StellaMod mod, bool pressed, bool repeated)
{
  if(myDialogStack.empty())
    return;

  // Send the event to the dialog box on the top of the stack
  Dialog* activeDialog = myDialogStack.top();
  if(pressed)
    activeDialog->handleKeyDown(key, mod, repeated);
  else
    activeDialog->handleKeyUp(key, mod);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::handleMouseMotionEvent(int x, int y)
{
  if(myDialogStack.empty())
    return;

  // Send the event to the dialog box on the top of the stack
  Dialog* activeDialog = myDialogStack.top();
  activeDialog->surface().translateCoords(x, y);
  activeDialog->handleMouseMoved(x - activeDialog->_x, y - activeDialog->_y);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::handleMouseButtonEvent(MouseButton b, bool pressed,
                                             int x, int y)
{
  if(myDialogStack.empty())
    return;

  // Send the event to the dialog box on the top of the stack
  Dialog* activeDialog = myDialogStack.top();
  activeDialog->surface().translateCoords(x, y);

  switch(b)
  {
    case MouseButton::LEFT:
    case MouseButton::RIGHT:
      if(pressed)
      {
        // If more than two clicks have been recorded, we start over
        if(myLastClick.count == 2)
        {
          myLastClick.x = myLastClick.y = 0;
          myLastClick.time = 0;
          myLastClick.count = 0;
        }

        if(myLastClick.count && (myTime < myLastClick.time + _DOUBLE_CLICK_DELAY)
           && std::abs(myLastClick.x - x) < 3
           && std::abs(myLastClick.y - y) < 3)
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

        // Now account for repeated mouse events (click and hold), but only
        // if the dialog wants them
        if(activeDialog->handleMouseClicks(x - activeDialog->_x, y - activeDialog->_y, b))
        {
          myCurrentMouseDown.x = x;
          myCurrentMouseDown.y = y;
          myCurrentMouseDown.b = b;
          myClickRepeatTime = myTime + _REPEAT_INITIAL_DELAY;
        }
        else
          myCurrentMouseDown.b = MouseButton::NONE;

        activeDialog->handleMouseDown(x - activeDialog->_x, y - activeDialog->_y,
                                      b, myLastClick.count);
      }
      else
      {
        activeDialog->handleMouseUp(x - activeDialog->_x, y - activeDialog->_y,
                                    b, myLastClick.count);

        if(b == myCurrentMouseDown.b)
          myCurrentMouseDown.b = MouseButton::NONE;
      }
      break;

    case MouseButton::WHEELUP:
      activeDialog->handleMouseWheel(x - activeDialog->_x, y - activeDialog->_y, -1);
      break;

    case MouseButton::WHEELDOWN:
      activeDialog->handleMouseWheel(x - activeDialog->_x, y - activeDialog->_y, 1);
      break;

    case MouseButton::NONE:  // should never get here
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::handleJoyBtnEvent(int stick, int button, bool pressed)
{
  if(myDialogStack.empty())
    return;

  // Send the event to the dialog box on the top of the stack
  Dialog* activeDialog = myDialogStack.top();

  if(pressed && myButtonRepeatTime < myTime) // prevent pending repeats after enabling repeat again
  {
    myCurrentButtonDown.stick  = stick;
    myCurrentButtonDown.button = button;
    myButtonRepeatTime = myTime + (activeDialog->repeatEnabled() ? _REPEAT_INITIAL_DELAY : _REPEAT_NONE);
    myButtonLongPressTime = myTime + _LONG_PRESS_DELAY;

    activeDialog->handleJoyDown(stick, button);
  }
  else
  {
    // Only stop firing events if it's the current button
    if(stick == myCurrentButtonDown.stick)
    {
      myCurrentButtonDown.stick = myCurrentButtonDown.button = -1;
      myButtonRepeatTime = myButtonLongPressTime = 0;
    }
    activeDialog->handleJoyUp(stick, button);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::handleJoyAxisEvent(int stick, JoyAxis axis, JoyDir adir, int button)
{
  if(myDialogStack.empty())
    return;

  // Send the event to the dialog box on the top of the stack
  Dialog* activeDialog = myDialogStack.top();

  // Prevent long button press in button/axis combinations
  myButtonLongPressTime = myTime + _REPEAT_NONE;

  // Only stop firing events if it's the current stick
  if(myCurrentAxisDown.stick == stick && adir == JoyDir::NONE)
  {
    myCurrentAxisDown.stick = -1;
    myCurrentAxisDown.axis = JoyAxis::NONE;
    myAxisRepeatTime = 0;
  }
  else if(adir != JoyDir::NONE && myAxisRepeatTime < myTime)  // never repeat the 'off' event; prevent pending repeats after enabling repeat again
  {
    // Now account for repeated axis events (press and hold)
    myCurrentAxisDown.stick = stick;
    myCurrentAxisDown.axis  = axis;
    myCurrentAxisDown.adir = adir;
    myAxisRepeatTime = myTime + (activeDialog->repeatEnabled() ? _REPEAT_INITIAL_DELAY : _REPEAT_NONE);
  }
  activeDialog->handleJoyAxis(stick, axis, adir, button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::handleJoyHatEvent(int stick, int hat, JoyHatDir hdir, int button)
{
  if(myDialogStack.empty())
    return;

  // Send the event to the dialog box on the top of the stack
  Dialog* activeDialog = myDialogStack.top();

  // Prevent long button press in button/hat combinations
  myButtonLongPressTime = myTime + _REPEAT_NONE;

  // Only stop firing events if it's the current stick
  if(myCurrentHatDown.stick == stick && hdir == JoyHatDir::CENTER)
  {
    myCurrentHatDown.stick = myCurrentHatDown.hat = -1;
    myHatRepeatTime = 0;
  }
  else if(hdir != JoyHatDir::CENTER && myHatRepeatTime < myTime)  // never repeat the 'center' direction; prevent pending repeats after enabling repeat again
  {
    // Now account for repeated hat events (press and hold)
    myCurrentHatDown.stick = stick;
    myCurrentHatDown.hat  = hat;
    myCurrentHatDown.hdir = hdir;
    myHatRepeatTime = myTime + (activeDialog->repeatEnabled() ? _REPEAT_INITIAL_DELAY : _REPEAT_NONE);
  }
  activeDialog->handleJoyHat(stick, hat, hdir, button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::reset()
{
  myCurrentMouseDown.b = MouseButton::NONE;
  myLastClick.x = myLastClick.y = 0;
  myLastClick.time = 0;
  myLastClick.count = 0;

  myCurrentButtonDown.stick = myCurrentButtonDown.button = -1;
  myCurrentAxisDown.stick = -1;
  myCurrentAxisDown.axis = JoyAxis::NONE;
  myCurrentHatDown.stick = myCurrentHatDown.hat = -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt64 DialogContainer::_DOUBLE_CLICK_DELAY = 500;
uInt64 DialogContainer::_REPEAT_INITIAL_DELAY = 400;
uInt64 DialogContainer::_REPEAT_SUSTAIN_DELAY = 50;

