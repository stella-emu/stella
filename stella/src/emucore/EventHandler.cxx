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
// Copyright (c) 1995-1998 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: EventHandler.cxx,v 1.4 2003-09-06 21:17:48 stephena Exp $
//============================================================================

#include <sstream>

#include "Console.hxx"
#include "Event.hxx"
#include "EventHandler.hxx"
#include "Frontend.hxx"
#include "MediaSrc.hxx"
#include "StellaEvent.hxx"
#include "System.hxx"
#include "bspf.hxx"


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::EventHandler(Console* console)
    : myConsole(console),
      myCurrentState(0)
{
  // Create the event object which will be used for this handler
  myEvent = new Event();

  // Erase the KeyEvent array 
  for(Int32 i = 0; i < StellaEvent::LastKCODE; ++i)
    myKeyTable[i] = Event::LastType;

  // Erase the JoyEvent array
  for(Int32 i = 0; i < StellaEvent::LastJSTICK; ++i)
    for(Int32 j = 0; j < StellaEvent::LastJCODE; ++j)
      myJoyTable[i][j] = Event::LastType;

  // Erase the Message array 
  for(Int32 i = 0; i < Event::LastType; ++i)
    myMessageTable[i] = "";

  // Set unchanging messages
  myMessageTable[Event::ConsoleColor]            = "Color Mode";
  myMessageTable[Event::ConsoleBlackWhite]       = "BW Mode";
  myMessageTable[Event::ConsoleLeftDifficultyA]  = "Left Difficulty A";
  myMessageTable[Event::ConsoleLeftDifficultyB]  = "Left Difficulty B";
  myMessageTable[Event::ConsoleRightDifficultyA] = "Right Difficulty A";
  myMessageTable[Event::ConsoleRightDifficultyB] = "Right Difficulty B";

  setDefaultKeymap();
  setDefaultJoymap();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::~EventHandler()
{
  if(myEvent)
    delete myEvent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event* EventHandler::event()
{
  return myEvent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setMediaSource(MediaSource* mediaSource)
{
  myMediaSource = mediaSource;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::sendKeyEvent(StellaEvent::KeyCode key, Int32 state)
{
  Event::Type event = myKeyTable[key];

  // Ignore unmapped events
  if(event == Event::LastType)
    return;

  // Take care of special events that aren't technically part of
  // the emulation core
  if(state == 1)
  {
    if(event == Event::SaveState)
    {
      saveState();
      return;
    }
    else if(event == Event::ChangeState)
    {
      changeState();
      return;
    }
    else if(event == Event::LoadState)
    {
      loadState();
      return;
    }
//    else if(event == Event::TakeSnapshot)
//    {
//      FIXME ... make a call to takeSnapshot()
//      return;
//    }
    else if(event == Event::Pause)
    {
      myConsole->frontend().setPauseEvent();
      return;
    }
    else if(event == Event::Quit)
    {
      myConsole->frontend().setQuitEvent();
      return;
    }

    if(myMessageTable[event] != "")
      myMediaSource->showMessage(myMessageTable[event], 120);
  }

  // Otherwise, pass it to the emulation core
  myEvent->set(event, state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::sendJoyEvent(StellaEvent::JoyStick stick,
     StellaEvent::JoyCode code, Int32 state)
{
  Event::Type event = myJoyTable[stick][code];

  // Ignore unmapped events
  if(event == Event::LastType)
    return;

  // Take care of special events that aren't technically part of
  // the emulation core
  if(state == 1)
  {
    if(event == Event::SaveState)
    {
      saveState();
      return;
    }
    else if(event == Event::ChangeState)
    {
      changeState();
      return;
    }
    else if(event == Event::LoadState)
    {
      loadState();
      return;
    }
//    else if(event == Event::TakeSnapshot)
//    {
//      FIXME ... make a call to takeSnapshot()
//      return;
//    }
    else if(event == Event::Pause)
    {
      myConsole->frontend().setPauseEvent();
      return;
    }
    else if(event == Event::Quit)
    {
      myConsole->frontend().setQuitEvent();
      return;
    }

    if(myMessageTable[event] != "")
      myMediaSource->showMessage(myMessageTable[event], 120);
  }

  // Otherwise, pass it to the emulation core
  myEvent->set(event, state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::sendEvent(Event::Type type, Int32 value)
{
  myEvent->set(type, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultKeymap()
{
  myKeyTable[StellaEvent::KCODE_1]         = Event::KeyboardZero1;
  myKeyTable[StellaEvent::KCODE_2]         = Event::KeyboardZero2;
  myKeyTable[StellaEvent::KCODE_3]         = Event::KeyboardZero3;
  myKeyTable[StellaEvent::KCODE_q]         = Event::KeyboardZero4;
  myKeyTable[StellaEvent::KCODE_w]         = Event::KeyboardZero5;
  myKeyTable[StellaEvent::KCODE_e]         = Event::KeyboardZero6;
  myKeyTable[StellaEvent::KCODE_a]         = Event::KeyboardZero7;
  myKeyTable[StellaEvent::KCODE_s]         = Event::KeyboardZero8;
  myKeyTable[StellaEvent::KCODE_d]         = Event::KeyboardZero9;
  myKeyTable[StellaEvent::KCODE_z]         = Event::KeyboardZeroStar;
  myKeyTable[StellaEvent::KCODE_x]         = Event::KeyboardZero0;
  myKeyTable[StellaEvent::KCODE_c]         = Event::KeyboardZeroPound;

  myKeyTable[StellaEvent::KCODE_8]         = Event::KeyboardOne1;
  myKeyTable[StellaEvent::KCODE_9]         = Event::KeyboardOne2;
  myKeyTable[StellaEvent::KCODE_0]         = Event::KeyboardOne3;
  myKeyTable[StellaEvent::KCODE_i]         = Event::KeyboardOne4;
  myKeyTable[StellaEvent::KCODE_o]         = Event::KeyboardOne5;
  myKeyTable[StellaEvent::KCODE_p]         = Event::KeyboardOne6;
  myKeyTable[StellaEvent::KCODE_k]         = Event::KeyboardOne7;
  myKeyTable[StellaEvent::KCODE_l]         = Event::KeyboardOne8;
  myKeyTable[StellaEvent::KCODE_SEMICOLON] = Event::KeyboardOne9;
  myKeyTable[StellaEvent::KCODE_COMMA]     = Event::KeyboardOneStar;
  myKeyTable[StellaEvent::KCODE_PERIOD]    = Event::KeyboardOne0;
  myKeyTable[StellaEvent::KCODE_SLASH]     = Event::KeyboardOnePound;

  myKeyTable[StellaEvent::KCODE_UP]        = Event::JoystickZeroUp;
  myKeyTable[StellaEvent::KCODE_DOWN]      = Event::JoystickZeroDown;
  myKeyTable[StellaEvent::KCODE_LEFT]      = Event::JoystickZeroLeft;
  myKeyTable[StellaEvent::KCODE_RIGHT]     = Event::JoystickZeroRight;
  myKeyTable[StellaEvent::KCODE_SPACE]     = Event::JoystickZeroFire;
//  myKeyTable[StellaEvent::KCODE_]         = Event::BoosterGripZeroTrigger;
//  myKeyTable[StellaEvent::KCODE_]         = Event::BoosterGripZeroBooster;

  myKeyTable[StellaEvent::KCODE_y]         = Event::JoystickOneUp;
  myKeyTable[StellaEvent::KCODE_h]         = Event::JoystickOneDown;
  myKeyTable[StellaEvent::KCODE_g]         = Event::JoystickOneLeft;
  myKeyTable[StellaEvent::KCODE_j]         = Event::JoystickOneRight;
  myKeyTable[StellaEvent::KCODE_f]         = Event::JoystickOneFire;
//  myKeyTable[StellaEvent::KCODE_]         = Event::BoosterGripOneTrigger;
//  myKeyTable[StellaEvent::KCODE_]         = Event::BoosterGripOneBooster;

  myKeyTable[StellaEvent::KCODE_F1]        = Event::ConsoleSelect;
  myKeyTable[StellaEvent::KCODE_F2]        = Event::ConsoleReset;
  myKeyTable[StellaEvent::KCODE_F3]        = Event::ConsoleColor;
  myKeyTable[StellaEvent::KCODE_F4]        = Event::ConsoleBlackWhite;
  myKeyTable[StellaEvent::KCODE_F5]        = Event::ConsoleLeftDifficultyA;
  myKeyTable[StellaEvent::KCODE_F6]        = Event::ConsoleLeftDifficultyB;
  myKeyTable[StellaEvent::KCODE_F7]        = Event::ConsoleRightDifficultyA;
  myKeyTable[StellaEvent::KCODE_F8]        = Event::ConsoleRightDifficultyB;
  myKeyTable[StellaEvent::KCODE_F9]        = Event::SaveState;
  myKeyTable[StellaEvent::KCODE_F10]       = Event::ChangeState;
  myKeyTable[StellaEvent::KCODE_F11]       = Event::LoadState;
  myKeyTable[StellaEvent::KCODE_F12]       = Event::TakeSnapshot;

  myKeyTable[StellaEvent::KCODE_PAUSE]     = Event::Pause;
  myKeyTable[StellaEvent::KCODE_ESCAPE]    = Event::Quit;

#if 0
      DrivingZeroClockwise, DrivingZeroCounterClockwise, DrivingZeroFire,
      DrivingOneClockwise, DrivingOneCounterClockwise, DrivingOneFire,
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultJoymap()
{
  // Left joystick
  myJoyTable[StellaEvent::JSTICK_0][StellaEvent::JAXIS_UP]    = Event::JoystickZeroUp;
  myJoyTable[StellaEvent::JSTICK_0][StellaEvent::JAXIS_DOWN]  = Event::JoystickZeroDown;
  myJoyTable[StellaEvent::JSTICK_0][StellaEvent::JAXIS_LEFT]  = Event::JoystickZeroLeft;
  myJoyTable[StellaEvent::JSTICK_0][StellaEvent::JAXIS_RIGHT] = Event::JoystickZeroRight;
  myJoyTable[StellaEvent::JSTICK_0][StellaEvent::JBUTTON_0]   = Event::JoystickZeroFire;

  // Right joystick
  myJoyTable[StellaEvent::JSTICK_1][StellaEvent::JAXIS_UP]    = Event::JoystickOneUp;
  myJoyTable[StellaEvent::JSTICK_1][StellaEvent::JAXIS_DOWN]  = Event::JoystickOneDown;
  myJoyTable[StellaEvent::JSTICK_1][StellaEvent::JAXIS_LEFT]  = Event::JoystickOneLeft;
  myJoyTable[StellaEvent::JSTICK_1][StellaEvent::JAXIS_RIGHT] = Event::JoystickOneRight;
  myJoyTable[StellaEvent::JSTICK_1][StellaEvent::JBUTTON_0]   = Event::JoystickOneFire;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveState()
{
  ostringstream buf;

  // Do a state save using the System
  string md5      = myConsole->properties().get("Cartridge.MD5");
  string filename = myConsole->frontend().stateFilename(md5, myCurrentState);
  int result      = myConsole->system().saveState(filename, md5);

  // Print appropriate message
  buf.str("");
  if(result == 1)
    buf << "State " << myCurrentState << " saved";
  else if(result == 2)
    buf << "Error saving state " << myCurrentState;
  else if(result == 3)
    buf << "Invalid state " << myCurrentState << " file";

  string message = buf.str();
  myMediaSource->showMessage(message, 120);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::changeState()
{
  if(myCurrentState == 9)
    myCurrentState = 0;
  else
    ++myCurrentState;

  // Print appropriate message
  ostringstream buf;
  buf << "Changed to slot " << myCurrentState;
  string message = buf.str();
  myMediaSource->showMessage(message, 120);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::loadState()
{
  ostringstream buf;

  // Do a state save using the System
  string md5      = myConsole->properties().get("Cartridge.MD5");
  string filename = myConsole->frontend().stateFilename(md5, myCurrentState);
  int result      = myConsole->system().loadState(filename, md5);

  if(result == 1)
    buf << "State " << myCurrentState << " loaded";
  else if(result == 2)
    buf << "Error loading state " << myCurrentState;
  else if(result == 3)
    buf << "Invalid state " << myCurrentState << " file";

  string message = buf.str();
  myMediaSource->showMessage(message, 120);
}
