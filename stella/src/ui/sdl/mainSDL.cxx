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
// Copyright (c) 1995-2002 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: mainSDL.cxx,v 1.71 2004-04-16 13:10:37 stephena Exp $
//============================================================================

#include <fstream>
#include <iostream>
#include <string>
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
#include "FrameBufferSDL.hxx"
#include "FrameBufferSoft.hxx"
#include "PropsSet.hxx"
#include "Sound.hxx"
#include "SoundSDL.hxx"
#include "Settings.hxx"

#ifdef DISPLAY_OPENGL
  #include "FrameBufferGL.hxx"

  // Indicates whether to use OpenGL mode
  static bool theUseOpenGLFlag;
#endif

#if defined(UNIX)
  #include "SettingsUNIX.hxx"
#elif defined(WIN32) 
  #include "SettingsWin32.hxx"
#else
  #error Unsupported platform!
#endif

static void cleanup();
static bool setupJoystick();
static void handleEvents();
static uInt32 getTicks();
static bool setupProperties(PropertiesSet& set);

#ifdef JOYSTICK_SUPPORT
  static SDL_Joystick* theLeftJoystick = (SDL_Joystick*) NULL;
  static SDL_Joystick* theRightJoystick = (SDL_Joystick*) NULL;
  static uInt32 theLeftJoystickNumber;
  static uInt32 theRightJoystickNumber;
//  static uInt32 thePaddleNumber;
#endif

// Pointer to the console object or the null pointer
static Console* theConsole = (Console*) NULL;

// Pointer to the display object or the null pointer
static FrameBufferSDL* theDisplay = (FrameBufferSDL*) NULL;

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

struct Switches
{
  SDLKey scanCode;
  StellaEvent::KeyCode keyCode;
};

// Place the most used keys first to speed up access
// Todo - initialize this array in the same order as the SDLK
// keys are defined, so it can be a constant-time LUT
static Switches keyList[] = {
    { SDLK_F1,          StellaEvent::KCODE_F1         },
    { SDLK_F2,          StellaEvent::KCODE_F2         },
    { SDLK_F3,          StellaEvent::KCODE_F3         },
    { SDLK_F4,          StellaEvent::KCODE_F4         },
    { SDLK_F5,          StellaEvent::KCODE_F5         },
    { SDLK_F6,          StellaEvent::KCODE_F6         },
    { SDLK_F7,          StellaEvent::KCODE_F7         },
    { SDLK_F8,          StellaEvent::KCODE_F8         },
    { SDLK_F9,          StellaEvent::KCODE_F9         },
    { SDLK_F10,         StellaEvent::KCODE_F10        },
    { SDLK_F11,         StellaEvent::KCODE_F11        },
    { SDLK_F12,         StellaEvent::KCODE_F12        },
    { SDLK_F13,         StellaEvent::KCODE_F13        },
    { SDLK_F14,         StellaEvent::KCODE_F14        },
    { SDLK_F15,         StellaEvent::KCODE_F15        },

    { SDLK_UP,          StellaEvent::KCODE_UP         },
    { SDLK_DOWN,        StellaEvent::KCODE_DOWN       },
    { SDLK_LEFT,        StellaEvent::KCODE_LEFT       },
    { SDLK_RIGHT,       StellaEvent::KCODE_RIGHT      },
    { SDLK_SPACE,       StellaEvent::KCODE_SPACE      },
    { SDLK_LCTRL,       StellaEvent::KCODE_LCTRL      },
    { SDLK_RCTRL,       StellaEvent::KCODE_RCTRL      },
    { SDLK_LALT,        StellaEvent::KCODE_LALT       },
    { SDLK_RALT,        StellaEvent::KCODE_RALT       },
    { SDLK_LSUPER,      StellaEvent::KCODE_LWIN       },
    { SDLK_RSUPER,      StellaEvent::KCODE_RWIN       },
    { SDLK_MENU,        StellaEvent::KCODE_MENU       },

    { SDLK_a,           StellaEvent::KCODE_a          },
    { SDLK_b,           StellaEvent::KCODE_b          },
    { SDLK_c,           StellaEvent::KCODE_c          },
    { SDLK_d,           StellaEvent::KCODE_d          },
    { SDLK_e,           StellaEvent::KCODE_e          },
    { SDLK_f,           StellaEvent::KCODE_f          },
    { SDLK_g,           StellaEvent::KCODE_g          },
    { SDLK_h,           StellaEvent::KCODE_h          },
    { SDLK_i,           StellaEvent::KCODE_i          },
    { SDLK_j,           StellaEvent::KCODE_j          },
    { SDLK_k,           StellaEvent::KCODE_k          },
    { SDLK_l,           StellaEvent::KCODE_l          },
    { SDLK_m,           StellaEvent::KCODE_m          },
    { SDLK_n,           StellaEvent::KCODE_n          },
    { SDLK_o,           StellaEvent::KCODE_o          },
    { SDLK_p,           StellaEvent::KCODE_p          },
    { SDLK_q,           StellaEvent::KCODE_q          },
    { SDLK_r,           StellaEvent::KCODE_r          },
    { SDLK_s,           StellaEvent::KCODE_s          },
    { SDLK_t,           StellaEvent::KCODE_t          },
    { SDLK_u,           StellaEvent::KCODE_u          },
    { SDLK_v,           StellaEvent::KCODE_v          },
    { SDLK_w,           StellaEvent::KCODE_w          },
    { SDLK_x,           StellaEvent::KCODE_x          },
    { SDLK_y,           StellaEvent::KCODE_y          },
    { SDLK_z,           StellaEvent::KCODE_z          },

    { SDLK_0,           StellaEvent::KCODE_0          },
    { SDLK_1,           StellaEvent::KCODE_1          },
    { SDLK_2,           StellaEvent::KCODE_2          },
    { SDLK_3,           StellaEvent::KCODE_3          },
    { SDLK_4,           StellaEvent::KCODE_4          },
    { SDLK_5,           StellaEvent::KCODE_5          },
    { SDLK_6,           StellaEvent::KCODE_6          },
    { SDLK_7,           StellaEvent::KCODE_7          },
    { SDLK_8,           StellaEvent::KCODE_8          },
    { SDLK_9,           StellaEvent::KCODE_9          },

    { SDLK_KP0,         StellaEvent::KCODE_KP0        },
    { SDLK_KP1,         StellaEvent::KCODE_KP1        },
    { SDLK_KP2,         StellaEvent::KCODE_KP2        },
    { SDLK_KP3,         StellaEvent::KCODE_KP3        },
    { SDLK_KP4,         StellaEvent::KCODE_KP4        },
    { SDLK_KP5,         StellaEvent::KCODE_KP5        },
    { SDLK_KP6,         StellaEvent::KCODE_KP6        },
    { SDLK_KP7,         StellaEvent::KCODE_KP7        },
    { SDLK_KP8,         StellaEvent::KCODE_KP8        },
    { SDLK_KP9,         StellaEvent::KCODE_KP9        },
    { SDLK_KP_PERIOD,   StellaEvent::KCODE_KP_PERIOD  },
    { SDLK_KP_DIVIDE,   StellaEvent::KCODE_KP_DIVIDE  },
    { SDLK_KP_MULTIPLY, StellaEvent::KCODE_KP_MULTIPLY},
    { SDLK_KP_MINUS,    StellaEvent::KCODE_KP_MINUS   },
    { SDLK_KP_PLUS,     StellaEvent::KCODE_KP_PLUS    },
    { SDLK_KP_ENTER,    StellaEvent::KCODE_KP_ENTER   },
    { SDLK_KP_EQUALS,   StellaEvent::KCODE_KP_EQUALS  },

    { SDLK_BACKSPACE,   StellaEvent::KCODE_BACKSPACE  },
    { SDLK_TAB,         StellaEvent::KCODE_TAB        },
    { SDLK_CLEAR,       StellaEvent::KCODE_CLEAR      },
    { SDLK_RETURN,      StellaEvent::KCODE_RETURN     },
    { SDLK_ESCAPE,      StellaEvent::KCODE_ESCAPE     },
    { SDLK_COMMA,       StellaEvent::KCODE_COMMA      },
    { SDLK_MINUS,       StellaEvent::KCODE_MINUS      },
    { SDLK_PERIOD,      StellaEvent::KCODE_PERIOD     },
    { SDLK_SLASH,       StellaEvent::KCODE_SLASH      },
    { SDLK_BACKSLASH,   StellaEvent::KCODE_BACKSLASH  },
    { SDLK_SEMICOLON,   StellaEvent::KCODE_SEMICOLON  },
    { SDLK_EQUALS,      StellaEvent::KCODE_EQUALS     },
    { SDLK_QUOTE,       StellaEvent::KCODE_QUOTE      },
    { SDLK_BACKQUOTE,   StellaEvent::KCODE_BACKQUOTE  },
    { SDLK_LEFTBRACKET, StellaEvent::KCODE_LEFTBRACKET},
    { SDLK_RIGHTBRACKET,StellaEvent::KCODE_RIGHTBRACKET},

    { SDLK_PRINT,       StellaEvent::KCODE_PRTSCREEN  },
    { SDLK_MODE,        StellaEvent::KCODE_SCRLOCK    },
    { SDLK_PAUSE,       StellaEvent::KCODE_PAUSE      },
    { SDLK_INSERT,      StellaEvent::KCODE_INSERT     },
    { SDLK_HOME,        StellaEvent::KCODE_HOME       },
    { SDLK_PAGEUP,      StellaEvent::KCODE_PAGEUP     },
    { SDLK_DELETE,      StellaEvent::KCODE_DELETE     },
    { SDLK_END,         StellaEvent::KCODE_END        },
    { SDLK_PAGEDOWN,    StellaEvent::KCODE_PAGEDOWN   }
  };

// Lookup table for joystick numbers and events
StellaEvent::JoyStick joyList[StellaEvent::LastJSTICK] = {
    StellaEvent::JSTICK_0, StellaEvent::JSTICK_1,
    StellaEvent::JSTICK_2, StellaEvent::JSTICK_3
};
StellaEvent::JoyCode joyButtonList[StellaEvent::LastJCODE] = {
    StellaEvent::JBUTTON_0, StellaEvent::JBUTTON_1, StellaEvent::JBUTTON_2, 
    StellaEvent::JBUTTON_3, StellaEvent::JBUTTON_4, StellaEvent::JBUTTON_5, 
    StellaEvent::JBUTTON_6, StellaEvent::JBUTTON_7, StellaEvent::JBUTTON_8, 
    StellaEvent::JBUTTON_9
};


/**
  Returns number of ticks in microseconds
*/
#ifdef HAVE_GETTIMEOFDAY
inline uInt32 getTicks()
{
  timeval now;
  gettimeofday(&now, 0);

  return (uInt32) (now.tv_sec * 1000000 + now.tv_usec);
}
#else
inline uInt32 getTicks()
{
  return (uInt32) SDL_GetTicks() * 1000;
}
#endif


/**
  This routine should be called once setupDisplay is called
  to create the joystick stuff.
*/
bool setupJoystick()
{
#ifdef JOYSTICK_SUPPORT
  // Initialize the joystick subsystem
  if((SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1) || (SDL_NumJoysticks() <= 0))
  {
    if(theShowInfoFlag)
      cout << "No joysticks present, use the keyboard.\n";
    theLeftJoystick = theRightJoystick = 0;
    return true;
  }

  theLeftJoystickNumber = (uInt32) theConsole->settings().getInt("joyleft");
  if((theLeftJoystick = SDL_JoystickOpen(theLeftJoystickNumber)) != NULL)
  {
    if(theShowInfoFlag)
      cout << "Left joystick is a "
           << SDL_JoystickName(theLeftJoystickNumber)
           << " with " << SDL_JoystickNumButtons(theLeftJoystick) << " buttons.\n";
  }
  else
  {
    if(theShowInfoFlag)
      cout << "Left joystick not present, use keyboard instead.\n";
  }

  theRightJoystickNumber = theConsole->settings().getInt("joyright");
  if((theRightJoystick = SDL_JoystickOpen(theRightJoystickNumber)) != NULL)
  {
    if(theShowInfoFlag)
      cout << "Right joystick is a "
           << SDL_JoystickName(theRightJoystickNumber)
           << " with " << SDL_JoystickNumButtons(theRightJoystick) << " buttons.\n";
  }
  else
  {
    if(theShowInfoFlag)
      cout << "Right joystick not present, use keyboard instead.\n";
  }
#endif

  return true;
}


/**
  This routine should be called regularly to handle events
*/
void handleEvents()
{
  SDL_Event event;
  Uint8 type;
  SDLKey key;
  SDLMod mod;

  // Check for an event
  while(SDL_PollEvent(&event))
  {
    // keyboard events
    if(event.type == SDL_KEYDOWN)
    {
      key = event.key.keysym.sym;
      mod = event.key.keysym.mod;
      type = event.type;

      // An attempt to speed up event processing
      // All SDL-specific event actions are accessed by either
      // Control or Alt keys.  So we quickly check for those.
      if(mod & KMOD_ALT)
      {
        if(key == SDLK_EQUALS)
          theDisplay->resize(1);
        else if(key == SDLK_MINUS)
          theDisplay->resize(-1);
        else if(key == SDLK_RETURN)
        {
          theDisplay->toggleFullscreen();
        }
#ifdef DISPLAY_OPENGL
        else if(key == SDLK_f && theUseOpenGLFlag)
          ((FrameBufferGL*)theDisplay)->toggleFilter();
#endif
#ifdef DEVELOPER_SUPPORT
        else if(key == SDLK_END)       // Alt-End increases XStart
        {
          theConsole->changeXStart(1);
          theDisplay->resize(0);
        }
        else if(key == SDLK_HOME)      // Alt-Home decreases XStart
        {
          theConsole->changeXStart(0);
          theDisplay->resize(0);
        }
        else if(key == SDLK_PAGEUP)    // Alt-PageUp increases YStart
        {
          theConsole->changeYStart(1);
          theDisplay->resize(0);
        }
        else if(key == SDLK_PAGEDOWN)  // Alt-PageDown decreases YStart
        {
          theConsole->changeYStart(0);
          theDisplay->resize(0);
        }
#endif
      }
      else if(mod & KMOD_CTRL)
      {
        if(key == SDLK_g)
        {
          // don't change grabmouse in fullscreen mode
          if(!theDisplay->fullScreen())
          {
            theGrabMouseIndicator = !theGrabMouseIndicator;
            theDisplay->grabMouse(theGrabMouseIndicator);
          }
        }
        else if(key == SDLK_h)
        {
          // don't change hidecursor in fullscreen mode
          if(!theDisplay->fullScreen())
          {
            theHideCursorIndicator = !theHideCursorIndicator;
            theDisplay->showCursor(!theHideCursorIndicator);
          }
        }
#ifdef DEVELOPER_SUPPORT
        else if(key == SDLK_f)         // Ctrl-f toggles NTSC/PAL mode
        {
          theConsole->toggleFormat();
          theDisplay->setupPalette();
        }
        else if(key == SDLK_p)         // Ctrl-p toggles different palettes
        {
          theConsole->togglePalette();
          theDisplay->setupPalette();
        }
        else if(key == SDLK_END)       // Ctrl-End increases Width
        {
          theConsole->changeWidth(1);
          theDisplay->resize(0);
        }
        else if(key == SDLK_HOME)      // Ctrl-Home decreases Width
        {
          theConsole->changeWidth(0);
          theDisplay->resize(0);
        }
        else if(key == SDLK_PAGEUP)    // Ctrl-PageUp increases Height
        {
          theConsole->changeHeight(1);
          theDisplay->resize(0);
        }
        else if(key == SDLK_PAGEDOWN)  // Ctrl-PageDown decreases Height
        {
          theConsole->changeHeight(0);
          theDisplay->resize(0);
        }
        else if(key == SDLK_s)         // Ctrl-s saves properties to a file
        {
          if(theConsole->settings().getBool("mergeprops"))  // Attempt to merge with propertiesSet
          {
            theConsole->saveProperties(theSettings->userPropertiesFilename(), true);
          }
          else  // Save to file in home directory
          {
            string newPropertiesFile = theConsole->settings().baseDir() + "/" + \
              theConsole->properties().get("Cartridge.Name") + ".pro";
            replace(newPropertiesFile.begin(), newPropertiesFile.end(), ' ', '_');
            theConsole->saveProperties(newPropertiesFile);
          }
        }
#endif
      }
      else // check all the other keys
      {
        for(unsigned int i = 0; i < sizeof(keyList) / sizeof(Switches); ++i)
        {
          if(keyList[i].scanCode == key)
            theConsole->eventHandler().sendKeyEvent(keyList[i].keyCode, 1);
        }
      }
    }
    else if(event.type == SDL_KEYUP)
    {
      key  = event.key.keysym.sym;
      type = event.type;

      for(unsigned int i = 0; i < sizeof(keyList) / sizeof(Switches); ++i)
      { 
        if(keyList[i].scanCode == key)
          theConsole->eventHandler().sendKeyEvent(keyList[i].keyCode, 0);
      }
    }
    else if(event.type == SDL_MOUSEMOTION)
    {
      Int32 resistance;
      uInt32 zoom  = theDisplay->zoomLevel();
      Int32 width = theDisplay->width() * zoom;
      Event::Type type = Event::NoType;

      // Grabmouse and hidecursor introduce some lag into the mouse movement,
      // so we need to fudge the numbers a bit
      if(theGrabMouseIndicator && theHideCursorIndicator)
      {
        mouseX = (int)((float)mouseX + (float)event.motion.xrel
                 * 1.5 * (float) zoom);
      }
      else
      {
        mouseX = mouseX + event.motion.xrel * zoom;
      }

      // Check to make sure mouseX is within the game window
      if(mouseX < 0)
        mouseX = 0;
      else if(mouseX > width)
        mouseX = width;
  
      resistance = (Int32)(1000000.0 * (width - mouseX) / width);

      // Now, set the event of the correct paddle to the calculated resistance
      if(thePaddleMode == 0)
        type = Event::PaddleZeroResistance;
      else if(thePaddleMode == 1)
        type = Event::PaddleOneResistance;
      else if(thePaddleMode == 2)
        type = Event::PaddleTwoResistance;
      else if(thePaddleMode == 3)
        type = Event::PaddleThreeResistance;

      theConsole->eventHandler().sendEvent(type, resistance);
    }
    else if(event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
    {
      Event::Type type = Event::LastType;
      Int32 value;

      if(event.type == SDL_MOUSEBUTTONDOWN)
        value = 1;
      else
        value = 0;

      if(thePaddleMode == 0)
        type = Event::PaddleZeroFire;
      else if(thePaddleMode == 1)
        type = Event::PaddleOneFire;
      else if(thePaddleMode == 2)
        type = Event::PaddleTwoFire;
      else if(thePaddleMode == 3)
        type = Event::PaddleThreeFire;

      theConsole->eventHandler().sendEvent(type, value);
    }
    else if(event.type == SDL_ACTIVEEVENT)
    {
      if((event.active.state & SDL_APPACTIVE) && (event.active.gain == 0))
      {
        if(!theConsole->eventHandler().doPause())
        {
          theConsole->eventHandler().sendEvent(Event::Pause, 1);
        }
      }
    }
    else if(event.type == SDL_QUIT)
    {
      theConsole->eventHandler().sendEvent(Event::Quit, 1);
    }
    else if(event.type == SDL_VIDEOEXPOSE)
    {
      theDisplay->refresh();
    }

#ifdef JOYSTICK_SUPPORT
    // Read joystick events and modify event states
    StellaEvent::JoyStick stick;
    StellaEvent::JoyCode code;
    Int32 state;
    Uint8 axis;
    Sint16 value;

    if(event.jbutton.which >= StellaEvent::LastJSTICK)
      return;
    stick = joyList[event.jbutton.which];

    if((event.type == SDL_JOYBUTTONDOWN) || (event.type == SDL_JOYBUTTONUP))
    {
      if(event.jbutton.button >= StellaEvent::LastJCODE)
        return;

      code  = joyButtonList[event.jbutton.button];
      state = event.jbutton.state == SDL_PRESSED ? 1 : 0;

      theConsole->eventHandler().sendJoyEvent(stick, code, state);
    }
    else if(event.type == SDL_JOYAXISMOTION)
    {
      axis = event.jaxis.axis;
      value = event.jaxis.value;

      if(axis == 0)  // x-axis
      {
        theConsole->eventHandler().sendJoyEvent(stick, StellaEvent::JAXIS_LEFT,
          (value < -16384) ? 1 : 0);
        theConsole->eventHandler().sendJoyEvent(stick, StellaEvent::JAXIS_RIGHT,
          (value > 16384) ? 1 : 0);
      }
      else if(axis == 1)  // y-axis
      {
        theConsole->eventHandler().sendJoyEvent(stick, StellaEvent::JAXIS_UP,
          (value < -16384) ? 1 : 0);
        theConsole->eventHandler().sendJoyEvent(stick, StellaEvent::JAXIS_DOWN,
          (value > 16384) ? 1 : 0);
      }
    }
#endif
  }
}


/**
  Setup the properties set by first checking for a user file
  "$HOME/.stella/stella.pro", then a system-wide file "/etc/stella.pro".
  Return false if neither file is found, else return true.

  @param set The properties set to setup
*/
bool setupProperties(PropertiesSet& set)
{
  bool useMemList = false;
  string theAlternateProFile = theSettings->getString("altpro");
  string theUserProFile = theSettings->userPropertiesFilename();
  string theSystemProFile = theSettings->systemPropertiesFilename();

#ifdef DEVELOPER_SUPPORT
  // If the user wishes to merge any property modifications to the
  // PropertiesSet file, then the PropertiesSet file MUST be loaded
  // into memory.
  useMemList = theSettings->getBool("mergeprops");
#endif

  // Check to see if the user has specified an alternate .pro file.

  if(theAlternateProFile != "")
  {
    set.load(theAlternateProFile, &Console::defaultProperties(), useMemList);
    return true;
  }

  if(theUserProFile != "")
  {
    set.load(theUserProFile, &Console::defaultProperties(), useMemList);
    return true;
  }
  else if(theSystemProFile != "")
  {
    set.load(theSystemProFile, &Console::defaultProperties(), useMemList);
    return true;
  }
  else
  {
    set.load("", &Console::defaultProperties(), false);
    return true;
  }
}


/**
  Does general cleanup in case any operation failed (or at end of program).
*/
void cleanup()
{
  if(theSettings)
    delete theSettings;

  if(theConsole)
    delete theConsole;

  if(theSound)
    delete theSound;

  if(theDisplay)
    delete theDisplay;

  if(SDL_WasInit(SDL_INIT_EVERYTHING))
  {
#ifdef JOYSTICK_SUPPORT
    if(SDL_JoystickOpened(theLeftJoystickNumber))
      SDL_JoystickClose(theLeftJoystick);
    if(SDL_JoystickOpened(theRightJoystickNumber))
      SDL_JoystickClose(theRightJoystick);
#endif

    SDL_Quit();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int main(int argc, char* argv[])
{
#if defined(UNIX)
  theSettings = new SettingsUNIX();
#elif defined(WIN32) 
  theSettings = new SettingsWin32();
#else
  #error Unsupported platform!
#endif
  if(!theSettings)
  {
    cleanup();
    return 0;
  }
  theSettings->loadConfig();

  // Take care of commandline arguments
  if(!theSettings->loadCommandLine(argc, argv))
  {
    string message = "Stella version 1.4_cvs\n\nUsage: stella [options ...] romfile";
    theSettings->usage(message);
    cleanup();
    return 0;
  }

  // Cache some settings so they don't have to be repeatedly searched for
  thePaddleMode = theSettings->getInt("paddle");
  theShowInfoFlag = theSettings->getBool("showinfo");

  // Request that the SDL window be centered, if possible
  putenv("SDL_VIDEO_CENTERED=1");

  // Get a pointer to the file which contains the cartridge ROM
  const char* file = argv[argc - 1];

  // Open the cartridge image and read it in
  ifstream in(file, ios_base::binary);
  if(!in)
  {
    cerr << "ERROR: Couldn't open " << file << "..." << endl;
    cleanup();
    return 0;
  }

  uInt8* image = new uInt8[512 * 1024];
  in.read((char*)image, 512 * 1024);
  uInt32 size = in.gcount();
  in.close();

  // Create a properties set for us to use and set it up
  PropertiesSet propertiesSet;
  if(!setupProperties(propertiesSet))
  {
    delete[] image;
    cleanup();
    return 0;
  }

  // Create an SDL window
  string videodriver = theSettings->getString("video");
  if(videodriver == "soft")
  {
    theDisplay = new FrameBufferSoft();
    if(theShowInfoFlag)
      cout << "Using software mode for video.\n";
  }
#ifdef DISPLAY_OPENGL
  else if(videodriver == "gl")
  {
    theDisplay = new FrameBufferGL();
    theUseOpenGLFlag = true;
    if(theShowInfoFlag)
      cout << "Using OpenGL mode for video.\n";
  }
#endif
  else   // a driver that doesn't exist was requested, so use software mode
  {
    theDisplay = new FrameBufferSoft();
    if(theShowInfoFlag)
      cout << "Using software mode for video.\n";
  }

  if(!theDisplay)
  {
    cerr << "ERROR: Couldn't set up display.\n";
    delete[] image;
    cleanup();
    return 0;
  }

  // Create a sound object for playing audio
  if(theSettings->getBool("sound"))
  {
    uInt32 fragsize = theSettings->getInt("fragsize");
    uInt32 bufsize  = theSettings->getInt("bufsize");
    theSound = new SoundSDL(fragsize, bufsize);
    if(theShowInfoFlag)
    {
      cout << "Sound enabled, using fragment size = " << fragsize;
      cout << " and buffer size = " << bufsize << "." << endl;
    }
  }
  else  // even if sound has been disabled, we still need a sound object
  {
    theSound = new Sound();
    if(theShowInfoFlag)
      cout << "Sound disabled.\n";
  }
  theSound->setVolume(theSettings->getInt("volume"));

  // Get just the filename of the file containing the ROM image
  const char* filename = (!strrchr(file, '/')) ? file : strrchr(file, '/') + 1;

  // Create the 2600 game console
  theConsole = new Console(image, size, filename, *theSettings, propertiesSet,
                           *theDisplay, *theSound);

  // Free the image since we don't need it any longer
  delete[] image;

  // Setup the SDL joysticks
  // This must be done after the console is created
  if(!setupJoystick())
  {
    cerr << "ERROR: Couldn't set up joysticks.\n";
    cleanup();
    return 0;
  }

  // These variables are common to both timing options
  // and are needed to calculate the overall frames per second.
  uInt32 frameTime = 0, numberOfFrames = 0;

  if(theSettings->getBool("accurate"))   // normal, CPU-intensive timing
  {
    // Set up accurate timing stuff
    uInt32 startTime, delta;
    uInt32 timePerFrame = (uInt32)(1000000.0 / (double)theSettings->getInt("framerate"));

    // Set the base for the timers
    frameTime = 0;

    // Main game loop
    for(;;)
    {
      // Exit if the user wants to quit
      if(theConsole->eventHandler().doQuit())
      {
        break;
      }

      startTime = getTicks();
      handleEvents();
      theConsole->update();

      // Now, waste time if we need to so that we are at the desired frame rate
      for(;;)
      {
        delta = getTicks() - startTime;

        if(delta >= timePerFrame)
          break;
      }

      frameTime += getTicks() - startTime;
      ++numberOfFrames;
    }
  }
  else    // less accurate, less CPU-intensive timing
  {
    // Set up less accurate timing stuff
    uInt32 startTime, virtualTime, currentTime;
    uInt32 timePerFrame = (uInt32)(1000000.0 / (double)theSettings->getInt("framerate"));

    // Set the base for the timers
    virtualTime = getTicks();
    frameTime = 0;

    // Main game loop
    for(;;)
    {
      // Exit if the user wants to quit
      if(theConsole->eventHandler().doQuit())
      {
        break;
      }

      startTime = getTicks();
      handleEvents();
      theConsole->update();

      currentTime = getTicks();
      virtualTime += timePerFrame;
      if(currentTime < virtualTime)
      {
        SDL_Delay((virtualTime - currentTime)/1000);
      }

      currentTime = getTicks() - startTime;
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
    cout << "Cartridge Name: " << theConsole->properties().get("Cartridge.Name");
    cout << endl;
    cout << "Cartridge MD5:  " << theConsole->properties().get("Cartridge.MD5");
    cout << endl << endl;
  }

  // Cleanup time ...
  cleanup();
  return 0;
}
