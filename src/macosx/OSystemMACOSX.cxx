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
// Copyright (c) 1995-2010 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
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

extern "C" {
  void macOpenConsole(char *romname);
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
  theOSystem->deleteConsole();
  theOSystem->createConsole(romname);
}

// Allow the Menus Objective-C object to pass event sends into the 
//  application.
void macOSXSendMenuEvent(int event)
{
  switch(event)
  {
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
  setConfigFile()

  See OSystem.hxx for a further explanation
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemMACOSX::OSystemMACOSX()
  : OSystem()
{
  setBaseDir("~/Library/Application Support/Stella");

  // This will be overridden, as OSX uses plist files for settings
  setConfigFile("~/Library/Application Support/Stella/stellarc");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemMACOSX::~OSystemMACOSX()
{
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
