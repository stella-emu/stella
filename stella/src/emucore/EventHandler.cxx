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
// $Id: EventHandler.cxx,v 1.26 2004-06-13 04:57:17 bwmott Exp $
//============================================================================

#include <algorithm>
#include <sstream>

#include "Console.hxx"
#include "Event.hxx"
#include "EventHandler.hxx"
#include "MediaSrc.hxx"
#include "Settings.hxx"
#include "StellaEvent.hxx"
#include "System.hxx"
#include "FrameBuffer.hxx"
#include "Sound.hxx"
#include "bspf.hxx"

#ifdef SNAPSHOT_SUPPORT
  #include "Snapshot.hxx"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::EventHandler(Console* console)
    : myConsole(console),
      myCurrentState(0),
      myPauseStatus(false),
      myQuitStatus(false),
      myMenuStatus(false),
      myRemapEnabledFlag(true)
{
  // Create the event object which will be used for this handler
  myEvent = new Event();

  // Erase the KeyEvent array 
  for(Int32 i = 0; i < StellaEvent::LastKCODE; ++i)
    myKeyTable[i] = Event::NoType;

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
void EventHandler::sendKeyEvent(StellaEvent::KeyCode key, Int32 state)
{
  // First check if we are changing menu mode, and only change when not paused
  // Sound is paused when entering menu mode, but the framebuffer is kept active
  if(myRemapEnabledFlag && key == StellaEvent::KCODE_TAB && state == 1 && !myPauseStatus)
  {
    myMenuStatus = !myMenuStatus;
    myConsole->frameBuffer().showMenu(myMenuStatus);
    myConsole->sound().mute(myMenuStatus);
    return;
  }

  // Determine where the event should be sent
  if(myMenuStatus)
    myConsole->frameBuffer().sendKeyEvent(key, state);
  else
    sendEvent(myKeyTable[key], state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::sendJoyEvent(StellaEvent::JoyStick stick,
     StellaEvent::JoyCode code, Int32 state)
{
  // Determine where the event should be sent
  if(myMenuStatus)
    myConsole->frameBuffer().sendJoyEvent(stick, code, state);
  else
    sendEvent(myJoyTable[stick*StellaEvent::LastJCODE + code], state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::sendEvent(Event::Type event, Int32 state)
{
  // Ignore unmapped events
  if(event == Event::NoType)
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
    else if(event == Event::TakeSnapshot)
    {
      takeSnapshot();
      return;
    }
    else if(event == Event::Pause)
    {
      myPauseStatus = !myPauseStatus;
      myConsole->frameBuffer().pause(myPauseStatus);
      myConsole->sound().mute(myPauseStatus);
      return;
    }
    else if(event == Event::Quit)
    {
      myQuitStatus = !myQuitStatus;
      myConsole->settings().saveConfig();
      return;
    }

    if(ourMessageTable[event] != "")
      myConsole->frameBuffer().showMessage(ourMessageTable[event]);
  }

  // Otherwise, pass it to the emulation core
  myEvent->set(event, state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setKeymap()
{
  // Since istringstream swallows whitespace, we have to make the
  // delimiters be spaces
  string list = myConsole->settings().getString("keymap");
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
  string list = myConsole->settings().getString("joymap");
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
  myKeyTable[StellaEvent::KCODE_4]         = Event::BoosterGripZeroTrigger;
  myKeyTable[StellaEvent::KCODE_5]         = Event::BoosterGripZeroBooster;

  myKeyTable[StellaEvent::KCODE_y]         = Event::JoystickOneUp;
  myKeyTable[StellaEvent::KCODE_h]         = Event::JoystickOneDown;
  myKeyTable[StellaEvent::KCODE_g]         = Event::JoystickOneLeft;
  myKeyTable[StellaEvent::KCODE_j]         = Event::JoystickOneRight;
  myKeyTable[StellaEvent::KCODE_f]         = Event::JoystickOneFire;
  myKeyTable[StellaEvent::KCODE_6]         = Event::BoosterGripOneTrigger;
  myKeyTable[StellaEvent::KCODE_7]         = Event::BoosterGripOneBooster;

  myKeyTable[StellaEvent::KCODE_INSERT]    = Event::DrivingZeroCounterClockwise;
  myKeyTable[StellaEvent::KCODE_PAGEUP]    = Event::DrivingZeroClockwise;
  myKeyTable[StellaEvent::KCODE_HOME]      = Event::DrivingZeroFire;

  myKeyTable[StellaEvent::KCODE_DELETE]    = Event::DrivingOneCounterClockwise;
  myKeyTable[StellaEvent::KCODE_PAGEDOWN]  = Event::DrivingOneClockwise;
  myKeyTable[StellaEvent::KCODE_END]       = Event::DrivingOneFire;

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

#ifndef MAC_OSX
  myKeyTable[StellaEvent::KCODE_ESCAPE]    = Event::Quit;
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
  ostringstream buf;

  // Do a state save using the System
  string md5      = myConsole->properties().get("Cartridge.MD5");
  string filename = myConsole->settings().stateFilename(md5, myCurrentState);
  int result      = myConsole->system().saveState(filename, md5);

  // Print appropriate message
  buf.str("");
  if(result == 1)
    buf << "State " << myCurrentState << " saved";
  else if(result == 2)
    buf << "Error saving state " << myCurrentState;
  else if(result == 3)
    buf << "Invalid state " << myCurrentState << " file";

  myConsole->frameBuffer().showMessage(buf.str());
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

  myConsole->frameBuffer().showMessage(buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::loadState()
{
  ostringstream buf;

  // Do a state save using the System
  string md5      = myConsole->properties().get("Cartridge.MD5");
  string filename = myConsole->settings().stateFilename(md5, myCurrentState);
  int result      = myConsole->system().loadState(filename, md5);

  if(result == 1)
    buf << "State " << myCurrentState << " loaded";
  else if(result == 2)
    buf << "Error loading state " << myCurrentState;
  else if(result == 3)
    buf << "Invalid state " << myCurrentState << " file";

  myConsole->frameBuffer().showMessage(buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::takeSnapshot()
{
#ifdef SNAPSHOT_SUPPORT
  // Figure out the correct snapshot name
  string filename;
  string sspath = myConsole->settings().getString("ssdir");
  string ssname = myConsole->settings().getString("ssname");

  if(ssname == "romname")
    sspath = sspath + BSPF_PATH_SEPARATOR + myConsole->properties().get("Cartridge.Name");
  else if(ssname == "md5sum")
    sspath = sspath + BSPF_PATH_SEPARATOR + myConsole->properties().get("Cartridge.MD5");

  // Replace all spaces in name with underscores
  replace(sspath.begin(), sspath.end(), ' ', '_');

  // Check whether we want multiple snapshots created
  if(!myConsole->settings().getBool("sssingle"))
  {
    // Determine if the file already exists, checking each successive filename
    // until one doesn't exist
    filename = sspath + ".png";
    if(myConsole->settings().fileExists(filename))
    {
      ostringstream buf;
      for(uInt32 i = 1; ;++i)
      {
        buf.str("");
        buf << sspath << "_" << i << ".png";
        if(!myConsole->settings().fileExists(buf.str()))
          break;
      }
      filename = buf.str();
    }
  }
  else
    filename = sspath + ".png";

  // Now save the snapshot file
  uInt32 multiplier = myConsole->settings().getInt("zoom");

  myConsole->snapshot().savePNG(filename, multiplier);
  myConsole->frameBuffer().showMessage("Snapshot saved");
#else
  myConsole->frameBuffer().showMessage("Snapshots unsupported");
#endif
}
