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
// $Id: mainSDL.cxx,v 1.29 2005-03-26 04:19:53 stephena Exp $
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
#include "StellaEvent.hxx"
#include "EventHandler.hxx"
#include "FrameBuffer.hxx"
#include "FrameBufferSoft.hxx"
#include "PropsSet.hxx"
#include "Sound.hxx"
#include "SoundSDL.hxx"
#include "Settings.hxx"
#include "OSystem.hxx"

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
static bool SetupJoystick();
static void HandleEvents();
static uInt32 GetTicks();
static void SetupProperties(PropertiesSet& set);
static void ShowInfo(const string& msg);
static void SetPaddleMode(Int32 mode);

#ifdef JOYSTICK_SUPPORT
  // Lookup table for joystick numbers and events
  StellaEvent::JoyStick joyList[StellaEvent::LastJSTICK] = {
    StellaEvent::JSTICK_0, StellaEvent::JSTICK_1, StellaEvent::JSTICK_2,
    StellaEvent::JSTICK_3, StellaEvent::JSTICK_5, StellaEvent::JSTICK_5
  };
  StellaEvent::JoyCode joyButtonList[StellaEvent::LastJCODE] = {
    StellaEvent::JBUTTON_0,  StellaEvent::JBUTTON_1,  StellaEvent::JBUTTON_2,
    StellaEvent::JBUTTON_3,  StellaEvent::JBUTTON_4,  StellaEvent::JBUTTON_5,
    StellaEvent::JBUTTON_6,  StellaEvent::JBUTTON_7,  StellaEvent::JBUTTON_8,
    StellaEvent::JBUTTON_9,  StellaEvent::JBUTTON_10, StellaEvent::JBUTTON_11,
    StellaEvent::JBUTTON_12, StellaEvent::JBUTTON_13, StellaEvent::JBUTTON_14,
    StellaEvent::JBUTTON_15, StellaEvent::JBUTTON_16, StellaEvent::JBUTTON_17,
    StellaEvent::JBUTTON_18, StellaEvent::JBUTTON_19
  };

  enum JoyType { JT_NONE, JT_REGULAR, JT_STELLADAPTOR_1, JT_STELLADAPTOR_2 };

  struct Stella_Joystick
  {
    SDL_Joystick* stick;
    JoyType       type;
  };

  static Stella_Joystick theJoysticks[StellaEvent::LastJSTICK];

  // Static lookup tables for Stelladaptor axis support
  static Event::Type SA_Axis[2][2][3] = {
    Event::JoystickZeroLeft, Event::JoystickZeroRight, Event::PaddleZeroResistance,
    Event::JoystickZeroUp,   Event::JoystickZeroDown,  Event::PaddleOneResistance,
    Event::JoystickOneLeft,  Event::JoystickOneRight,  Event::PaddleTwoResistance,
    Event::JoystickOneUp,    Event::JoystickOneDown,   Event::PaddleThreeResistance
  };
  static Event::Type SA_DrivingValue[2] = {
    Event::DrivingZeroValue, Event::DrivingOneValue
  };
#endif

// Lookup table for paddle resistance events
static Event::Type Paddle_Resistance[4] = {
  Event::PaddleZeroResistance, Event::PaddleOneResistance,
  Event::PaddleTwoResistance,  Event::PaddleThreeResistance
};

// Lookup table for paddle button events
static Event::Type Paddle_Button[4] = {
  Event::PaddleZeroFire, Event::PaddleOneFire,
  Event::PaddleTwoFire,  Event::PaddleThreeFire
};

// Pointer to the main parent osystem object or the null pointer
static OSystem* theOSystem = (OSystem*) NULL;

// Pointer to the display object or the null pointer
static EventHandler* theEventHandler = (EventHandler*) NULL;

// Pointer to the display object or the null pointer
static FrameBuffer* theDisplay = (FrameBuffer*) NULL;

// Pointer to the sound object or the null pointer
static Sound* theSound = (Sound*) NULL;

// Pointer to the settings object or the null pointer
static Settings* theSettings = (Settings*) NULL;

// Indicates if the mouse should be grabbed
static bool theGrabMouseIndicator = false;

// Indicates if the mouse cursor should be hidden
static bool theHideCursorIndicator = false;

// Indicates the current paddle mode
static Int32 thePaddleMode;

// Indicates relative mouse position horizontally
static Int32 mouseX = 0;

// Indicates whether to show information during program execution
static bool theShowInfoFlag;

struct KeyList
{
  SDLKey scanCode;
  StellaEvent::KeyCode keyCode;
};


/**
  Set the mouse to emulate the given paddle
*/
void SetPaddleMode(Int32 mode)
{
	thePaddleMode = mode;

    ostringstream buf;
    buf << "Mouse is paddle " << mode;
    theDisplay->showMessage(buf.str());

	theSettings->setInt("paddle",thePaddleMode);
}


/**
  Prints given message based on 'theShowInfoFlag'
*/
static void ShowInfo(const string& msg)
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
  This routine should be called once setupDisplay is called
  to create the joystick stuff.
*/
bool SetupJoystick()
{
#ifdef JOYSTICK_SUPPORT
  ostringstream message;

  // Keep track of how many Stelladaptors we've found
  uInt8 saCount = 0;

  // First clear the joystick array
  for(uInt32 i = 0; i < StellaEvent::LastJSTICK; i++)
  {
    theJoysticks[i].stick = (SDL_Joystick*) NULL;
    theJoysticks[i].type  = JT_NONE;
  }

  // Initialize the joystick subsystem
  if((SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1) || (SDL_NumJoysticks() <= 0))
  {
    ShowInfo("No joysticks present, use the keyboard.");
    return true;
  }

  // Try to open 4 regular joysticks and 2 Stelladaptor devices
  uInt32 limit = SDL_NumJoysticks() <= StellaEvent::LastJSTICK ?
                 SDL_NumJoysticks() : StellaEvent::LastJSTICK;
  for(uInt32 i = 0; i < limit; i++)
  {
    message.str("");

    string name = SDL_JoystickName(i);
    theJoysticks[i].stick = SDL_JoystickOpen(i);

    // Skip if we couldn't open it for any reason
    if(theJoysticks[i].stick == NULL)
      continue;

    // Figure out what type of joystick this is
    if(name.find("Stelladaptor", 0) != string::npos)
    {
      saCount++;
      if(saCount > 2)  // Ignore more than 2 Stelladaptors
      {
        theJoysticks[i].type = JT_NONE;
        continue;
      }
      else if(saCount == 1)
      {
        name = "Left Stelladaptor (Left joystick, Paddles 0 and 1, Left driving controller)";
        theJoysticks[i].type = JT_STELLADAPTOR_1;
      }
      else if(saCount == 2)
      {
        name = "Right Stelladaptor (Right joystick, Paddles 2 and 3, Right driving controller)";
        theJoysticks[i].type = JT_STELLADAPTOR_2;
      }

      message << "Joystick " << i << ": " << name;
      ShowInfo(message.str());
    }
    else
    {
      theJoysticks[i].type = JT_REGULAR;

      message << "Joystick " << i << ": " << SDL_JoystickName(i)
              << " with " << SDL_JoystickNumButtons(theJoysticks[i].stick)
              << " buttons.";
      ShowInfo(message.str());
    }
  }
#endif

  return true;
}


/**
  This routine should be called regularly to handle events
*/
void HandleEvents()
{
  SDL_Event event;

  // Check for an event
  while(SDL_PollEvent(&event))
  {
    switch(event.type)
    {
      // keyboard events
      case SDL_KEYUP:
      case SDL_KEYDOWN:
      {
        SDLKey key  = event.key.keysym.sym;
        SDLMod mod  = event.key.keysym.mod;
        uInt8 state = event.key.type == SDL_KEYDOWN ? 1 : 0;

        // An attempt to speed up event processing
        // All SDL-specific event actions are accessed by either
        // Control or Alt keys.  So we quickly check for those.
        if(mod & KMOD_ALT && state)
        {
          switch(int(key))
          {
            case SDLK_EQUALS:
              theDisplay->resize(1);
              break;

            case SDLK_MINUS:
              theDisplay->resize(-1);
              break;

            case SDLK_RETURN:
              theDisplay->toggleFullscreen();
              break;

            case SDLK_LEFTBRACKET:
              theSound->adjustVolume(-1);
              break;

            case SDLK_RIGHTBRACKET:
              theSound->adjustVolume(1);
              break;

            case SDLK_f:
              theDisplay->toggleFilter();
              break;
          }

#ifdef DEVELOPER_SUPPORT
          if(theOSystem->eventHandler().state() == EventHandler::S_EMULATE)
          {
            switch(int(key))
            {
              case SDLK_END:       // Alt-End increases XStart
                theOSystem->console().changeXStart(1);
                theDisplay->resize(0);
                break;

              case SDLK_HOME:      // Alt-Home decreases XStart
                theOSystem->console().changeXStart(0);
                theDisplay->resize(0);
                break;

              case SDLK_PAGEUP:    // Alt-PageUp increases YStart
                theOSystem->console().changeYStart(1);
                theDisplay->resize(0);
                break;

              case SDLK_PAGEDOWN:  // Alt-PageDown decreases YStart
                theOSystem->console().changeYStart(0);
                theDisplay->resize(0);
                break;
            }
          }
#endif
        }
        else if(mod & KMOD_CTRL && state)
        {
          if(key == SDLK_q)
          {
            theOSystem->eventHandler().handleEvent(Event::Quit, 1);
          }
          else if(theOSystem->eventHandler().state() == EventHandler::S_EMULATE)
          {
            switch(int(key))
            {
              case SDLK_g:
                // don't change grabmouse in fullscreen mode
                if(!theDisplay->fullScreen())
                {
                  theGrabMouseIndicator = !theGrabMouseIndicator;
                  theSettings->setBool("grabmouse", theGrabMouseIndicator);
                  theDisplay->grabMouse(theGrabMouseIndicator);
                }
                break;

              case SDLK_h:
                // don't change hidecursor in fullscreen mode
                if(!theDisplay->fullScreen())
                {
                  theHideCursorIndicator = !theHideCursorIndicator;
                  theSettings->setBool("hidecursor", theHideCursorIndicator);
                  theDisplay->showCursor(!theHideCursorIndicator);
                }
                break;

              case SDLK_0:         // Ctrl-0 sets the mouse to paddle 0
                SetPaddleMode(0);
                break;

              case SDLK_1:         // Ctrl-1 sets the mouse to paddle 1
                SetPaddleMode(1);
                break;

              case SDLK_2:         // Ctrl-2 sets the mouse to paddle 2
                SetPaddleMode(2);
                break;

              case SDLK_3:         // Ctrl-3 sets the mouse to paddle 3
                SetPaddleMode(3);
                break;

              case SDLK_f:         // Ctrl-f toggles NTSC/PAL mode
                theOSystem->console().toggleFormat();
                theDisplay->setupPalette();
                break;

              case SDLK_p:         // Ctrl-p toggles different palettes
                theOSystem->console().togglePalette();
//                theDisplay->setupPalette();
                break;

#ifdef DEVELOPER_SUPPORT
              case SDLK_END:       // Ctrl-End increases Width
                theOSystem->console().changeWidth(1);
                theDisplay->resize(0);
                break;

              case SDLK_HOME:      // Ctrl-Home decreases Width
                theOSystem->console().changeWidth(0);
                theDisplay->resize(0);
                break;

              case SDLK_PAGEUP:    // Ctrl-PageUp increases Height
                theOSystem->console().changeHeight(1);
                theDisplay->resize(0);
                break;

              case SDLK_PAGEDOWN:  // Ctrl-PageDown decreases Height
                theOSystem->console().changeHeight(0);
                theDisplay->resize(0);
                break;
#endif
              case SDLK_s:         // Ctrl-s saves properties to a file
                // Attempt to merge with propertiesSet
                if(theSettings->getBool("mergeprops"))
                  theOSystem->console().saveProperties(theOSystem->propertiesOutputFilename(), true);
                else  // Save to file in home directory
                {
                  string newPropertiesFile = theOSystem->baseDir() + "/" + \
                    theOSystem->console().properties().get("Cartridge.Name") + ".pro";
                  theOSystem->console().saveProperties(newPropertiesFile);
                }
                break;
            }
          }
        }

        // Otherwise, let the event handler deal with it
        theOSystem->eventHandler().handleKeyEvent(key, mod, state);
        break;  // SDL_KEYUP, SDL_KEYDOWN
      }

      case SDL_MOUSEMOTION:
      {
        theOSystem->eventHandler().handleMouseMotionEvent(event);
        break; // SDL_MOUSEMOTION
      }

      case SDL_MOUSEBUTTONUP:
      case SDL_MOUSEBUTTONDOWN:
      {
        uInt8 state = event.button.type == SDL_MOUSEBUTTONDOWN ? 1 : 0;
        theOSystem->eventHandler().handleMouseButtonEvent(event, state);
        break;  // SDL_MOUSEBUTTONUP, SDL_MOUSEBUTTONDOWN
      }

      case SDL_ACTIVEEVENT:
      {
        if((event.active.state & SDL_APPACTIVE) && (event.active.gain == 0))
        {
          if(!theOSystem->eventHandler().doPause())
            theOSystem->eventHandler().handleEvent(Event::Pause, 1);
        }
        break; // SDL_ACTIVEEVENT
      }

      case SDL_QUIT:
      {
        theOSystem->eventHandler().handleEvent(Event::Quit, 1);
        break;  // SDL_QUIT
      }

      case SDL_VIDEOEXPOSE:
      {
        theDisplay->refresh();
        break;  // SDL_VIDEOEXPOSE
      }
    }

#ifdef JOYSTICK_SUPPORT
    // Read joystick events and modify event states
    StellaEvent::JoyStick stick;
    StellaEvent::JoyCode code;
    Int32 state;
    Uint8 axis;
    Uint8 button;
    Int32 resistance;
    Sint16 value;
    JoyType type;

    if(event.jbutton.which >= StellaEvent::LastJSTICK)
      return;

    stick = joyList[event.jbutton.which];
    type  = theJoysticks[event.jbutton.which].type;

    // Figure put what type of joystick we're dealing with
    // Stelladaptors behave differently, and can't be remapped
    switch(type)
    {
      case JT_NONE:
        break;

      case JT_REGULAR:
        switch(event.type)
        {
          case SDL_JOYBUTTONUP:
          case SDL_JOYBUTTONDOWN:
            if(event.jbutton.button >= StellaEvent::LastJCODE)
              return;

            code  = joyButtonList[event.jbutton.button];
            state = event.jbutton.state == SDL_PRESSED ? 1 : 0;

            theOSystem->eventHandler().sendJoyEvent(stick, code, state);
            break;

          case SDL_JOYAXISMOTION:
            axis = event.jaxis.axis;
            value = event.jaxis.value;

            if(axis == 0)  // x-axis
            {
              theOSystem->eventHandler().sendJoyEvent(stick, StellaEvent::JAXIS_LEFT,
                (value < -16384) ? 1 : 0);
              theOSystem->eventHandler().sendJoyEvent(stick, StellaEvent::JAXIS_RIGHT,
                (value > 16384) ? 1 : 0);
            }
            else if(axis == 1)  // y-axis
            {
              theOSystem->eventHandler().sendJoyEvent(stick, StellaEvent::JAXIS_UP,
                (value < -16384) ? 1 : 0);
              theOSystem->eventHandler().sendJoyEvent(stick, StellaEvent::JAXIS_DOWN,
                (value > 16384) ? 1 : 0);
            }
            break;
        }
        break;  // Regular joystick

      case JT_STELLADAPTOR_1:
      case JT_STELLADAPTOR_2:
        switch(event.type)
        {
          case SDL_JOYBUTTONUP:
          case SDL_JOYBUTTONDOWN:
            button = event.jbutton.button;
            state  = event.jbutton.state == SDL_PRESSED ? 1 : 0;

            // Send button events for the joysticks/paddles/driving controllers
            if(button == 0)
            {
              if(type == JT_STELLADAPTOR_1)
              {
                theOSystem->eventHandler().handleEvent(Event::JoystickZeroFire, state);
                theOSystem->eventHandler().handleEvent(Event::DrivingZeroFire, state);
                theOSystem->eventHandler().handleEvent(Event::PaddleZeroFire, state);
              }
              else
              {
                theOSystem->eventHandler().handleEvent(Event::JoystickOneFire, state);
                theOSystem->eventHandler().handleEvent(Event::DrivingOneFire, state);
                theOSystem->eventHandler().handleEvent(Event::PaddleTwoFire, state);
              }
            }
            else if(button == 1)
            {
              if(type == JT_STELLADAPTOR_1)
                theOSystem->eventHandler().handleEvent(Event::PaddleOneFire, state);
              else
                theOSystem->eventHandler().handleEvent(Event::PaddleThreeFire, state);
            }
            break;

          case SDL_JOYAXISMOTION:
            axis = event.jaxis.axis;
            value = event.jaxis.value;

            // Send axis events for the joysticks
            theOSystem->eventHandler().handleEvent(SA_Axis[type-2][axis][0],
                        (value < -16384) ? 1 : 0);
            theOSystem->eventHandler().handleEvent(SA_Axis[type-2][axis][1],
                        (value > 16384) ? 1 : 0);

            // Send axis events for the paddles
            resistance = (Int32) (1000000.0 * (32767 - value) / 65534);
            theOSystem->eventHandler().handleEvent(SA_Axis[type-2][axis][2], resistance);

            // Send events for the driving controllers
            if(axis == 1)
            {
              if(value <= -16384-4096)
                theOSystem->eventHandler().handleEvent(SA_DrivingValue[type-2],2);
              else if(value > 16384+4096)
                theOSystem->eventHandler().handleEvent(SA_DrivingValue[type-2],1);
              else if(value >= 16384-4096)
                theOSystem->eventHandler().handleEvent(SA_DrivingValue[type-2],0);
              else
                theOSystem->eventHandler().handleEvent(SA_DrivingValue[type-2],3);
            }
            break;
        }
        break;  // Stelladaptor joystick

      default:
        break;
    }
#endif
  }
}


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
      HandleEvents();
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
      HandleEvents();
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

  if(theSound)
    delete theSound;

  if(theDisplay)
    delete theDisplay;

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
  thePaddleMode = theSettings->getInt("paddle");
  theShowInfoFlag = theSettings->getBool("showinfo");
  theGrabMouseIndicator = theSettings->getBool("grabmouse");
  theHideCursorIndicator = theSettings->getBool("hidecursor");
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

  // Set the SDL_VIDEODRIVER environment variable, if possible
  if(theSettings->getString("video_driver") != "")
  {
    ostringstream buf;
    buf << "SDL_VIDEODRIVER=" << theSettings->getString("video_driver");
    putenv((char*) buf.str().c_str());

    buf.str("");
    buf << "Video driver: " << theSettings->getString("video_driver");
    ShowInfo(buf.str());
  }

  // Create an SDL window
  string videodriver = theSettings->getString("video");
  if(videodriver == "soft")
    theDisplay = new FrameBufferSoft(theOSystem);
#ifdef DISPLAY_OPENGL
  else if(videodriver == "gl")
    theDisplay = new FrameBufferGL(theOSystem);
#endif
  else   // a driver that doesn't exist was requested, so use software mode
    theDisplay = new FrameBufferSoft(theOSystem);

  if(!theDisplay)
  {
    cerr << "ERROR: Couldn't set up display.\n";
    Cleanup();
    return 0;
  }

  // Create a sound object for playing audio, even if sound has been disabled
  if(theSettings->getBool("sound"))
    theSound = new SoundSDL(theOSystem);
  else
    theSound = new Sound(theOSystem);

  // Setup the SDL joysticks (must be done after FrameBuffer is created)
  if(!SetupJoystick())
  {
    cerr << "ERROR: Couldn't set up joysticks.\n";
    Cleanup();
    return 0;
  }

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
