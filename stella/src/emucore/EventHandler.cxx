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
// Copyright (c) 1995-2004 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: EventHandler.cxx,v 1.35 2005-02-27 23:41:18 stephena Exp $
//============================================================================

#include <algorithm>
#include <sstream>
#include <SDL.h>

#include "Event.hxx"
#include "EventHandler.hxx"
#include "Settings.hxx"
#include "StellaEvent.hxx"
#include "System.hxx"
#include "FrameBuffer.hxx"
#include "Sound.hxx"
#include "OSystem.hxx"
#include "bspf.hxx"

#ifdef SNAPSHOT_SUPPORT
  #include "Snapshot.hxx"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::EventHandler(OSystem* osystem)
    : myOSystem(osystem),
      myState(S_NONE),
      myLSState(0),
      myPauseFlag(false),
      myExitGameFlag(false),
      myQuitFlag(false)
{
  // Add this eventhandler object to the OSystem
  myOSystem->attach(this);

  // Create the event object which will be used for this handler
  myEvent = new Event();

  // Erase the KeyEvent arrays
  for(Int32 i = 0; i < 256; ++i)
  {
    myKeyTable[i]     = Event::NoType;
    myAltKeyTable[i]  = Event::NoType;
    myCtrlKeyTable[i] = Event::NoType;
  }

  // Erase the JoyEvent array
  for(Int32 i = 0; i < StellaEvent::LastJSTICK*StellaEvent::LastJCODE; ++i)
    myJoyTable[i] = Event::NoType;

  // Erase the Message array 
  for(Int32 i = 0; i < Event::LastType; ++i)
    ourMessageTable[i] = "";

  // Set unchanging messages
  ourMessageTable[Event::ConsoleColor]            = "Color Mode";
  ourMessageTable[Event::ConsoleBlackWhite]       = "BW Mode";
  ourMessageTable[Event::ConsoleLeftDifficultyA]  = "Left Difficulty A";
  ourMessageTable[Event::ConsoleLeftDifficultyB]  = "Left Difficulty B";
  ourMessageTable[Event::ConsoleRightDifficultyA] = "Right Difficulty A";
  ourMessageTable[Event::ConsoleRightDifficultyB] = "Right Difficulty B";

  setKeymap();
  setJoymap();
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
void EventHandler::reset(State state)
{
  myState = state;
  myLSState = 0;
  myPauseFlag = false;
  myExitGameFlag = false;
  myQuitFlag = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleKeyEvent(SDLKey key, SDLMod mod, uInt8 state)
{
  // Handle keys here that are accessible no matter which mode we're in

  // Toggle menu mode
  if(key == SDLK_TAB && state == 1 && !myPauseFlag)
  {
    if(myState == S_EMULATE)
    {
      myState = S_MENU;
      myOSystem->frameBuffer().showMenu(true);  // FIXME - move this into gui class
      myOSystem->sound().mute(true);
      return;
    }
    else if(myState == S_MENU)
    {
      myState = S_EMULATE;
      myOSystem->frameBuffer().showMenu(false);  // FIXME - move this into gui class
      myOSystem->sound().mute(false);
      return;
    }
  }

  // Determine which mode we're in, then send the event to the appropriate place
  switch(myState)
  {
    case S_EMULATE:
//      if(mod & KMOD_ALT && state)
//        handleEvent(myAltKeyTable[key], state);
//      else if(mod & KMOD_CTRL && state)
//        handleEvent(myCtrlKeyTable[key], state);
//      else
      handleEvent(myKeyTable[key], state);
      break;

    case S_MENU:
//FIXME      myOSystem->gui().menu().handleKeyEvent(key, mod, state);
      break;

    case S_BROWSER:
//FIXME      myOSystem->gui().browser().handleKeyEvent(key, mod, state);
      break;

    case S_DEBUGGER:
      // Not yet implemented
      break;

    case S_NONE:
      return;
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::sendJoyEvent(StellaEvent::JoyStick stick,
     StellaEvent::JoyCode code, Int32 state)
{
// FIXME
/*
  // Determine where the event should be sent
  if(myMenuStatus)
    myOSystem->frameBuffer().sendJoyEvent(stick, code, state);
  else
    handleEvent(myJoyTable[stick*StellaEvent::LastJCODE + code], state);
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleEvent(Event::Type event, Int32 state)
{
  // Ignore unmapped events
  if(event == Event::NoType)
    return;

  // Take care of special events that aren't part of the emulation core
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
    else if(event == Event::TakeSnapshot)
    {
      takeSnapshot();
      return;
    }
    else if(event == Event::Pause)
    {
      myPauseFlag = !myPauseFlag;
      myOSystem->frameBuffer().pause(myPauseFlag);
      myOSystem->sound().mute(myPauseFlag);
      return;
    }
    else if(event == Event::ExitGame)
    {
      myExitGameFlag = true;
      myOSystem->sound().mute(true);
      return;
    }
    else if(event == Event::Quit)
    {
      myQuitFlag = true;
      myOSystem->settings().saveConfig();
      return;
    }

    if(ourMessageTable[event] != "")
      myOSystem->frameBuffer().showMessage(ourMessageTable[event]);
  }

  // Otherwise, pass it to the emulation core
  myEvent->set(event, state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setKeymap()
{
  // Since istringstream swallows whitespace, we have to make the
  // delimiters be spaces
  string list = myOSystem->settings().getString("keymap");
  replace(list.begin(), list.end(), ':', ' ');

  if(isValidList(list, StellaEvent::LastKCODE))
  {
    istringstream buf(list);
    string key;

    // Fill the keymap table with events
    for(Int32 i = 0; i < StellaEvent::LastKCODE; ++i)
    {
      buf >> key;
      myKeyTable[i] = (Event::Type) atoi(key.c_str());
    }
  }
  else
    setDefaultKeymap();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setJoymap()
{
  // Since istringstream swallows whitespace, we have to make the
  // delimiters be spaces
  string list = myOSystem->settings().getString("joymap");
  replace(list.begin(), list.end(), ':', ' ');

  if(isValidList(list, StellaEvent::LastJSTICK*StellaEvent::LastJCODE))
  {
    istringstream buf(list);
    string key;

    // Fill the joymap table with events
    for(Int32 i = 0; i < StellaEvent::LastJSTICK*StellaEvent::LastJCODE; ++i)
    {
      buf >> key;
      myJoyTable[i] = (Event::Type) atoi(key.c_str());
    }
  }
  else
    setDefaultJoymap();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::getKeymapArray(Event::Type** array, uInt32* size)
{
  *array = myKeyTable;
  *size  = StellaEvent::LastKCODE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::getJoymapArray(Event::Type** array, uInt32* size)
{
  *array = myJoyTable;
  *size  = StellaEvent::LastJSTICK * StellaEvent::LastJCODE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultKeymap()
{
  myKeyTable[ SDLK_1 ]         = Event::KeyboardZero1;
  myKeyTable[ SDLK_2 ]         = Event::KeyboardZero2;
  myKeyTable[ SDLK_3 ]         = Event::KeyboardZero3;
  myKeyTable[ SDLK_q ]         = Event::KeyboardZero4;
  myKeyTable[ SDLK_w ]         = Event::KeyboardZero5;
  myKeyTable[ SDLK_e ]         = Event::KeyboardZero6;
  myKeyTable[ SDLK_a ]         = Event::KeyboardZero7;
  myKeyTable[ SDLK_s ]         = Event::KeyboardZero8;
  myKeyTable[ SDLK_d ]         = Event::KeyboardZero9;
  myKeyTable[ SDLK_z ]         = Event::KeyboardZeroStar;
  myKeyTable[ SDLK_x ]         = Event::KeyboardZero0;
  myKeyTable[ SDLK_c ]         = Event::KeyboardZeroPound;

  myKeyTable[ SDLK_8 ]         = Event::KeyboardOne1;
  myKeyTable[ SDLK_9 ]         = Event::KeyboardOne2;
  myKeyTable[ SDLK_0 ]         = Event::KeyboardOne3;
  myKeyTable[ SDLK_i ]         = Event::KeyboardOne4;
  myKeyTable[ SDLK_o ]         = Event::KeyboardOne5;
  myKeyTable[ SDLK_p ]         = Event::KeyboardOne6;
  myKeyTable[ SDLK_k ]         = Event::KeyboardOne7;
  myKeyTable[ SDLK_l ]         = Event::KeyboardOne8;
  myKeyTable[ SDLK_SEMICOLON ] = Event::KeyboardOne9;
  myKeyTable[ SDLK_COMMA ]     = Event::KeyboardOneStar;
  myKeyTable[ SDLK_PERIOD ]    = Event::KeyboardOne0;
  myKeyTable[ SDLK_SLASH ]     = Event::KeyboardOnePound;

  myKeyTable[ SDLK_UP ]        = Event::JoystickZeroUp;
  myKeyTable[ SDLK_DOWN ]      = Event::JoystickZeroDown;
  myKeyTable[ SDLK_LEFT ]      = Event::JoystickZeroLeft;
  myKeyTable[ SDLK_RIGHT ]     = Event::JoystickZeroRight;
  myKeyTable[ SDLK_SPACE ]     = Event::JoystickZeroFire;
  myKeyTable[ SDLK_4 ]         = Event::BoosterGripZeroTrigger;
  myKeyTable[ SDLK_5 ]         = Event::BoosterGripZeroBooster;

  myKeyTable[ SDLK_y ]         = Event::JoystickOneUp;
  myKeyTable[ SDLK_h ]         = Event::JoystickOneDown;
  myKeyTable[ SDLK_g ]         = Event::JoystickOneLeft;
  myKeyTable[ SDLK_j ]         = Event::JoystickOneRight;
  myKeyTable[ SDLK_f ]         = Event::JoystickOneFire;
  myKeyTable[ SDLK_6 ]         = Event::BoosterGripOneTrigger;
  myKeyTable[ SDLK_7 ]         = Event::BoosterGripOneBooster;

  myKeyTable[ SDLK_INSERT ]    = Event::DrivingZeroCounterClockwise;
  myKeyTable[ SDLK_PAGEUP ]    = Event::DrivingZeroClockwise;
  myKeyTable[ SDLK_HOME ]      = Event::DrivingZeroFire;

  myKeyTable[ SDLK_DELETE ]    = Event::DrivingOneCounterClockwise;
  myKeyTable[ SDLK_PAGEDOWN ]  = Event::DrivingOneClockwise;
  myKeyTable[ SDLK_END ]       = Event::DrivingOneFire;

  myKeyTable[ SDLK_F1 ]        = Event::ConsoleSelect;
  myKeyTable[ SDLK_F2 ]        = Event::ConsoleReset;
  myKeyTable[ SDLK_F3 ]        = Event::ConsoleColor;
  myKeyTable[ SDLK_F4 ]        = Event::ConsoleBlackWhite;
  myKeyTable[ SDLK_F5 ]        = Event::ConsoleLeftDifficultyA;
  myKeyTable[ SDLK_F6 ]        = Event::ConsoleLeftDifficultyB;
  myKeyTable[ SDLK_F7 ]        = Event::ConsoleRightDifficultyA;
  myKeyTable[ SDLK_F8 ]        = Event::ConsoleRightDifficultyB;
  myKeyTable[ SDLK_F9 ]        = Event::SaveState;
  myKeyTable[ SDLK_F10 ]       = Event::ChangeState;
  myKeyTable[ SDLK_F11 ]       = Event::LoadState;
  myKeyTable[ SDLK_F12 ]       = Event::TakeSnapshot;

  myKeyTable[ SDLK_PAUSE ]     = Event::Pause;
#ifndef MAC_OSX
  myKeyTable[ SDLK_ESCAPE ]    = Event::ExitGame;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultJoymap()
{
  // Left joystick
  uInt32 i = StellaEvent::JSTICK_0 * StellaEvent::LastJCODE;
  myJoyTable[i + StellaEvent::JAXIS_UP]    = Event::JoystickZeroUp;
  myJoyTable[i + StellaEvent::JAXIS_DOWN]  = Event::JoystickZeroDown;
  myJoyTable[i + StellaEvent::JAXIS_LEFT]  = Event::JoystickZeroLeft;
  myJoyTable[i + StellaEvent::JAXIS_RIGHT] = Event::JoystickZeroRight;
  myJoyTable[i + StellaEvent::JBUTTON_0]   = Event::JoystickZeroFire;

  // Right joystick
  i = StellaEvent::JSTICK_1 * StellaEvent::LastJCODE;
  myJoyTable[i + StellaEvent::JAXIS_UP]    = Event::JoystickOneUp;
  myJoyTable[i + StellaEvent::JAXIS_DOWN]  = Event::JoystickOneDown;
  myJoyTable[i + StellaEvent::JAXIS_LEFT]  = Event::JoystickOneLeft;
  myJoyTable[i + StellaEvent::JAXIS_RIGHT] = Event::JoystickOneRight;
  myJoyTable[i + StellaEvent::JBUTTON_0]   = Event::JoystickOneFire;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::isValidList(string list, uInt32 length)
{
  // Rudimentary check to see if the list contains 'length' keys
  istringstream buf(list);
  string key;
  uInt32 i = 0;

  while(buf >> key)
    i++;

  return (i == length);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveState()
{
  // Do a state save using the System
  string md5      = myOSystem->console().properties().get("Cartridge.MD5");
  string filename = myOSystem->stateFilename(md5, myLSState);
  int result      = myOSystem->console().system().saveState(filename, md5);

  // Print appropriate message
  ostringstream buf;
  if(result == 1)
    buf << "State " << myLSState << " saved";
  else if(result == 2)
    buf << "Error saving state " << myLSState;
  else if(result == 3)
    buf << "Invalid state " << myLSState << " file";

  myOSystem->frameBuffer().showMessage(buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::changeState()
{
  if(myLSState == 9)
    myLSState = 0;
  else
    ++myLSState;

  // Print appropriate message
  ostringstream buf;
  buf << "Changed to slot " << myLSState;

  myOSystem->frameBuffer().showMessage(buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::loadState()
{
  // Do a state save using the System
  string md5      = myOSystem->console().properties().get("Cartridge.MD5");
  string filename = myOSystem->stateFilename(md5, myLSState);
  int result      = myOSystem->console().system().loadState(filename, md5);

  // Print appropriate message
  ostringstream buf;
  if(result == 1)
    buf << "State " << myLSState << " loaded";
  else if(result == 2)
    buf << "Error loading state " << myLSState;
  else if(result == 3)
    buf << "Invalid state " << myLSState << " file";

  myOSystem->frameBuffer().showMessage(buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::takeSnapshot()
{
#ifdef SNAPSHOT_SUPPORT
  // Figure out the correct snapshot name
  string filename;
  string sspath = myOSystem->settings().getString("ssdir");
  string ssname = myOSystem->settings().getString("ssname");

  if(ssname == "romname")
    sspath = sspath + BSPF_PATH_SEPARATOR +
             myOSystem->console().properties().get("Cartridge.Name");
  else if(ssname == "md5sum")
    sspath = sspath + BSPF_PATH_SEPARATOR +
             myOSystem->console().properties().get("Cartridge.MD5");

  // Check whether we want multiple snapshots created
  if(!myOSystem->settings().getBool("sssingle"))
  {
    // Determine if the file already exists, checking each successive filename
    // until one doesn't exist
    filename = sspath + ".png";
    if(myOSystem->fileExists(filename))
    {
      ostringstream buf;
      for(uInt32 i = 1; ;++i)
      {
        buf.str("");
        buf << sspath << "_" << i << ".png";
        if(!myOSystem->fileExists(buf.str()))
          break;
      }
      filename = buf.str();
    }
  }
  else
    filename = sspath + ".png";

  // Now create a Snapshot object and save the PNG
  myOSystem->frameBuffer().refresh(true);
  Snapshot snapshot(myOSystem->frameBuffer());
  string result = snapshot.savePNG(filename);
  myOSystem->frameBuffer().showMessage(result);
#else
  myOSystem->frameBuffer().showMessage("Snapshots unsupported");
#endif
}
