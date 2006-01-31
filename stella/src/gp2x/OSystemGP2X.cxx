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
// $Id: OSystemGP2X.cxx,v 1.4 2006-01-31 17:26:56 stephena Exp $
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
  setConfigFiles()
  setCacheFile()

  And for initializing the following variables:

  myDriverList (a StringList)

  See OSystem.hxx for a further explanation
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemGP2X::OSystemGP2X()
{
  // GP2X needs all config files in exec directory
  
  char *currdir = getcwd(NULL, 0);
  string basedir = currdir;
  free(currdir);
  setBaseDir(basedir);
  
  string statedir = basedir + "/state";
  setStateDir(statedir);
  
  setPropertiesDir(basedir, basedir);

  string userPropertiesFile   = basedir + "/user.pro";
  string systemPropertiesFile = basedir + "/stella.pro";
  setConfigFiles(userPropertiesFile, systemPropertiesFile);

  string userConfigFile   = basedir + "/stellarc";
  string systemConfigFile = statedir + "/stellarc";
  setConfigFiles(userConfigFile, systemConfigFile);

  string cacheFile = basedir + "/stella.cache";
  setCacheFile(cacheFile);

  // Set hat event handler to a known state
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
void OSystemGP2X::mainLoop()
{
  // These variables are common to both timing options
  // and are needed to calculate the overall frames per second.
  uInt32 frameTime = 0, numberOfFrames = 0;

  // Set up less accurate timing stuff
  uInt32 startTime, virtualTime, currentTime;

  // Set the base for the timers
  virtualTime = getTicks();
  frameTime = 0;

  // Main game loop
  for(;;)
  {
    // Exit if the user wants to quit
    if(myEventHandler->doQuit())
      break;

    startTime = getTicks();
    myEventHandler->poll(startTime);
    myFrameBuffer->update();

    currentTime = getTicks();
    virtualTime += myTimePerFrame;
    if(currentTime < virtualTime)
      SDL_Delay((virtualTime - currentTime)/1000);

    currentTime = getTicks() - startTime;
    frameTime += currentTime;
    ++numberOfFrames;
  }

  // Only print console information if a console was actually created
  if(mySettings->getBool("showinfo") && myConsole)
  {
    double executionTime = (double) frameTime / 1000000.0;
    double framesPerSecond = (double) numberOfFrames / executionTime;

    cout << endl;
    cout << numberOfFrames << " total frames drawn\n";
    cout << framesPerSecond << " frames/second\n";
    cout << endl;
    cout << "Cartridge Name: " << myConsole->properties().get("Cartridge.Name");
    cout << endl;
    cout << "Cartridge MD5:  " << myConsole->properties().get("Cartridge.MD5");
    cout << endl << endl;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 OSystemGP2X::getTicks()
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
  width  = 320;
  height = 240;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemGP2X::setDefaultJoymap()
{
//  myEventHandler->setDefaultJoyMapping(Event::JoystickZeroUp, 0, 0);    // Up
//  myEventHandler->setDefaultJoyMapping(Event::JoystickZeroLeft, 0, 2);  // Left
//  myEventHandler->setDefaultJoyMapping(Event::JoystickZeroDown, 0, 4);  // Down
//  myEventHandler->setDefaultJoyMapping(Event::JoystickZeroRight, 0, 6); // Right
  myEventHandler->setDefaultJoyMapping(Event::LauncherMode, 0, 8);      // Start
  myEventHandler->setDefaultJoyMapping(Event::CmdMenuMode, 0, 9);       // Select
  myEventHandler->setDefaultJoyMapping(Event::ConsoleReset, 0, 10);     // L
  myEventHandler->setDefaultJoyMapping(Event::ConsoleSelect, 0, 11);    // R
  myEventHandler->setDefaultJoyMapping(Event::TakeSnapshot, 0, 12);	    // A
  myEventHandler->setDefaultJoyMapping(Event::JoystickZeroFire, 0, 13); // B
  myEventHandler->setDefaultJoyMapping(Event::Pause, 0, 14);            // X
  myEventHandler->setDefaultJoyMapping(Event::MenuMode, 0, 15);         // Y
  myEventHandler->setDefaultJoyMapping(Event::VolumeIncrease, 0, 16);   // Vol+
  myEventHandler->setDefaultJoyMapping(Event::VolumeDecrease, 0, 17);   // Vol-
  myEventHandler->setDefaultJoyMapping(Event::NoType, 0, 18);           // Click
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemGP2X::setDefaultJoyHatMap()
{
  myEventHandler->setDefaultJoyHatMapping(Event::JoystickZeroUp, 0, 0, kJHatUp);
  myEventHandler->setDefaultJoyHatMapping(Event::JoystickZeroLeft, 0, 0, kJHatLeft);
  myEventHandler->setDefaultJoyHatMapping(Event::JoystickZeroDown, 0, 0, kJHatDown);
  myEventHandler->setDefaultJoyHatMapping(Event::JoystickZeroRight, 0, 0, kJHatRight);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemGP2X::pollEvent()
{
  // Translate joystick button events that act as directions into proper
  // SDL HAT events.  This method will use 'case 2', as discussed on the
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
    if(myCurrentEvents[i] != myPreviousEvents[i])
      changeDetected = true;
  }

  // Create an appropriate SDL HAT event for the new state
  uInt8 value = SDL_HAT_CENTERED;
  if(changeDetected)
  {
    // Merge 'in-between' values to the proper direction
    // For example, if both 'up' and 'upright' are on, turn 'upright' off
    if(myCurrentEvents[kJDirUp])              // up
    {
      myCurrentEvents[kJDirUpLeft] = myCurrentEvents[kJDirUpRight] = 0;
      value |= SDL_HAT_UP;
    }
    else if(myCurrentEvents[kJDirDown])       // down
    {
      myCurrentEvents[kJDirDownLeft] = myCurrentEvents[kJDirDownRight] = 0;
      value |= SDL_HAT_DOWN;
    }
    else if(myCurrentEvents[kJDirLeft])       // left
    {
      myCurrentEvents[kJDirUpLeft] = myCurrentEvents[kJDirDownLeft] = 0;
      value |= SDL_HAT_LEFT;
    }
    else if(myCurrentEvents[kJDirRight])      // right
    {
      myCurrentEvents[kJDirUpRight] = myCurrentEvents[kJDirDownRight] = 0;
      value |= SDL_HAT_RIGHT;
    }

    // Now consider diagonal positions
    if(myCurrentEvents[kJDirUpLeft])          // up-left
    {
      value |= SDL_HAT_UP | SDL_HAT_LEFT;
    }
    else if(myCurrentEvents[kJDirUpRight])    // up-right
    {
      value |= SDL_HAT_UP | SDL_HAT_RIGHT;
    }
    else if(myCurrentEvents[kJDirDownLeft])   // down-left
    {
      value |= SDL_HAT_DOWN | SDL_HAT_LEFT;
    }
    else if(myCurrentEvents[kJDirDownRight])  // down-right
    {
      value |= SDL_HAT_DOWN | SDL_HAT_RIGHT;
    }

    // Finally, create the HAT event and pass it to SDL
    SDL_JoyHatEvent event;
    event.type  = SDL_JOYHATMOTION;
    event.which = 0;
    event.hat   = 0;
    event.value = value;

    SDL_PushEvent((SDL_Event*)&event);
  }
}
