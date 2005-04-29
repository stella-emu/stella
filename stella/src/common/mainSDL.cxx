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
// $Id: mainSDL.cxx,v 1.32 2005-04-29 19:05:05 stephena Exp $
//============================================================================

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdlib>

#ifdef HAVE_GETTIMEOFDAY
  #include <time.h>
  #include <sys/time.h>
#endif

#include <SDL.h>

#include "bspf.hxx"
#include "Console.hxx"
#include "Event.hxx"
#include "EventHandler.hxx"
#include "FrameBuffer.hxx"
#include "FrameBufferSoft.hxx"
#include "PropsSet.hxx"
#include "Sound.hxx"
#include "Settings.hxx"
#include "OSystem.hxx"

#ifdef SOUND_SUPPORT
  #include "SoundSDL.hxx"
#else
  #include "SoundNull.hxx"
#endif

#ifdef DISPLAY_OPENGL
  #include "FrameBufferGL.hxx"
#endif

#if defined(UNIX)
  #include "SettingsUNIX.hxx"
  #include "OSystemUNIX.hxx"
#elif defined(WIN32)
  #include "SettingsWin32.hxx"
  #include "OSystemWin32.hxx"
#else
  #error Unsupported platform!
#endif

static void mainGameLoop();
static Console* CreateConsole(const string& romfile);
static void Cleanup();
static uInt32 GetTicks();
static void SetupProperties(PropertiesSet& set);
static void ShowInfo(const string& msg);

// Pointer to the main parent osystem object or the null pointer
static OSystem* theOSystem = (OSystem*) NULL;

// Pointer to the display object or the null pointer
static EventHandler* theEventHandler = (EventHandler*) NULL;

// Pointer to the sound object or the null pointer
static Sound* theSound = (Sound*) NULL;

// Pointer to the settings object or the null pointer
static Settings* theSettings = (Settings*) NULL;

// Indicates whether to show information during program execution
static bool theShowInfoFlag;


/**
  Prints given message based on 'theShowInfoFlag'
*/
static inline void ShowInfo(const string& msg)
{
  if(theShowInfoFlag && msg != "")
    cout << msg << endl;
}


/**
  Returns number of ticks in microseconds
*/
#ifdef HAVE_GETTIMEOFDAY
inline uInt32 GetTicks()
{
  timeval now;
  gettimeofday(&now, 0);

  return (uInt32) (now.tv_sec * 1000000 + now.tv_usec);
}
#else
inline uInt32 GetTicks()
{
  return (uInt32) SDL_GetTicks() * 1000;
}
#endif


/**
  Setup the properties set by first checking for a user file,
  then a system-wide file.
*/
void SetupProperties(PropertiesSet& set)
{
  bool useMemList = false;
  string theAltPropertiesFile = theSettings->getString("altpro");
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

  ShowInfo(buf.str());
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


// FIXME - move this into OSystem, so that different systems can make use
//         of system-specific timers (probably more accurate than SDL can provide)
/**
  Runs the main game loop until the current game exits.
*/
void mainGameLoop()
{
  // These variables are common to both timing options
  // and are needed to calculate the overall frames per second.
  uInt32 frameTime = 0, numberOfFrames = 0;

  if(theSettings->getBool("accurate"))   // normal, CPU-intensive timing
  {
    // Set up accurate timing stuff
    uInt32 startTime, delta;
    uInt32 timePerFrame = (uInt32)(1000000.0 / (double) theSettings->getInt("framerate"));

    // Set the base for the timers
    frameTime = 0;

    // Main game loop
    for(;;)
    {
      // Exit if the user wants to quit
      if(theOSystem->eventHandler().doExitGame() ||
         theOSystem->eventHandler().doQuit())
        break;

      startTime = GetTicks();
      theOSystem->eventHandler().poll();
      theOSystem->frameBuffer().update();

      // Now, waste time if we need to so that we are at the desired frame rate
      for(;;)
      {
        delta = GetTicks() - startTime;

        if(delta >= timePerFrame)
          break;
      }

      frameTime += GetTicks() - startTime;
      ++numberOfFrames;
    }
  }
  else    // less accurate, less CPU-intensive timing
  {
    // Set up less accurate timing stuff
    uInt32 startTime, virtualTime, currentTime;
    uInt32 timePerFrame = (uInt32)(1000000.0 / (double) theSettings->getInt("framerate"));

    // Set the base for the timers
    virtualTime = GetTicks();
    frameTime = 0;

    // Main game loop
    for(;;)
    {
      // Exit if the user wants to quit
      if(theOSystem->eventHandler().doExitGame() ||
         theOSystem->eventHandler().doQuit())
        break;

      startTime = GetTicks();
      theOSystem->eventHandler().poll();
      theOSystem->frameBuffer().update();

      currentTime = GetTicks();
      virtualTime += timePerFrame;
      if(currentTime < virtualTime)
      {
        SDL_Delay((virtualTime - currentTime)/1000);
      }

      currentTime = GetTicks() - startTime;
      frameTime += currentTime;
      ++numberOfFrames;
    }
  }

  if(theShowInfoFlag)
  {
    double executionTime = (double) frameTime / 1000000.0;
    double framesPerSecond = (double) numberOfFrames / executionTime;

    cout << endl;
    cout << numberOfFrames << " total frames drawn\n";
    cout << framesPerSecond << " frames/second\n";
    cout << endl;
    cout << "Cartridge Name: " << theOSystem->console().properties().get("Cartridge.Name");
    cout << endl;
    cout << "Cartridge MD5:  " << theOSystem->console().properties().get("Cartridge.MD5");
    cout << endl << endl;
  }
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

  if(theSound)
    delete theSound;

  if(theEventHandler)
    delete theEventHandler;

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
  theSettings = new SettingsUNIX(theOSystem);
#elif defined(WIN32)
  theOSystem = new OSystemWin32();
  theSettings = new SettingsWin32(theOSystem);
#else
  #error Unsupported platform!
#endif
  if(!theSettings)
  {
    Cleanup();
    return 0;
  }
  theSettings->loadConfig();

  // Take care of commandline arguments
  if(!theSettings->loadCommandLine(argc, argv))
  {
    Cleanup();
    return 0;
  }

  // Finally, make sure the settings are valid
  // We do it once here, so the rest of the program can assume valid settings
//FIXME  theSettings->validate();

  // Create the event handler for the system
  theEventHandler = new EventHandler(theOSystem);

  // Cache some settings so they don't have to be repeatedly searched for
  theShowInfoFlag = theSettings->getBool("showinfo");
  bool theRomLauncherFlag = false;//true;//FIXMEtheSettings->getBool("romlauncher");

  // Create a properties set for us to use and set it up
  PropertiesSet propertiesSet;
  SetupProperties(propertiesSet);
  theOSystem->attach(&propertiesSet);

  // Check to see if the 'listroms' argument was given
  // If so, list the roms and immediately exit
  if(theSettings->getBool("listrominfo"))
  {
    propertiesSet.print();
    Cleanup();
    return 0;
  }

  // Request that the SDL window be centered, if possible
  putenv("SDL_VIDEO_CENTERED=1");

  // Create the SDL framebuffer
  if(!theOSystem->createFrameBuffer())
  {
    cerr << "ERROR: Couldn't set up display.\n";
    Cleanup();
    return 0;
  }

  // Create a sound object for playing audio, even if sound has been disabled
#ifdef SOUND_SUPPORT
  theSound = new SoundSDL(theOSystem);
#else
  theSound = new SoundNull(theOSystem);
#endif

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
  ostringstream message;
  message << "Framerate:  " << theSettings->getInt("framerate");
  ShowInfo(message.str());

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
            mainGameLoop();
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
    mainGameLoop();
  }

  // Cleanup time ...
  Cleanup();
  return 0;
}
