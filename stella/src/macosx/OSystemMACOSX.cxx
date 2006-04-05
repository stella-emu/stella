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
// $Id: OSystemMACOSX.cxx,v 1.11 2006-04-05 02:18:58 markgrebe Exp $
//============================================================================

#include <cstdlib>
#include <sstream>
#include <fstream>
#include <string>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h> /* for MAXPATHLEN */

#include "bspf.hxx"
#include "OSystem.hxx"
#include "OSystemMACOSX.hxx"
#include "MenusEvents.h"

#ifdef HAVE_GETTIMEOFDAY
  #include <time.h>
  #include <sys/time.h>
#endif

extern "C" {
  void macOpenConsole(char *romname);
  uInt16 macOSXDisplayWidth(void);
  uInt16 macOSXDisplayHeight(void);
  void setEmulationMenus(void);
  void setLauncherMenus(void);
  void setOptionsMenus(void);
  void setCommandMenus(void);
  void setDebuggerMenus(void);
  void macOSXSendMenuEvent(int event);
}

// Pointer to the main parent osystem object or the null pointer
extern OSystem* theOSystem;
extern char parentdir[MAXPATHLEN];

// Allow the SDL main object to pass request to open cartridges from 
//  the OS into the application.
void macOpenConsole(char *romname)
{
	theOSystem->createConsole(romname);
}

// Allow the Menus Objective-C object to pass event sends into the 
//  application.
void macOSXSendMenuEvent(int event)
{
    switch(event) {
        case MENU_PAUSE:
            theOSystem->eventHandler().handleEvent(Event::Pause, 1);
            break;
        case MENU_OPEN:
            theOSystem->eventHandler().handleEvent(Event::LauncherMode, 1);
            break;
        case MENU_VOLUME_INCREASE:
            theOSystem->eventHandler().handleEvent(Event::VolumeIncrease, 1);
            break;
        case MENU_VOLUME_DECREASE:
            theOSystem->eventHandler().handleEvent(Event::VolumeDecrease, 1);
            break;
    }
}

/**
  Each derived class is responsible for calling the following methods
  in its constructor:

  setBaseDir()
  setStateDir()
  setPropertiesFiles()
  setConfigFile()
  setCacheFile()

  See OSystem.hxx for a further explanation
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemMACOSX::OSystemMACOSX()
{
  // First set variables that the OSystem needs
  string basedir = string(getenv("HOME")) + "/.stella";
  setBaseDir(basedir);

  setStateDir(basedir + "/state");

  setPropertiesDir(basedir);
  setConfigFile(basedir + "/stellarc");

  setCacheFile(basedir + "/stella.cache");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemMACOSX::~OSystemMACOSX()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemMACOSX::mainLoop()
{
#if 0
    double lasttime = Atari_time();
    double lastcurtime = 0;
    double curtime, delaytime;
	double timePerFrame = 1.0 / 60.0;

    // Main game loop
    for(;;)
    {
      // Exit if the user wants to quit
      if(myEventHandler->doQuit())
        break;

      myEventHandler->poll();
      myFrameBuffer->update();

      if (timePerFrame > 0.0)
      {
	    curtime = Atari_time();
	    delaytime = lasttime + timePerFrame - curtime;
	    if (delaytime > 0)
	      usleep((int) (delaytime * 1e6));
	    curtime = Atari_time();

	    lastcurtime = curtime;

	    lasttime += timePerFrame;
	    if ((lasttime + timePerFrame) < curtime)
	      lasttime = curtime;
	  }
    }
#else
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
      virtualTime = startTime + myTimePerFrame;
      if(currentTime < virtualTime)
      {
      SDL_Delay((virtualTime - currentTime)/1000);
      }
	  
      currentTime = getTicks() - startTime;
      frameTime += currentTime;
      ++numberOfFrames;
    }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 OSystemMACOSX::getTicks()
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
void OSystemMACOSX::getScreenDimensions(int& width, int& height)
{
  width  = (int)macOSXDisplayWidth();
  height = (int)macOSXDisplayHeight();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemMACOSX::stateChanged(EventHandler::State state)
{
  switch(state)
  {
    case EventHandler::S_EMULATE:
      setEmulationMenus();
      break;

    case EventHandler::S_LAUNCHER:
      setLauncherMenus();
      break;

    case EventHandler::S_MENU:
      setOptionsMenus();
      break;

    case EventHandler::S_CMDMENU:
      setCommandMenus();
      break;

    case EventHandler::S_DEBUGGER:
      setDebuggerMenus();
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemMACOSX::pauseChanged(bool status)
{
}
