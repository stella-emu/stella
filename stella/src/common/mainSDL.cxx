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
// $Id: mainSDL.cxx,v 1.34 2005-05-02 19:35:57 stephena Exp $
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

static Console* CreateConsole(const string& romfile);
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
  bool useMemList = false;
  string theAltPropertiesFile = theOSystem->settings().getString("altpro");
  string thePropertiesFile    = theOSystem->propertiesInputFilename();

  // When 'listrominfo' or 'mergeprops' is specified, we need to have the
  // full list in memory
// FIXME - we need the whole list in memory
//  if(theSettings->getBool("listrominfo") || theSettings->getBool("mergeprops"))
    useMemList = true;

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
  Creates a new game console for the specified game.
*/

Console* CreateConsole(const string& romfile)
{
  Console* console = (Console*) NULL;

  // Open the cartridge image and read it in
  ifstream in(romfile.c_str(), ios_base::binary);
  if(!in)
    cerr << "ERROR: Couldn't open " << romfile << "..." << endl;
  else
  {
    uInt8* image = new uInt8[512 * 1024];
    in.read((char*)image, 512 * 1024);
    uInt32 size = in.gcount();
    in.close();

    // Remove old console from the OSystem
    theOSystem->detachConsole();

    // Create an instance of the 2600 game console
    console = new Console(image, size, theOSystem);

    // Free the image since we don't need it any longer
    delete[] image;
  }

  return console;
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

  // Create the event handler for the system
  EventHandler handler(theOSystem);

  // Cache some settings so they don't have to be repeatedly searched for
  bool theRomLauncherFlag = false;//true;//FIXMEtheSettings->getBool("romlauncher");

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
  if(theOSystem->settings().getBool("showinfo"))
    cout << framerate << endl;

  //// Main loop ////
  // Load a ROM and start the main game loop
  // If the game is given from the commandline, exiting game means exit emulator
  // If the game is loaded from the ROM launcher, exiting game means show launcher
  string romfile = "";
  Console* theConsole  = (Console*) NULL;
////
  ostringstream rom;
  rom << argv[argc - 1];
  romfile = rom.str();
////
  if(theRomLauncherFlag)
  {
    for(;;)
    {
//      theOSystem->gui().showRomLauncher();
      if(theOSystem->eventHandler().doQuit())
        break;
      else  // FIXME - add code here to delay a little
      {
        cerr << "GUI not yet written, run game again (y or n): ";
        char runagain;
        cin >> runagain;
        if(runagain == 'n')
          break;
        else
        {
          if((theConsole = CreateConsole(romfile)) != NULL)
            theOSystem->mainGameLoop();
          else
            break;
        }
      }
    }
  }
  else
  {
    ostringstream romfile;
    romfile << argv[argc - 1];
    theConsole = CreateConsole(romfile.str());
    theOSystem->mainGameLoop();
  }

  // Cleanup time ...
  Cleanup();
  return 0;
}
