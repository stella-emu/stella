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
// $Id: EventHandler.cxx,v 1.3 2003-09-04 23:23:06 stephena Exp $
//============================================================================

#include <sstream>

#include "Console.hxx"
#include "Event.hxx"
#include "EventHandler.hxx"
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
  {
    keyTable[i].type    = Event::LastType;
    keyTable[i].message = "";
  }

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
void EventHandler::sendKeyEvent(StellaEvent::KeyCode key,
     StellaEvent::KeyState state)
{
  // Ignore unmapped keys
  if(keyTable[key].type == Event::LastType)
    return;

  // Take care of special events that aren't technically part of
  // the emulation core
  if(state == StellaEvent::KSTATE_PRESSED)
  {
    if(keyTable[key].type == Event::SaveState)
    {
      saveState();
      return;
    }
    else if(keyTable[key].type == Event::ChangeState)
    {
      changeState();
      return;
    }
    else if(keyTable[key].type == Event::LoadState)
    {
      loadState();
      return;
    }
//    else if(keyTable[key].type == Event::TakeSnapshot)
//      cerr << "Event::TakeSnapshot received\n";

    if(keyTable[key].message != "")
      myMediaSource->showMessage(keyTable[key].message, 120);
  }

  // Otherwise, pass it to the emulation core
  myEvent->set(keyTable[key].type, state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultKeymap()
{
  keyTable[StellaEvent::KCODE_1].type         = Event::KeyboardZero1;
  keyTable[StellaEvent::KCODE_2].type         = Event::KeyboardZero2;
  keyTable[StellaEvent::KCODE_3].type         = Event::KeyboardZero3;
  keyTable[StellaEvent::KCODE_q].type         = Event::KeyboardZero4;
  keyTable[StellaEvent::KCODE_w].type         = Event::KeyboardZero5;
  keyTable[StellaEvent::KCODE_e].type         = Event::KeyboardZero6;
  keyTable[StellaEvent::KCODE_a].type         = Event::KeyboardZero7;
  keyTable[StellaEvent::KCODE_s].type         = Event::KeyboardZero8;
  keyTable[StellaEvent::KCODE_d].type         = Event::KeyboardZero9;
  keyTable[StellaEvent::KCODE_z].type         = Event::KeyboardZeroStar;
  keyTable[StellaEvent::KCODE_x].type         = Event::KeyboardZero0;
  keyTable[StellaEvent::KCODE_c].type         = Event::KeyboardZeroPound;

  keyTable[StellaEvent::KCODE_8].type         = Event::KeyboardOne1;
  keyTable[StellaEvent::KCODE_9].type         = Event::KeyboardOne2;
  keyTable[StellaEvent::KCODE_0].type         = Event::KeyboardOne3;
  keyTable[StellaEvent::KCODE_i].type         = Event::KeyboardOne4;
  keyTable[StellaEvent::KCODE_o].type         = Event::KeyboardOne5;
  keyTable[StellaEvent::KCODE_p].type         = Event::KeyboardOne6;
  keyTable[StellaEvent::KCODE_k].type         = Event::KeyboardOne7;
  keyTable[StellaEvent::KCODE_l].type         = Event::KeyboardOne8;
  keyTable[StellaEvent::KCODE_SEMICOLON].type = Event::KeyboardOne9;
  keyTable[StellaEvent::KCODE_COMMA].type     = Event::KeyboardOneStar;
  keyTable[StellaEvent::KCODE_PERIOD].type    = Event::KeyboardOne0;
  keyTable[StellaEvent::KCODE_SLASH].type     = Event::KeyboardOnePound;

  keyTable[StellaEvent::KCODE_UP].type        = Event::JoystickZeroUp;
  keyTable[StellaEvent::KCODE_DOWN].type      = Event::JoystickZeroDown;
  keyTable[StellaEvent::KCODE_LEFT].type      = Event::JoystickZeroLeft;
  keyTable[StellaEvent::KCODE_RIGHT].type     = Event::JoystickZeroRight;
  keyTable[StellaEvent::KCODE_SPACE].type     = Event::JoystickZeroFire;
//  keyTable[StellaEvent::KCODE_].type         = Event::BoosterGripZeroTrigger;
//  keyTable[StellaEvent::KCODE_].type         = Event::BoosterGripZeroBooster;

  keyTable[StellaEvent::KCODE_y].type         = Event::JoystickOneUp;
  keyTable[StellaEvent::KCODE_h].type         = Event::JoystickOneDown;
  keyTable[StellaEvent::KCODE_g].type         = Event::JoystickOneLeft;
  keyTable[StellaEvent::KCODE_j].type         = Event::JoystickOneRight;
  keyTable[StellaEvent::KCODE_f].type         = Event::JoystickOneFire;
//  keyTable[StellaEvent::KCODE_].type         = Event::BoosterGripOneTrigger;
//  keyTable[StellaEvent::KCODE_].type         = Event::BoosterGripOneBooster;

  keyTable[StellaEvent::KCODE_F1].type        = Event::ConsoleSelect;
  keyTable[StellaEvent::KCODE_F2].type        = Event::ConsoleReset;
  keyTable[StellaEvent::KCODE_F3].type        = Event::ConsoleColor;
  keyTable[StellaEvent::KCODE_F4].type        = Event::ConsoleBlackWhite;
  keyTable[StellaEvent::KCODE_F5].type        = Event::ConsoleLeftDifficultyA;
  keyTable[StellaEvent::KCODE_F6].type        = Event::ConsoleLeftDifficultyB;
  keyTable[StellaEvent::KCODE_F7].type        = Event::ConsoleRightDifficultyA;
  keyTable[StellaEvent::KCODE_F8].type        = Event::ConsoleRightDifficultyB;
  keyTable[StellaEvent::KCODE_F9].type        = Event::SaveState;
  keyTable[StellaEvent::KCODE_F10].type       = Event::ChangeState;
  keyTable[StellaEvent::KCODE_F11].type       = Event::LoadState;
  keyTable[StellaEvent::KCODE_F12].type       = Event::TakeSnapshot;

  keyTable[StellaEvent::KCODE_F3].message      = "Color Mode";
  keyTable[StellaEvent::KCODE_F4].message      = "BW Mode";
  keyTable[StellaEvent::KCODE_F5].message      = "Left Difficulty A";
  keyTable[StellaEvent::KCODE_F6].message      = "Left Difficulty B";
  keyTable[StellaEvent::KCODE_F7].message      = "Right Difficulty A";
  keyTable[StellaEvent::KCODE_F8].message      = "Right Difficulty B";

#if 0
      DrivingZeroClockwise, DrivingZeroCounterClockwise, DrivingZeroFire,
      DrivingOneClockwise, DrivingOneCounterClockwise, DrivingOneFire,
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultJoymap()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveState()
{
  ostringstream buf;
  string md5 = myConsole->properties().get("Cartridge.MD5");
//  string filename = frontend->stateFilename(md5, myCurrentState);

// FIXME !!!  This MUST be changed to reflect the frontend/OS being used
  string stateDir = "/home/stephena/.stella/state/";
  buf << stateDir << md5 << ".st" << myCurrentState;
  string filename = buf.str();
/////////////////////////////////////////////////////////

  // Do a state save using the System
  int result = myConsole->system().saveState(filename, md5);

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
  string md5 = myConsole->properties().get("Cartridge.MD5");
//  string filename = frontend->stateFilename(md5, myCurrentState);

// FIXME !!!  This MUST be changed to reflect the frontend/OS being used
  string stateDir = "/home/stephena/.stella/state/";
  buf << stateDir << md5 << ".st" << myCurrentState;
  string filename = buf.str();
/////////////////////////////////////////////////////////

  // Do a state save using the System
  int result = myConsole->system().loadState(filename, md5);

  // Print appropriate message
  buf.str("");
  if(result == 1)
    buf << "State " << myCurrentState << " loaded";
  else if(result == 2)
    buf << "Error loading state " << myCurrentState;
  else if(result == 3)
    buf << "Invalid state " << myCurrentState << " file";

  string message = buf.str();
  myMediaSource->showMessage(message, 120);
}
