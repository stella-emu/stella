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
// $Id: DialogContainer.cxx,v 1.8 2005-06-07 21:22:39 stephena Exp $
//============================================================================

#include "OSystem.hxx"
#include "Dialog.hxx"
#include "Stack.hxx"
#include "EventHandler.hxx"
#include "bspf.hxx"
#include "DialogContainer.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DialogContainer::DialogContainer(OSystem* osystem)
    : myOSystem(osystem),
      myBaseDialog(NULL),
      myTime(0)
{
  myCurrentKeyDown.keycode = 0;
  myCurrentMouseDown.button = -1;
  myLastClick.x = myLastClick.y = 0;
  myLastClick.time = 0;
  myLastClick.count = 0;
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

/*  FIXME - there are still some problems with this code
  if(myCurrentMouseDown.button != -1 && myClickRepeatTime < myTime)
  {
    activeDialog->handleMouseDown(myCurrentMouseDown.x - activeDialog->_x,
                                  myCurrentMouseDown.y - activeDialog->_y,
                                  myCurrentMouseDown.button, 1);
    myClickRepeatTime = myTime + kClickRepeatSustainDelay;
  }
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::draw()
{
  // Draw all the dialogs on the stack
  for(int i = 0; i < myDialogStack.size(); i++)
  {
    myDialogStack[i]->open();
    myDialogStack[i]->drawDialog();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::addDialog(Dialog* d)
{
  myDialogStack.push(d);
  myOSystem->frameBuffer().refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::removeDialog()
{
  if(!myDialogStack.empty())
  {
    myDialogStack.pop();
    myOSystem->frameBuffer().refresh();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::reStack()
{
  // Pop all items from the stack, and then add the base menu
  while(!myDialogStack.empty())
    myDialogStack.pop();
  myDialogStack.push(myBaseDialog);

  // Now make sure all dialog boxes are in a known (closed) state
  myBaseDialog->reset();

  // Reset all continuous events
  myCurrentKeyDown.keycode = 0;
  myCurrentMouseDown.button = -1;
  myLastClick.x = myLastClick.y = 0;
  myLastClick.time = 0;
  myLastClick.count = 0;
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::handleMouseButtonEvent(MouseButton b, int x, int y, uInt8 state)
{
  if(myDialogStack.empty())
    return;

  // Send the event to the dialog box on the top of the stack
  Dialog* activeDialog = myDialogStack.top();

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
      myCurrentMouseDown.button = 1;  // in the future, we may differentiate buttons
      myClickRepeatTime = myTime + kClickRepeatInitialDelay;

      activeDialog->handleMouseDown(x - activeDialog->_x, y - activeDialog->_y,
                                    1, myLastClick.count);
      break;

    case EVENT_LBUTTONUP:
    case EVENT_RBUTTONUP:
      activeDialog->handleMouseUp(x - activeDialog->_x, y - activeDialog->_y,
                                  1, myLastClick.count);
      // Since all buttons are treated equally, we don't need to check which button
      //if (button == myCurrentClickDown.button)
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
    activeDialog->handleJoyDown(stick, button);
  else
    activeDialog->handleJoyUp(stick, button);
}
