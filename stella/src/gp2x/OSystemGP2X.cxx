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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
// Modified on 2006/01/06 by Alex Zaballa for use on GP2X
//============================================================================

#include <cstdlib>
#include <sstream>
#include <fstream>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "bspf.hxx"
#include "OSystem.hxx"
#include "OSystemGP2X.hxx"

#ifdef HAVE_GETTIMEOFDAY
  #include <time.h>
  #include <sys/time.h>
#endif

/**
  Each derived class is responsible for calling the following methods
  in its constructor:

  setBaseDir()
  setStateDir()
  setPropertiesDir()
  setConfigFile()
  setCacheFile()

  See OSystem.hxx for a further explanation
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemGP2X::OSystemGP2X() : OSystem()
{
  // GP2X needs all config files in exec directory
  
  char *currdir = getcwd(NULL, 0);
  string basedir = currdir;
  free(currdir);
  setBaseDir(basedir);
  
  setConfigFile(basedir + "/stellarc");

  setCacheFile(basedir + "/stella.cache");

  // Set event arrays to a known state
  myPreviousEvents = new uInt8[8];  memset(myPreviousEvents, 0, 8);
  myCurrentEvents  = new uInt8[8];  memset(myCurrentEvents, 0, 8);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemGP2X::~OSystemGP2X()
{
  delete[] myPreviousEvents;
  delete[] myCurrentEvents;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 OSystemGP2X::getTicks() const
{
#ifdef HAVE_GETTIMEOFDAY
  timeval now;
  gettimeofday(&now, 0);

  return (uInt32) (now.tv_sec * 1000000 + now.tv_usec);
#else
  return (uInt32) SDL_GetTicks() * 1000;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemGP2X::getScreenDimensions(int& width, int& height)
{
  width  = 400;
  height = 300;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemGP2X::setDefaultJoymap()
{
  myEventHandler->setDefaultJoyMapping(Event::LauncherMode, kEmulationMode, 0, 8);	// Start
  myEventHandler->setDefaultJoyMapping(Event::TakeSnapshot, kEmulationMode, 0, 9);	// Select
  myEventHandler->setDefaultJoyMapping(Event::ConsoleReset, kEmulationMode, 0, 10);	// L
  myEventHandler->setDefaultJoyMapping(Event::ConsoleSelect, kEmulationMode, 0, 11);	// R
  myEventHandler->setDefaultJoyMapping(Event::CmdMenuMode, kEmulationMode, 0, 12);	// A
  myEventHandler->setDefaultJoyMapping(Event::JoystickZeroFire1, kEmulationMode, 0, 13);	// B
  myEventHandler->setDefaultJoyMapping(Event::MenuMode, kEmulationMode, 0, 14);         // Y
//  myEventHandler->setDefaultJoyMapping(Event::Pause, kEmulationMode, 0, 15);            // X
  myEventHandler->setDefaultJoyMapping(Event::VolumeIncrease, kEmulationMode, 0, 16);	// Vol+
  myEventHandler->setDefaultJoyMapping(Event::VolumeDecrease, kEmulationMode, 0, 17);	// Vol-
  myEventHandler->setDefaultJoyMapping(Event::NoType, kEmulationMode, 0, 18);		// Click
  //Begin Menu Navigation Mapping
  myEventHandler->setDefaultJoyMapping(Event::UICancel, kMenuMode, 0, 8);		// Start
  myEventHandler->setDefaultJoyMapping(Event::UIOK, kMenuMode, 0, 9);			// Select
  myEventHandler->setDefaultJoyMapping(Event::UIPgUp, kMenuMode, 0, 10);		// L
  myEventHandler->setDefaultJoyMapping(Event::UIPgDown, kMenuMode, 0, 11);		// R
//  myEventHandler->setDefaultJoyMapping(Event::UITabPrev, kMenuMode, 0, 12);		// A
  myEventHandler->setDefaultJoyMapping(Event::UISelect, kMenuMode, 0, 13);		// B
//  myEventHandler->setDefaultJoyMapping(Event::UITabNext, kMenuMode, 0, 14);             // Y
  myEventHandler->setDefaultJoyMapping(Event::UICancel, kMenuMode, 0, 15);              // X
  myEventHandler->setDefaultJoyMapping(Event::UINavNext, kMenuMode, 0, 16);		// Vol+
  myEventHandler->setDefaultJoyMapping(Event::UINavPrev, kMenuMode, 0, 17);		// Vol-
  myEventHandler->setDefaultJoyMapping(Event::NoType, kMenuMode, 0, 18);		// Click
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemGP2X::pollEvent()
{
  // Translate joystick button events that act as directions into proper
  // SDL axis events.  This method will use 'case 2', as discussed on the
  // GP2X forums.  Technically, this code should be integrated directly
  // into the GP2X SDL port.

  // Swap event buffers
  uInt8* tmp = myCurrentEvents;
  myCurrentEvents = myPreviousEvents;
  myPreviousEvents = tmp;

  // Scan all the buttons, and detect if an event has occurred
  bool changeDetected = false;
  SDL_Joystick* stick = myEventHandler->getJoystick(0);
  for(int i = 0; i < 8; ++i)
  {
    myCurrentEvents[i] = SDL_JoystickGetButton(stick, i);
    myActiveEvents[i] = myCurrentEvents[i] != myPreviousEvents[i];
    changeDetected = changeDetected || myActiveEvents[i];
  }

  if(changeDetected)
  {
    SDL_JoyAxisEvent eventA0, eventA1;
    eventA0.type  = eventA1.type  = SDL_JOYAXISMOTION;
    eventA0.which = eventA1.which = 0;
    eventA0.value = 0;eventA1.value = 0;
    eventA0.axis  = 0;
    eventA1.axis  = 1;

    bool axisZeroChanged = false, axisOneChanged = false;

    axisOneChanged = axisOneChanged || myActiveEvents[kJDirUp];
    if(myCurrentEvents[kJDirUp])         // up
    {
      eventA1.value = -32768;
    }
    axisOneChanged = axisOneChanged || myActiveEvents[kJDirDown];
    if(myCurrentEvents[kJDirDown])       // down
    {
      eventA1.value =  32767;
    }
    axisZeroChanged = axisZeroChanged || myActiveEvents[kJDirLeft];
    if(myCurrentEvents[kJDirLeft])       // left
    {
      eventA0.value = -32768;
    }
    axisZeroChanged = axisZeroChanged || myActiveEvents[kJDirRight];
    if(myCurrentEvents[kJDirRight])      // right
    {
      eventA0.value =  32767;
    }

    axisOneChanged  = axisOneChanged || myActiveEvents[kJDirUpLeft];
    axisZeroChanged = axisZeroChanged || myActiveEvents[kJDirUpLeft];
    if(myCurrentEvents[kJDirUpLeft])     // up-left
    {
      eventA1.value = -16834;
      eventA0.value = -16834;
    }
    axisOneChanged  = axisOneChanged || myActiveEvents[kJDirUpRight];
    axisZeroChanged = axisZeroChanged || myActiveEvents[kJDirUpRight];
    if(myCurrentEvents[kJDirUpRight])    // up-right
    {
      eventA1.value = -16834;
      eventA0.value =  16834;
    }
    axisOneChanged  = axisOneChanged || myActiveEvents[kJDirDownLeft];
    axisZeroChanged = axisZeroChanged || myActiveEvents[kJDirDownLeft];
    if(myCurrentEvents[kJDirDownLeft])   // down-left
    {
      eventA1.value =  16834;
      eventA0.value = -16834;
    }
    axisOneChanged  = axisOneChanged || myActiveEvents[kJDirDownRight];
    axisZeroChanged = axisZeroChanged || myActiveEvents[kJDirDownRight];
    if(myCurrentEvents[kJDirDownRight])  // down-right
    {
      eventA1.value =  16834;
      eventA0.value =  16834;
    }

    if(axisZeroChanged) SDL_PushEvent((SDL_Event*)&eventA0);
    if(axisOneChanged)  SDL_PushEvent((SDL_Event*)&eventA1);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystemGP2X::joyButtonHandled(int button)
{
  return (button < 8);
}
