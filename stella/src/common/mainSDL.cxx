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
// $Id: mainSDL.cxx,v 1.48 2005-06-28 04:40:21 urchlay Exp $
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
#include "FSNode.hxx"
#include "OSystem.hxx"

#if defined(UNIX)
  #include "SettingsUNIX.hxx"
  #include "OSystemUNIX.hxx"
#elif defined(WIN32)
  #include "SettingsWin32.hxx"
  #include "OSystemWin32.hxx"
#elif defined(MAC_OSX)
  #include "SettingsMACOSX.hxx"
  #include "OSystemMACOSX.hxx"
extern "C" {
int stellaMain(int argc, char* argv[]);
}
#else
  #error Unsupported platform!
#endif

static void SetupProperties(PropertiesSet& set);
static void Cleanup();

// Pointer to the main parent osystem object or the null pointer
OSystem* theOSystem = (OSystem*) NULL;


/**
  Setup the properties set by first checking for a user file,
  then a system-wide file.
*/
void SetupProperties(PropertiesSet& set)
{
  bool useMemList = true;  // It seems we always need the list in memory
  string theAltPropertiesFile = theOSystem->settings().getString("pro");
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
  if(theOSystem)
    delete theOSystem;

  if(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)
    SDL_Quit();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if defined(MAC_OSX)
int stellaMain(int argc, char* argv[])
#else
int main(int argc, char* argv[])
#endif
{
  // Create the parent OSystem object and settings
#if defined(UNIX)
  theOSystem = new OSystemUNIX();
  SettingsUNIX settings(theOSystem);
#elif defined(WIN32)
  theOSystem = new OSystemWin32();
  SettingsWin32 settings(theOSystem);
#elif defined(MAC_OSX)
  theOSystem = new OSystemMACOSX();
  SettingsMACOSX settings(theOSystem);
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
  if(theOSystem->settings().getBool("center"))
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
  theOSystem->eventHandler().setupJoysticks();

  //// Main loop ////
  // First we check if a ROM is specified on the commandline.  If so, and if
  //   the ROM actually exists, use it to create a new console.
  // If not, use the built-in ROM launcher.  In this case, we enter 'launcher'
  //   mode and let the main event loop take care of opening a new console/ROM.
  string romfile = argv[argc - 1];
  if(argc == 1 || !FilesystemNode::fileExists(romfile))
    theOSystem->createLauncher();
  else
  {
    theOSystem->createConsole(romfile);

    if(theOSystem->settings().getBool("holdreset"))
      theOSystem->eventHandler().handleEvent(Event::ConsoleReset, 1);

    if(theOSystem->settings().getBool("holdselect"))
      theOSystem->eventHandler().handleEvent(Event::ConsoleSelect, 1);

    if(theOSystem->settings().getBool("holdbutton0"))
      theOSystem->eventHandler().handleEvent(Event::JoystickZeroFire, 1);

    if(theOSystem->settings().getBool("debug"))
      handler.enterDebugMode();
  }

  // Start the main loop, and don't exit until the user issues a QUIT command
  theOSystem->mainLoop();

  // Cleanup time ...
  Cleanup();
  return 0;
}
