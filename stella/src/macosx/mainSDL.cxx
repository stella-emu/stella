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
// $Id: mainSDL.cxx,v 1.2 2004-07-10 13:20:36 stephena Exp $
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

#include "SettingsMACOSX.hxx"

extern "C" {
int stellaMain(int argc, char* argv[]);
void setPaddleMode(int mode);
void setLeftJoystickMode(int mode);
void setRightJoystickMode(int mode);
void getPrefsSettings(int *gl, int *volume, float *aspect);
void setPrefsSettings(int gl, int volume, float aspect);
char *browseFile(void);
void prefsStart(void);
void hideApp(void);
void showHelp(void);
void miniturizeWindow(void);
void centerAppWindow(void);
void setPaddleMenu(int number);
void setSpeedLimitMenu(int number);
void initVideoMenu(bool theUseOpenGLFlag);
void enableGameMenus(void);
void AboutBoxScroll(void);
}

static void cleanup();
static bool setupJoystick();
static void handleEvents();
static bool handleInitialEvents();
static bool setupProperties(PropertiesSet& set);
static double Atari_time(void);

#ifdef JOYSTICK_SUPPORT
//  static uInt32 thePaddleNumber;

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

// Indicates user wants to play a new game, not quit 
static bool newGame;

// Indicates the time per frame, which is the inverse of the framerate
static double timePerFrame;

// Pointer to hold the name of the next game to play
static char *gameFile;

static bool fileToLoad = false;


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

void setPaddleMode(int mode)
{
	thePaddleMode = mode;
	theSettings->setInt("paddle",thePaddleMode);
}

void getPrefsSettings(int *gl, int *volume, float *aspect)
{
  if (theSettings->getString("video") == "gl")
	*gl = 1;
  else
	*gl = 0;
	
  *volume = theSettings->getInt("volume");
  if (*volume == -1)
	*volume = 100;
	
  *aspect = theSettings->getFloat("gl_aspect");
}

void setPrefsSettings(int gl, int volume, float aspect)
{
  if (gl)
	theSettings->setString("video","gl");
  else
    theSettings->setString("video","soft");
	
  theSettings->setInt("volume",volume);
  theSound->setVolume(volume);
  
  theSettings->setFloat("gl_aspect",aspect);
}

/**
  This routine should be called once setupDisplay is called
  to create the joystick stuff.
*/
bool setupJoystick()
{
#ifdef JOYSTICK_SUPPORT
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
    if(theShowInfoFlag)
      cout << "No joysticks present, use the keyboard.\n";

    return true;
  }

  // Try to open 4 regular joysticks and 2 Stelladaptor devices
  uInt32 limit = SDL_NumJoysticks() <= StellaEvent::LastJSTICK ?
                 SDL_NumJoysticks() : StellaEvent::LastJSTICK;
  for(uInt32 i = 0; i < limit; i++)
  {
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

      if(theShowInfoFlag)
        cout << "Joystick " << i << ": " << name << endl;
    }
    else
    {
      theJoysticks[i].type = JT_REGULAR;
      if(theShowInfoFlag)
      {
        cout << "Joystick " << i << ": " << SDL_JoystickName(i)
             << " with " << SDL_JoystickNumButtons(theJoysticks[i].stick)
             << " buttons.\n";
      }
    }
  }
#endif

  return true;
}

/**
  This routine is called to handle events if user cancels initial open panel 
*/
bool handleInitialEvents(void)
{
  SDL_Event event;
  SDLKey key;
  SDLMod mod;
  bool status;

  // Check for an event
  while(1)
  {
    while(SDL_PollEvent(&event))
	{
      // keyboard events
      if(event.type == SDL_KEYDOWN)
      {
        key = event.key.keysym.sym;
        mod = event.key.keysym.mod;

        if((mod & KMOD_META) && !(mod & KMOD_SHIFT))
        {
          if(key == SDLK_q)         // Command-q quits
		  {
		    return(false);
		  }
		  else if(key == SDLK_o)         // Command-o starts a new game
		  {
		    return(true);
		  }
	    }
	  }
      else if(event.type == SDL_QUIT)
      {
		return(false);
      }
	}
	AboutBoxScroll();
	usleep(16667);
  }
  return status;
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
	  if((mod & KMOD_META) && (mod & KMOD_SHIFT))
	  {
        if(key == SDLK_p)         // Shift-Command-p toggles different palettes
        {
          theConsole->togglePalette();
          theDisplay->setupPalette();
        }
        else if(key == SDLK_f)         // Command-f toggles NTSC/PAL mode
        {
          theConsole->toggleFormat();
          theDisplay->setupPalette();
        }
	  }
      else if((mod & KMOD_META) && !(mod & KMOD_SHIFT))
      {
        if(key == SDLK_EQUALS) {
          theDisplay->resize(1);
		  centerAppWindow();
		  }
        else if(key == SDLK_MINUS) {
          theDisplay->resize(-1);
		  centerAppWindow();
		  }
        else if(key == SDLK_RETURN)
        {
          theDisplay->toggleFullscreen();
		  centerAppWindow();
        }
        else if(key == SDLK_q)         // Command-q quits
		{
		    newGame = false;
			theConsole->eventHandler().sendEvent(Event::Quit, 1);
		}
        else if(key == SDLK_o)         // Command-o starts a new game
		{
			char *file;
			bool wasFullscreen;
			
			wasFullscreen = theDisplay->fullScreen();
			if (wasFullscreen) {
				theDisplay->toggleFullscreen();
				centerAppWindow();
			  }
            theDisplay->grabMouse(false);
            theDisplay->showCursor(true);
			file = browseFile();
			if (file) 
			{
		        newGame = true;
			    fileToLoad = true;
			    gameFile = file;
				if (wasFullscreen) {
					theDisplay->toggleFullscreen();
					centerAppWindow();
				}
			    theConsole->eventHandler().sendEvent(Event::Quit, 1);
			}
		}
        else if(key == SDLK_g)
        {
          // don't change grabmouse in fullscreen mode
          if(!theDisplay->fullScreen())
          {
            theGrabMouseIndicator = !theGrabMouseIndicator;
            theSettings->setBool("grabmouse", theGrabMouseIndicator);
            theDisplay->grabMouse(theGrabMouseIndicator);
            theHideCursorIndicator = !theHideCursorIndicator;
            theSettings->setBool("hidecursor", theHideCursorIndicator);
            theDisplay->showCursor(!theHideCursorIndicator);
          }
        }
#ifdef DISPLAY_OPENGL
        else if(key == SDLK_f && theUseOpenGLFlag)
          ((FrameBufferGL*)theDisplay)->toggleFilter();
#endif
        else if(key == SDLK_l)         // Command-l toggles frame limit
        {
		  if (theSettings->getInt("framerate") != 10000000)
		  {
			theSettings->setInt("framerate",10000000);
			timePerFrame = 1.0 / 10000000.0;
            setSpeedLimitMenu(0);
		  }
		  else
		  {
			theSettings->setInt("framerate",60);
			timePerFrame = 1.0 / 60.0;
            setSpeedLimitMenu(1);
		  }
        }
        else if(key == SDLK_p)         // Command-p toggles pause mode
        {
			theConsole->eventHandler().sendEvent(Event::Pause, 1);
        }
        else if(key == SDLK_h)         // Command-h hides the application
        {
          if(!theDisplay->fullScreen())
			hideApp();
        }
        else if(key == SDLK_m)         // Command-m minimizes the window
        {
          if(!theDisplay->fullScreen())
		    miniturizeWindow();
        }
        else if(key == SDLK_COMMA)		// Command-, runs the preferences pane.
        {
          if(!theDisplay->fullScreen())
		    prefsStart();
        }
        else if(key == SDLK_SLASH)	// Command-? runs the help.
        {
          if(!theDisplay->fullScreen())
			showHelp();
        }
        else if (key == SDLK_0)   // Command-0 sets the mouse to paddle 0
        {
          setPaddleMode(0);
          setPaddleMenu(0);
        }
        else if (key == SDLK_1)   // Command-0 sets the mouse to paddle 1
        {
          setPaddleMode(1);
          setPaddleMenu(1);
        }
        else if (key == SDLK_2)   // Command-0 sets the mouse to paddle 2
        {
          setPaddleMode(2);
          setPaddleMenu(2);
        }
        else if (key == SDLK_3)   // Command-0 sets the mouse to paddle 3
        {
          setPaddleMode(3);
          setPaddleMenu(3);
        }
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
    else if(event.type == SDL_MOUSEMOTION )
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
	    static bool appActivateWasPaused;
		static bool appInputWasPaused;
		
	    if (event.active.state & (SDL_APPACTIVE)) {
		   if (event.active.gain == 0) {
		     appActivateWasPaused = theConsole->eventHandler().doPause();
		     if (!appActivateWasPaused)
               theConsole->eventHandler().sendEvent(Event::Pause, 1);
			 }
		   else {
		     if (!appActivateWasPaused)
               theConsole->eventHandler().sendEvent(Event::Pause, 1);
		   }
		  }
	    if (event.active.state & (SDL_APPINPUTFOCUS)) {
		   if (event.active.gain == 0) {
		     appInputWasPaused = theConsole->eventHandler().doPause() ||
			                     theDisplay->fullScreen();
		     if (!appInputWasPaused)
               theConsole->eventHandler().sendEvent(Event::Pause, 1);
			 }
		   else {
		     if (!appInputWasPaused)
               theConsole->eventHandler().sendEvent(Event::Pause, 1);
		   }
		  }
    }
    else if(event.type == SDL_QUIT)
    {
	  newGame = false;
      theConsole->eventHandler().sendEvent(Event::Quit, 1);
    }
    else if(event.type == SDL_VIDEOEXPOSE)
    {
      theDisplay->refresh();
    }
	else if(event.type == SDL_USEREVENT)
	{
		if (event.user.code == 1)
		{
		newGame = true;
		fileToLoad = true;
		gameFile = (char *) event.user.data1;
		theConsole->eventHandler().sendEvent(Event::Quit, 1);
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
        break;  // Regular joystick

      case JT_STELLADAPTOR_1:
      case JT_STELLADAPTOR_2:

        if((event.type == SDL_JOYBUTTONDOWN) || (event.type == SDL_JOYBUTTONUP))
        {
          button = event.jbutton.button;
          state  = event.jbutton.state == SDL_PRESSED ? 1 : 0;

          // Send button events for the joysticks/paddles
          if(button == 0)
          {
            if(type == JT_STELLADAPTOR_1)
            {
              theConsole->eventHandler().sendEvent(Event::JoystickZeroFire, state);
              theConsole->eventHandler().sendEvent(Event::DrivingZeroFire, state);
              theConsole->eventHandler().sendEvent(Event::PaddleZeroFire, state);
            }
            else
            {
              theConsole->eventHandler().sendEvent(Event::JoystickOneFire, state);
              theConsole->eventHandler().sendEvent(Event::DrivingOneFire, state);
              theConsole->eventHandler().sendEvent(Event::PaddleTwoFire, state);
            }
          }
          else if(button == 1)
          {
            if(type == JT_STELLADAPTOR_1)
              theConsole->eventHandler().sendEvent(Event::PaddleOneFire, state);
            else
              theConsole->eventHandler().sendEvent(Event::PaddleThreeFire, state);
          }
        }
        else if(event.type == SDL_JOYAXISMOTION)
        {
          axis = event.jaxis.axis;
          value = event.jaxis.value;

          // Send axis events for the joysticks
          theConsole->eventHandler().sendEvent(SA_Axis[type-2][axis][0],
                      (value < -16384) ? 1 : 0);
          theConsole->eventHandler().sendEvent(SA_Axis[type-2][axis][1],
                      (value > 16384) ? 1 : 0);

          // Send axis events for the paddles
          resistance = (Int32) (1000000.0 * (32767 - value) / 65534);
          theConsole->eventHandler().sendEvent(SA_Axis[type-2][axis][2], resistance);
		  
		  // Send events for the driving controllers
		  if (axis == 1)
		  {
		    if (value <= -16384-4096)
			{
               theConsole->eventHandler().sendEvent(SA_DrivingValue[type-2],2);
			}
		    else if (value > 16384+4096)
			{
               theConsole->eventHandler().sendEvent(SA_DrivingValue[type-2],1);
			}
		    else if (value >= 16384-4096)
			{
               theConsole->eventHandler().sendEvent(SA_DrivingValue[type-2],0);
			}
		    else 
			{
               theConsole->eventHandler().sendEvent(SA_DrivingValue[type-2],3);
			}
		  }
        }
        break;  // Stelladaptor joystick

      default:
        break;
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
    set.load(theAlternateProFile, useMemList);
    return true;
  }

  if(theUserProFile != "")
  {
    set.load(theUserProFile, useMemList);
    return true;
  }
  else if(theSystemProFile != "")
  {
    set.load(theSystemProFile, useMemList);
    return true;
  }
  else
  {
    set.load("", false);
    return true;
  }
}


/**
  Does general in case any operation failed (or at end of program).
*/
void cleanup()
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

  if(theSettings)
    delete theSettings;

  if(theConsole)
    delete theConsole;

  if(theSound)
    delete theSound;

  if(theDisplay)
    delete theDisplay;

  if(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)
    SDL_Quit();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int stellaMain(int argc, char* argv[])
{
  theSettings = new SettingsMACOSX();

  if(!theSettings)
  {
    cleanup();
    return 0;
  }
  theSettings->loadConfig();

  // Cache some settings so they don't have to be repeatedly searched for
  thePaddleMode = theSettings->getInt("paddle");
  setPaddleMenu(thePaddleMode);
  theShowInfoFlag = theSettings->getBool("showinfo");
  theGrabMouseIndicator = theSettings->getBool("grabmouse");
  theHideCursorIndicator = theSettings->getBool("hidecursor");
  timePerFrame = 1.0 / (double)theSettings->getInt("framerate");
  if (theSettings->getInt("framerate") == 60)
     setSpeedLimitMenu(1);

  // Initialize SDL Library
  Uint32 initflags = SDL_INIT_VIDEO | SDL_INIT_TIMER;
  if(SDL_Init(initflags) < 0)
  {
     return(0);
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
  
  initVideoMenu(theUseOpenGLFlag);

  if(!theDisplay)
  {
    cerr << "ERROR: Couldn't set up display.\n";
    cleanup();
    return 0;
  }

  // Create a sound object for playing audio
  if(theSettings->getBool("sound"))
  {
    uInt32 fragsize = theSettings->getInt("fragsize");
    theSound = new SoundSDL(fragsize);
    if(theShowInfoFlag)
    {
      cout << "Sound enabled, using fragment size = " << fragsize;
      cout << "." << endl;
    }
  }
  else  // even if sound has been disabled, we still need a sound object
  {
    theSound = new Sound();
    if(theShowInfoFlag)
      cout << "Sound disabled.\n";
  }
  theSound->setVolume(theSettings->getInt("volume"));
  
  if (argv[1] != NULL)
  {
    fileToLoad = true;
	gameFile = argv[1];
  }
  
while(1) {
  // Get a pointer to the file which contains the cartridge ROM
  char* file;

  if (fileToLoad)
  {
     file = gameFile;
  }
  else
  {
    do 
	{
      file = browseFile();
      if (file == NULL) 
	  {
	    if (!handleInitialEvents())
	        return 0;
	  }
	} while (file == NULL);
  }
  fileToLoad = false;
  
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

  // Get just the filename of the file containing the ROM image
  const char* filename = (!strrchr(file, '/')) ? file : strrchr(file, '/') + 1;

  // Create the 2600 game console
  theConsole = new Console(image, size, filename, *theSettings, propertiesSet,
                           *theDisplay, *theSound);

  // Free the image since we don't need it any longer
  delete[] image;
  free(file);

  // Setup the SDL joysticks
  // This must be done after the console is created
  if(!setupJoystick())
  {
    cerr << "ERROR: Couldn't set up joysticks.\n";
    cleanup();
    return 0;
  }

  double lasttime = Atari_time();
  double frametime = 0.1;	/* measure time between two runs */

  enableGameMenus();
  
  // Set the base for the timers
  // Main game loop
  for(;;)
  {
    // Exit if the user wants to quit
    if(theConsole->eventHandler().doQuit())
    {
      break;
    }

    handleEvents();
    theConsole->update();
    
    double lastcurtime = 0;
    double curtime, delaytime;

    if (timePerFrame > 0.0)
    {
	  curtime = Atari_time();
	  delaytime = lasttime + timePerFrame - curtime;
	  if (delaytime > 0)
	    usleep((int) (delaytime * 1e6));
	  curtime = Atari_time();

	  /* make average time */
	  frametime = (frametime * 4.0 + curtime - lastcurtime) * 0.2;
  //  fps = 1.0 / frametime;
	  lastcurtime = curtime;

	  lasttime += timePerFrame;
	  if ((lasttime + timePerFrame) < curtime)
	    lasttime = curtime;
	}
	AboutBoxScroll();
  }

  delete theConsole;
  theConsole = NULL;
    
  if (!newGame)
	break;
  newGame = false;
}

  // Cleanup time ...
  cleanup();
  
  return 0;
}

static double Atari_time(void)
{
  struct timeval tp;

  gettimeofday(&tp, NULL);
  return 1000000 * tp.tv_sec + 1e-6 * tp.tv_usec;
}


