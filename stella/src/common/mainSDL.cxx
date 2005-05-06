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
// $Id: mainSDL.cxx,v 1.37 2005-05-06 18:38:59 stephena Exp $
//============================================================================

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdlib>

#include <SDL.h>

#include "bspf.hxx"
#include "Console.hxx"
#include "Event.hxx"
#include "EventHandler.hxx"
#include "FrameBuffer.hxx"
#include "PropsSet.hxx"
#include "Sound.hxx"
#include "Settings.hxx"
#include "OSystem.hxx"

#if defined(UNIX)
  #include "SettingsUNIX.hxx"
  #include "OSystemUNIX.hxx"
#elif defined(WIN32)
  #include "SettingsWin32.hxx"
  #include "OSystemWin32.hxx"
#else
  #error Unsupported platform!
#endif

static void SetupProperties(PropertiesSet& set);
static void Cleanup();

// Pointer to the main parent osystem object or the null pointer
static OSystem* theOSystem = (OSystem*) NULL;


/**
  Setup the properties set by first checking for a user file,
  then a system-wide file.
*/
void SetupProperties(PropertiesSet& set)
{
  bool useMemList = true;  // It seems we always need the list in memory
  string theAltPropertiesFile = theOSystem->settings().getString("altpro");
  string thePropertiesFile    = theOSystem->propertiesInputFilename();

  stringstream buf;
  if(theAltPropertiesFile != "")
  {
    buf << "Game properties: \'" << theAltPropertiesFile << "\'\n";
    set.load(theAltPropertiesFile, useMemList);
  }
  else if(thePropertiesFile != "")
  {
    buf << "Game properties: \'" << thePropertiesFile << "\'\n";
    set.load(thePropertiesFile, useMemList);
  }
  else
    set.load("", false);

  if(theOSystem->settings().getBool("showinfo"))
    cout << buf.str() << endl;
}


/**
  Does general Cleanup in case any operation failed (or at end of program).
*/
void Cleanup()
{
/*  FIXME
#ifdef JOYSTICK_SUPPORT
  if(SDL_WasInit(SDL_INIT_JOYSTICK) & SDL_INIT_JOYSTICK)
  {
    for(uInt32 i = 0; i < StellaEvent::LastJSTICK; i++)
    {
      if(SDL_JoystickOpened(i))
        SDL_JoystickClose(theJoysticks[i].stick);
    }
  }
#endif
*/
  if(theOSystem)
    delete theOSystem;

  if(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)
    SDL_Quit();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int main(int argc, char* argv[])
{
  // Create the parent OSystem object and settings
#if defined(UNIX)
  theOSystem = new OSystemUNIX();
  SettingsUNIX settings(theOSystem);
#elif defined(WIN32)
  theOSystem = new OSystemWin32();
  SettingsWin32 settings(theOSystem);
#else
  #error Unsupported platform!
#endif
  theOSystem->settings().loadConfig();

  // Take care of commandline arguments
  if(!theOSystem->settings().loadCommandLine(argc, argv))
  {
    Cleanup();
    return 0;
  }

  // Finally, make sure the settings are valid
  // We do it once here, so the rest of the program can assume valid settings
  theOSystem->settings().validate();
  bool theShowInfoFlag = theOSystem->settings().getBool("showinfo");

  // Make sure the OSystem has a valid framerate set, since it's used for
  // more then just emulation mode
  theOSystem->setFramerate(theOSystem->settings().getInt("framerate"));

  // Create the event handler for the system
  EventHandler handler(theOSystem);

  // Create a properties set for us to use and set it up
  PropertiesSet propertiesSet;
  SetupProperties(propertiesSet);
  theOSystem->attach(&propertiesSet);

  // Check to see if the 'listroms' argument was given
  // If so, list the roms and immediately exit
  if(theOSystem->settings().getBool("listrominfo"))
  {
    propertiesSet.print();
    Cleanup();
    return 0;
  }

  // Request that the SDL window be centered, if possible
  putenv("SDL_VIDEO_CENTERED=1");

  // Create the framebuffer(s)
  if(!theOSystem->createFrameBuffer())
  {
    cerr << "ERROR: Couldn't set up display.\n";
    Cleanup();
    return 0;
  }

  // Create the sound object
  theOSystem->createSound();

  // Setup the SDL joysticks (must be done after FrameBuffer is created)
/*  FIXME - don't exit if joysticks can't be initialized
  if(!theOSystem->eventHandler().setupJoystick()) // move this into eventhandler
  {
    cerr << "ERROR: Couldn't set up joysticks.\n";
    Cleanup();
    return 0;
  }
*/

  // Print message about the framerate
  string framerate = "Framerate:  " + theOSystem->settings().getString("framerate");
  if(theShowInfoFlag)
    cout << framerate << endl;

  //// Main loop ////
  // First we check if a ROM is specified on the commandline.  If so, and if
  //   the ROM actually exists, use it to create a new console.
  // If not, use the built-in ROM launcher.  In this case, we enter 'launcher'
  //   mode and let the main event loop take care of opening a new console/ROM.
  string romfile = argv[argc - 1];
  if(argc == 1 || !theOSystem->fileExists(romfile))
    theOSystem->createLauncher();
  else
    theOSystem->createConsole(romfile);

  // Start the main loop, and don't exit until the user issues a QUIT command
  theOSystem->mainLoop();

  // Cleanup time ...
  Cleanup();
  return 0;
}
