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
// $Id: mainSDL.cxx,v 1.44 2003-09-04 23:23:06 stephena Exp $
//============================================================================

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>

#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <SDL.h>
#include <SDL_syswm.h>

#include "bspf.hxx"
#include "Console.hxx"
#include "Event.hxx"
#include "StellaEvent.hxx"
#include "EventHandler.hxx"
#include "MediaSrc.hxx"
#include "PropsSet.hxx"
#include "Sound.hxx"
#include "RectList.hxx"
#include "Settings.hxx"

#ifdef SOUND_ALSA
  #include "SoundALSA.hxx"
#endif

#ifdef SOUND_OSS
  #include "SoundOSS.hxx"
#endif

#ifdef SOUND_SDL
  #include "SoundSDL.hxx"
#endif

#ifdef HAVE_PNG
  #include "Snapshot.hxx"
#endif

// Hack for SDL < 1.2.0
#ifndef SDL_ENABLE
  #define SDL_ENABLE 1
#endif
#ifndef SDL_DISABLE
  #define SDL_DISABLE 0
#endif

#define MESSAGE_INTERVAL 2

// function prototypes
static bool setupDisplay();
static bool setupJoystick();
static bool createScreen();
static void recalculate8BitPalette();
static void setupPalette();
static void cleanup();
static bool setupDirs();

static void updateDisplay(MediaSource& mediaSource);
static void handleEvents();

static void doQuit();
static void resizeWindow(int mode);
static void centerWindow();
static void showCursor(bool show);
static void grabMouse(bool grab);
static void toggleFullscreen();
static void takeSnapshot();
static void togglePause();
static uInt32 maxWindowSizeForScreen();
static uInt32 getTicks();

static bool setupProperties(PropertiesSet& set);
static void handleRCFile();
static void usage();

// Globals for the SDL stuff
static SDL_Surface* screen = (SDL_Surface*) NULL;
static Uint32 palette[256];
static int bpp;
static Display* theX11Display = (Display*) NULL;
static Window theX11Window = 0;
static int theX11Screen = 0;
static int mouseX = 0;
static bool x11Available = false;
static SDL_SysWMinfo info;
static int sdlflags;
static RectList* rectList = (RectList*) NULL;
static uInt32 theWidth, theHeight, theMaxWindowSize, theWindowSize;
static string theSnapShotDir, theSnapShotName;

#ifdef HAVE_JOYSTICK
  static SDL_Joystick* theLeftJoystick = (SDL_Joystick*) NULL;
  static SDL_Joystick* theRightJoystick = (SDL_Joystick*) NULL;
#endif

#ifdef HAVE_PNG
  static Snapshot* snapshot;
#endif

struct Switches
{
  SDLKey scanCode;
  StellaEvent::KeyCode keyCode;
};

// Place the most used keys first to speed up access
static Switches list[] = {
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

    { SDLK_UP,          StellaEvent::KCODE_UP         },
    { SDLK_DOWN,        StellaEvent::KCODE_DOWN       },
    { SDLK_LEFT,        StellaEvent::KCODE_LEFT       },
    { SDLK_RIGHT,       StellaEvent::KCODE_RIGHT      },
    { SDLK_SPACE,       StellaEvent::KCODE_SPACE      },
    { SDLK_LCTRL,       StellaEvent::KCODE_CTRL       },
    { SDLK_RCTRL,       StellaEvent::KCODE_CTRL       },
    { SDLK_LALT,        StellaEvent::KCODE_ALT        },
    { SDLK_RALT,        StellaEvent::KCODE_ALT        },

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
    { SDLK_RETURN,      StellaEvent::KCODE_RETURN     },
    { SDLK_PAUSE,       StellaEvent::KCODE_PAUSE      },
    { SDLK_ESCAPE,      StellaEvent::KCODE_ESCAPE     },
    { SDLK_COMMA,       StellaEvent::KCODE_COMMA      },
    { SDLK_PERIOD,      StellaEvent::KCODE_PERIOD     },
    { SDLK_SLASH,       StellaEvent::KCODE_SLASH      },
    { SDLK_BACKSLASH,   StellaEvent::KCODE_BACKSLASH  },
    { SDLK_SEMICOLON,   StellaEvent::KCODE_SEMICOLON  },
    { SDLK_QUOTE,       StellaEvent::KCODE_QUOTE      },
    { SDLK_BACKQUOTE,   StellaEvent::KCODE_BACKQUOTE  },
    { SDLK_LEFTBRACKET, StellaEvent::KCODE_LEFTBRACKET},
    { SDLK_RIGHTBRACKET,StellaEvent::KCODE_RIGHTBRACKET}
  };

static Event theEvent;
static Event keyboardEvent;

// Pointer to the console object or the null pointer
static Console* theConsole = (Console*) NULL;

// Pointer to the sound object or the null pointer
static Sound* sound = (Sound*) NULL;

// Indicates if the user wants to quit
static bool theQuitIndicator = false;

// Indicates if the emulator should be paused
static bool thePauseIndicator = false;

// Indicates if the mouse should be grabbed
static bool theGrabMouseIndicator = false;

// Indicates if the mouse cursor should be hidden
static bool theHideCursorIndicator = false;

// Indicates if the entire frame should be redrawn
static bool theRedrawEntireFrameIndicator = true;

// Indicates whether the game is currently in fullscreen
static bool isFullscreen = false;

// Indicates whether the window is currently centered
static bool isCentered = false;

// The locations for various required files
static string homeDir;
static string stateDir;
static string homePropertiesFile;
static string systemPropertiesFile;
static string homeRCFile;
static string systemRCFile;


/**
  This routine should be called once the console is created to setup
  the SDL window for us to use.  Return false if any operation fails,
  otherwise return true.
*/
bool setupDisplay()
{
  Uint32 initflags = SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER;
  if(SDL_Init(initflags) < 0)
    return false;

  // Check which system we are running under
  x11Available = false;
  SDL_VERSION(&info.version);
  if(SDL_GetWMInfo(&info) > 0)
    if(info.subsystem == SDL_SYSWM_X11)
      x11Available = true;

  sdlflags = SDL_SWSURFACE;
  sdlflags |= theConsole->settings().theUseFullScreenFlag ? SDL_FULLSCREEN : 0;
  sdlflags |= theConsole->settings().theUsePrivateColormapFlag ? SDL_HWPALETTE : 0;

  // Get the desired width and height of the display
  theWidth  = theConsole->mediaSource().width();
  theHeight = theConsole->mediaSource().height();

  // Get the maximum size of a window for THIS screen
  // Must be called after display and screen are known, as well as
  // theWidth and theHeight
  // Defaults to 3 on systems without X11, maximum of 4 on any system.
  theMaxWindowSize = maxWindowSizeForScreen();

  // If theWindowSize is not 0, then it must have been set on the commandline
  // Now we check to see if it is within bounds
  if(theConsole->settings().theWindowSize != 0)
  {
    if(theConsole->settings().theWindowSize < 1)
      theWindowSize = 1;
    else if(theConsole->settings().theWindowSize > theMaxWindowSize)
      theWindowSize = theMaxWindowSize;
    else
      theWindowSize = theConsole->settings().theWindowSize;
  }
  else  // theWindowSize hasn't been set so we do the default
  {
    if(theMaxWindowSize < 2)
      theWindowSize = 1;
    else
      theWindowSize = 2;
  }

#ifdef HAVE_PNG
  // Take care of the snapshot stuff.
  snapshot = new Snapshot();

  if(theConsole->settings().theSnapShotDir == "")
    theSnapShotDir = homeDir;
  else
    theSnapShotDir = theConsole->settings().theSnapShotDir;

  if(theConsole->settings().theSnapShotName == "")
    theSnapShotName = "romname";
  else
    theSnapShotName = theConsole->settings().theSnapShotName;
#endif

  // Set up the rectangle list to be used in updateDisplay
  rectList = new RectList();
  if(!rectList)
  {
    cerr << "ERROR: Unable to get memory for SDL rects" << endl;
    return false;
  }

  // Set the window title and icon
  ostringstream name;
  name << "Stella: \"" << theConsole->properties().get("Cartridge.Name") << "\"";
  SDL_WM_SetCaption(name.str().c_str(), "stella");

  // Create the screen
  if(!createScreen())
    return false;
  setupPalette();

  // Make sure that theUseFullScreenFlag sets up fullscreen mode correctly
  theGrabMouseIndicator  = theConsole->settings().theGrabMouseFlag;
  theHideCursorIndicator = theConsole->settings().theHideCursorFlag;
  if(theConsole->settings().theUseFullScreenFlag)
  {
    grabMouse(true);
    showCursor(false);
    isFullscreen = true;
  }
  else
  {
    // Keep mouse in game window if grabmouse is selected
    grabMouse(theGrabMouseIndicator);

    // Show or hide the cursor depending on the 'hidecursor' argument
    showCursor(!theHideCursorIndicator);
  }

  // Center the window if centering is selected and not fullscreen
  if(theConsole->settings().theCenterWindowFlag && !theConsole->settings().theUseFullScreenFlag)
    centerWindow();

  return true;
}


/**
  This routine should be called once setupDisplay is called
  to create the joystick stuff.
*/
bool setupJoystick()
{
#ifdef HAVE_JOYSTICK
  if(SDL_NumJoysticks() <= 0)
  {
    if(theConsole->settings().theShowInfoFlag)
      cout << "No joysticks present, use the keyboard.\n";
    theLeftJoystick = theRightJoystick = 0;
    return true;
  }

  if((theLeftJoystick = SDL_JoystickOpen(0)) != NULL)
  {
    if(theConsole->settings().theShowInfoFlag)
      cout << "Left joystick is a " << SDL_JoystickName(0) <<
        " with " << SDL_JoystickNumButtons(theLeftJoystick) << " buttons.\n";
  }
  else
  {
    if(theConsole->settings().theShowInfoFlag)
      cout << "Left joystick not present, use keyboard instead.\n";
  }

  if((theRightJoystick = SDL_JoystickOpen(1)) != NULL)
  {
    if(theConsole->settings().theShowInfoFlag)
      cout << "Right joystick is a " << SDL_JoystickName(1) <<
        " with " << SDL_JoystickNumButtons(theRightJoystick) << " buttons.\n";
  }
  else
  {
    if(theConsole->settings().theShowInfoFlag)
      cout << "Right joystick not present, use keyboard instead.\n";
  }
#endif

  return true;
}


/**
  This routine is called whenever the screen needs to be recreated.
  It updates the global screen variable.  When this happens, the
  8-bit palette needs to be recalculated.
*/
bool createScreen()
{
  int w = theWidth  * theWindowSize * 2;
  int h = theHeight * theWindowSize;

  screen = SDL_SetVideoMode(w, h, 0, sdlflags);
  if(screen == NULL)
  {
    cerr << "ERROR: Unable to open SDL window: " << SDL_GetError() << endl;
    return false;
  }

  bpp = screen->format->BitsPerPixel;
  if(bpp == 8)
    recalculate8BitPalette();

  theRedrawEntireFrameIndicator = true;

  return true;
}


/**
  Recalculates palette of an 8-bit (256 color) screen.
*/
void recalculate8BitPalette()
{
  if(bpp != 8)
    return;

  // Map 2600 colors to the current screen
  const uInt32* gamePalette = theConsole->mediaSource().palette();
  SDL_Color colors[256];
  for(uInt32 i = 0; i < 256; ++i)
  {
    Uint8 r, g, b;

    r = (Uint8) ((gamePalette[i] & 0x00ff0000) >> 16);
    g = (Uint8) ((gamePalette[i] & 0x0000ff00) >> 8);
    b = (Uint8) (gamePalette[i] & 0x000000ff);

    colors[i].r = r;
    colors[i].g = g;
    colors[i].b = b;
  }
  SDL_SetColors(screen, colors, 0, 256);

  // Now see which colors we actually got
  SDL_PixelFormat* format = screen->format;
  for(uInt32 i = 0; i < 256; ++i)
  {
    Uint8 r, g, b;

    r = format->palette->colors[i].r;
    g = format->palette->colors[i].g;
    b = format->palette->colors[i].b;

    palette[i] = SDL_MapRGB(format, r, g, b);
  }

  theRedrawEntireFrameIndicator = true;
}


/**
  Set up the palette for a screen of any depth.
  Calls recalculate8BitPalette if necessary.
*/
void setupPalette()
{
  if(bpp == 8)
  {
    recalculate8BitPalette();
    return;
  }

  // Make the palette be 75% as bright if pause is selected
  float shade = 1.0;
  if(thePauseIndicator)
    shade = 0.75;

  const uInt32* gamePalette = theConsole->mediaSource().palette();
  for(uInt32 i = 0; i < 256; ++i)
  {
    Uint8 r, g, b;

    r = (Uint8) (((gamePalette[i] & 0x00ff0000) >> 16) * shade);
    g = (Uint8) (((gamePalette[i] & 0x0000ff00) >> 8) * shade);
    b = (Uint8) ((gamePalette[i] & 0x000000ff) * shade);

    switch(bpp)
    {
      case 15:
        palette[i] = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
        break;

      case 16:
        palette[i] = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
        break;

      case 24:
      case 32:
        palette[i] = (r << 16) | (g << 8) | b;
        break;
    }
  }

  theRedrawEntireFrameIndicator = true;
}


/**
  This routine is called when the program is about to quit.
*/
void doQuit()
{
  theQuitIndicator = true;
}


/**
  This routine is called when the user wants to resize the window.
  A '1' argument indicates that the window should increase in size, while '0'
  indicates that the windows should decrease in size.  A '-1' indicates that
  the window should be sized according to the current properties.
  Can't resize in fullscreen mode.  Will only resize up to the maximum size
  of the screen.
*/
void resizeWindow(int mode)
{
  // reset size to that given in properties
  // this is a special case of allowing a resize while in fullscreen mode
  if(mode == -1)
  {
    theWidth         = theConsole->mediaSource().width();
    theHeight        = theConsole->mediaSource().height();
    theMaxWindowSize = maxWindowSizeForScreen();
  }
  else if(mode == 1)   // increase size
  {
    if(isFullscreen)
      return;

    if(theWindowSize == theMaxWindowSize)
      theWindowSize = 1;
    else
      theWindowSize++;
  }
  else if(mode == 0)   // decrease size
  {
    if(isFullscreen)
      return;

    if(theWindowSize == 1)
      theWindowSize = theMaxWindowSize;
    else
      theWindowSize--;
  }

  if(!createScreen())
    return;

  // A resize may mean that the window is no longer centered
  isCentered = false;

  if(theConsole->settings().theCenterWindowFlag)
    centerWindow();
}


/**
  Centers the game window onscreen.  Only works in X11 for now.
*/
void centerWindow()
{
  if(!x11Available)
  {
    cerr << "Window centering only available under X11.\n";
    return;
  }

  if(isFullscreen || isCentered)
    return;

  int x, y, w, h;
  info.info.x11.lock_func();
  theX11Display = info.info.x11.display;
  theX11Window  = info.info.x11.wmwindow;
  theX11Screen  = DefaultScreen(theX11Display);

  w = DisplayWidth(theX11Display, theX11Screen);
  h = DisplayHeight(theX11Display, theX11Screen);
  x = (w - screen->w)/2;
  y = (h - screen->h)/2;

  XMoveWindow(theX11Display, theX11Window, x, y);
  info.info.x11.unlock_func();

  isCentered = true;
  theRedrawEntireFrameIndicator = true;
}


/**
  Toggles between fullscreen and window mode.  Grabmouse and hidecursor
  activated when in fullscreen mode.
*/
void toggleFullscreen()
{
  int width  = theWidth  * theWindowSize * 2;
  int height = theHeight * theWindowSize;

  isFullscreen = !isFullscreen;
  if(isFullscreen)
    sdlflags |= SDL_FULLSCREEN;
  else
    sdlflags &= ~SDL_FULLSCREEN;

  if(!createScreen())
    return;

  if(isFullscreen)  // now in fullscreen mode
  {
    grabMouse(true);
    showCursor(false);
  }
  else    // now in windowed mode
  {
    grabMouse(theGrabMouseIndicator);
    showCursor(!theHideCursorIndicator);

    if(theConsole->settings().theCenterWindowFlag)
        centerWindow();
  }
}


/**
  Toggles pausing of the emulator
*/
void togglePause()
{
  if(thePauseIndicator)	// emulator is already paused so continue
  {
    thePauseIndicator = false;
  }
  else	// we want to pause the game
  {
    thePauseIndicator = true;
  }

  // Pause the console
  theConsole->mediaSource().pause(thePauseIndicator);

  // Show a different palette depending on pause state
  setupPalette();
}


/**
  Shows or hides the cursor based on the given boolean value.
*/
void showCursor(bool show)
{
  if(show)
    SDL_ShowCursor(SDL_ENABLE);
  else
    SDL_ShowCursor(SDL_DISABLE);
}


/**
  Grabs or ungrabs the mouse based on the given boolean value.
*/
void grabMouse(bool grab)
{
  if(grab)
    SDL_WM_GrabInput(SDL_GRAB_ON);
  else
    SDL_WM_GrabInput(SDL_GRAB_OFF);
}


/**
  This routine should be called anytime the display needs to be updated
*/
void updateDisplay(MediaSource& mediaSource)
{
  uInt8* currentFrame = mediaSource.currentFrameBuffer();
  uInt8* previousFrame = mediaSource.previousFrameBuffer();
  uInt16 screenMultiple = (uInt16) theWindowSize;

  uInt32 width  = theWidth;
  uInt32 height = theHeight;

  struct Rectangle
  {
    uInt8 color;
    uInt16 x, y, width, height;
  } rectangles[2][160];

  // Start a new reclist on each display update
  rectList->start();

  // This array represents the rectangles that need displaying
  // on the current scanline we're processing
  Rectangle* currentRectangles = rectangles[0];

  // This array represents the rectangles that are still active
  // from the previous scanlines we have processed
  Rectangle* activeRectangles = rectangles[1];

  // Indicates the number of active rectangles
  uInt16 activeCount = 0;

  // This update procedure requires theWidth to be a multiple of four.  
  // This is validated when the properties are loaded.
  for(uInt16 y = 0; y < height; ++y)
  {
    // Indicates the number of current rectangles
    uInt16 currentCount = 0;

    // Look at four pixels at a time to see if anything has changed
    uInt32* current = (uInt32*)(currentFrame); 
    uInt32* previous = (uInt32*)(previousFrame);

    for(uInt16 x = 0; x < width; x += 4, ++current, ++previous)
    {
      // Has something changed in this set of four pixels?
      if((*current != *previous) || theRedrawEntireFrameIndicator)
      {
        uInt8* c = (uInt8*)current;
        uInt8* p = (uInt8*)previous;

        // Look at each of the bytes that make up the uInt32
        for(uInt16 i = 0; i < 4; ++i, ++c, ++p)
        {
          // See if this pixel has changed
          if((*c != *p) || theRedrawEntireFrameIndicator)
          {
            // Can we extend a rectangle or do we have to create a new one?
            if((currentCount != 0) && 
               (currentRectangles[currentCount - 1].color == *c) &&
               ((currentRectangles[currentCount - 1].x + 
                 currentRectangles[currentCount - 1].width) == (x + i)))
            {
              currentRectangles[currentCount - 1].width += 1;
            }
            else
            {
              currentRectangles[currentCount].x = x + i;
              currentRectangles[currentCount].y = y;
              currentRectangles[currentCount].width = 1;
              currentRectangles[currentCount].height = 1;
              currentRectangles[currentCount].color = *c;
              currentCount++;
            }
          }
        }
      }
    }

    // Merge the active and current rectangles flushing any that are of no use
    uInt16 activeIndex = 0;

    for(uInt16 t = 0; (t < currentCount) && (activeIndex < activeCount); ++t)
    {
      Rectangle& current = currentRectangles[t];
      Rectangle& active = activeRectangles[activeIndex];

      // Can we merge the current rectangle with an active one?
      if((current.x == active.x) && (current.width == active.width) &&
         (current.color == active.color))
      {
        current.y = active.y;
        current.height = active.height + 1;

        ++activeIndex;
      }
      // Is it impossible for this active rectangle to be merged?
      else if(current.x >= active.x)
      {
        // Flush the active rectangle
        SDL_Rect temp;

        temp.x = active.x * 2 * screenMultiple;
        temp.y = active.y * screenMultiple;
        temp.w = active.width * 2 * screenMultiple;
        temp.h = active.height * screenMultiple;

        rectList->add(&temp);
        SDL_FillRect(screen, &temp, palette[active.color]);

        ++activeIndex;
      }
    }

    // Flush any remaining active rectangles
    for(uInt16 s = activeIndex; s < activeCount; ++s)
    {
      Rectangle& active = activeRectangles[s];

      SDL_Rect temp;
      temp.x = active.x * 2 * screenMultiple;
      temp.y = active.y * screenMultiple;
      temp.w = active.width * 2 * screenMultiple;
      temp.h = active.height * screenMultiple;

      rectList->add(&temp);
      SDL_FillRect(screen, &temp, palette[active.color]);
    }

    // We can now make the current rectangles into the active rectangles
    Rectangle* tmp = currentRectangles;
    currentRectangles = activeRectangles;
    activeRectangles = tmp;
    activeCount = currentCount;
 
    currentFrame += width;
    previousFrame += width;
  }

  // Flush any rectangles that are still active
  for(uInt16 t = 0; t < activeCount; ++t)
  {
    Rectangle& active = activeRectangles[t];

    SDL_Rect temp;
    temp.x = active.x * 2 * screenMultiple;
    temp.y = active.y * screenMultiple;
    temp.w = active.width * 2 * screenMultiple;
    temp.h = active.height * screenMultiple;

    rectList->add(&temp);
    SDL_FillRect(screen, &temp, palette[active.color]);
  }

  // Now update all the rectangles at once
  SDL_UpdateRects(screen, rectList->numRects(), rectList->rects());

  // The frame doesn't need to be completely redrawn anymore
  theRedrawEntireFrameIndicator = false;
}


/**
  This routine should be called regularly to handle events
*/
void handleEvents()
{
  SDL_Event event;
  Uint8 axis;
  Uint8 button;
  Sint16 value;
  Uint8 type;
  Uint8 state;
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

      if(key == SDLK_ESCAPE)
      {
        doQuit();
      }
      else if(key == SDLK_EQUALS)
      {
        resizeWindow(1);
      }
      else if(key == SDLK_MINUS)
      {
        resizeWindow(0);
      }
      else if(key == SDLK_RETURN && mod & KMOD_ALT)
      {
        toggleFullscreen();
      }
      else if(key == SDLK_F12)
      {
        takeSnapshot();
      }
      else if(key == SDLK_PAUSE)
      {
        togglePause();
      }
      else if(key == SDLK_g)
      {
        // don't change grabmouse in fullscreen mode
        if(!isFullscreen)
        {
          theGrabMouseIndicator = !theGrabMouseIndicator;
          grabMouse(theGrabMouseIndicator);
        }
      }
      else if(key == SDLK_h)
      {
        // don't change hidecursor in fullscreen mode
        if(!isFullscreen)
        {
          theHideCursorIndicator = !theHideCursorIndicator;
          showCursor(!theHideCursorIndicator);
        }
      }
#ifdef DEVELOPER_SUPPORT
      if(key == SDLK_f)               // Alt-f switches between NTSC and PAL
      {
        if(mod & KMOD_ALT)
        {
          theConsole->toggleFormat();

          // update the palette
          setupPalette();
        }
      }

      else if(key == SDLK_END)        // End decreases XStart
      {                               // Alt-End decreases Width
        if(mod & KMOD_ALT)
          theConsole->changeWidth(0);
        else
          theConsole->changeXStart(0);

        // Make sure changes to the properties are reflected onscreen
        resizeWindow(-1);
      }
      else if(key == SDLK_HOME)       // Home increases XStart
      {                               // Alt-Home increases Width
        if(mod & KMOD_ALT)
          theConsole->changeWidth(1);
        else
          theConsole->changeXStart(1);

        // Make sure changes to the properties are reflected onscreen
        resizeWindow(-1);
      }
      else if(key == SDLK_PAGEDOWN)   // PageDown decreases YStart
      {                               // Alt-PageDown decreases Height
        if(mod & KMOD_ALT)
          theConsole->changeHeight(0);
        else
          theConsole->changeYStart(0);

        // Make sure changes to the properties are reflected onscreen
        resizeWindow(-1);
      }
      else if(key == SDLK_PAGEUP)     // PageUp increases YStart
      {                               // Alt-PageUp increases Height
        if(mod & KMOD_ALT)
          theConsole->changeHeight(1);
        else
          theConsole->changeYStart(1);

        // Make sure changes to the properties are reflected onscreen
        resizeWindow(-1);
      }
      else if(key == SDLK_s)          // Alt-s saves properties to a file
      {
        if(mod & KMOD_ALT)
        {
          if(theConsole->settings().theMergePropertiesFlag)  // Attempt to merge with propertiesSet
          {
            theConsole->saveProperties(homePropertiesFile, true);
          }
          else  // Save to file in home directory
          {
            string newPropertiesFile = homeDir + "/" + \
              theConsole->properties().get("Cartridge.Name") + ".pro";
            replace(newPropertiesFile.begin(), newPropertiesFile.end(), ' ', '_');
            theConsole->saveProperties(newPropertiesFile);
          }
        }
      }
#endif
      else // check all the other keys
      {
        for(unsigned int i = 0; i < sizeof(list) / sizeof(Switches); ++i)
        {
          if(list[i].scanCode == key)
          {
            theConsole->eventHandler().sendKeyEvent(list[i].keyCode,
              StellaEvent::KSTATE_PRESSED);
          }
        }
      }
    }
    else if(event.type == SDL_KEYUP)
    {
      key = event.key.keysym.sym;
      type = event.type;

      for(unsigned int i = 0; i < sizeof(list) / sizeof(Switches); ++i)
      { 
        if(list[i].scanCode == key)
        {
          theConsole->eventHandler().sendKeyEvent(list[i].keyCode,
            StellaEvent::KSTATE_RELEASED);
        }
      }
    }
    else if(event.type == SDL_MOUSEMOTION)
    {
      int resistance = 0, x = 0;
      float fudgeFactor = 1000000.0;
      Int32 width   = theWidth * theWindowSize * 2;

      // Grabmouse and hidecursor introduce some lag into the mouse movement,
      // so we need to fudge the numbers a bit
      if(theGrabMouseIndicator && theHideCursorIndicator)
      {
        mouseX = (int)((float)mouseX + (float)event.motion.xrel
                 * 1.5 * (float) theWindowSize);
      }
      else
      {
        mouseX = mouseX + event.motion.xrel * theWindowSize;
      }

      // Check to make sure mouseX is within the game window
      if(mouseX < 0)
        mouseX = 0;
      else if(mouseX > width)
        mouseX = width;
  
      x = width - mouseX;
      resistance = (Int32)((fudgeFactor * x) / width);

      // Now, set the event of the correct paddle to the calculated resistance
      if(theConsole->settings().thePaddleMode == 0)
        theEvent.set(Event::PaddleZeroResistance, resistance);
      else if(theConsole->settings().thePaddleMode == 1)
        theEvent.set(Event::PaddleOneResistance, resistance);
      else if(theConsole->settings().thePaddleMode == 2)
        theEvent.set(Event::PaddleTwoResistance, resistance);
      else if(theConsole->settings().thePaddleMode == 3)
        theEvent.set(Event::PaddleThreeResistance, resistance);
    }
    else if(event.type == SDL_MOUSEBUTTONDOWN)
    {
      if(theConsole->settings().thePaddleMode == 0)
        theEvent.set(Event::PaddleZeroFire, 1);
      else if(theConsole->settings().thePaddleMode == 1)
        theEvent.set(Event::PaddleOneFire, 1);
      else if(theConsole->settings().thePaddleMode == 2)
        theEvent.set(Event::PaddleTwoFire, 1);
      else if(theConsole->settings().thePaddleMode == 3)
        theEvent.set(Event::PaddleThreeFire, 1);
    }
    else if(event.type == SDL_MOUSEBUTTONUP)
    {
      if(theConsole->settings().thePaddleMode == 0)
        theEvent.set(Event::PaddleZeroFire, 0);
      else if(theConsole->settings().thePaddleMode == 1)
        theEvent.set(Event::PaddleOneFire, 0);
      else if(theConsole->settings().thePaddleMode == 2)
        theEvent.set(Event::PaddleTwoFire, 0);
      else if(theConsole->settings().thePaddleMode == 3)
        theEvent.set(Event::PaddleThreeFire, 0);
    }
    else if(event.type == SDL_ACTIVEEVENT)
    {
      if((event.active.state & SDL_APPACTIVE) && (event.active.gain == 0))
      {
        if(!thePauseIndicator)
        {
          togglePause();
        }
      }
    }
    else if(event.type == SDL_QUIT)
    {
      doQuit();
    }

#ifdef HAVE_JOYSTICK
    // Read joystick events and modify event states
    if(theLeftJoystick)
    {
      if(((event.type == SDL_JOYBUTTONDOWN) || (event.type == SDL_JOYBUTTONUP))
          && (event.jbutton.which == 0))
      {
        button = event.jbutton.button;
        state = event.jbutton.state;
        state = (state == SDL_PRESSED) ? 1 : 0;

        if(button == 0)  // fire button
        {
          theEvent.set(Event::JoystickZeroFire, state ? 
              1 : keyboardEvent.get(Event::JoystickZeroFire));

          // If we're using real paddles then set paddle event as well
          if(theConsole->settings().thePaddleMode == 4)
            theEvent.set(Event::PaddleZeroFire, state);
        }
        else if(button == 1)  // booster button
        {
          theEvent.set(Event::BoosterGripZeroTrigger, state ? 
              1 : keyboardEvent.get(Event::BoosterGripZeroTrigger));

          // If we're using real paddles then set paddle event as well
          if(theConsole->settings().thePaddleMode == 4)
            theEvent.set(Event::PaddleOneFire, state);
        }
      }
      else if((event.type == SDL_JOYAXISMOTION) && (event.jaxis.which == 0))
      {
        axis = event.jaxis.axis;
        value = event.jaxis.value;

        if(axis == 0)  // x-axis
        {
          theEvent.set(Event::JoystickZeroLeft, (value < -16384) ? 
              1 : keyboardEvent.get(Event::JoystickZeroLeft));
          theEvent.set(Event::JoystickZeroRight, (value > 16384) ? 
              1 : keyboardEvent.get(Event::JoystickZeroRight));

          // If we're using real paddles then set paddle events as well
          if(theConsole->settings().thePaddleMode == 4)
          {
            uInt32 r = (uInt32)((1.0E6L * (value + 32767L)) / 65536);
            theEvent.set(Event::PaddleZeroResistance, r);
          }
        }
        else if(axis == 1)  // y-axis
        {
          theEvent.set(Event::JoystickZeroUp, (value < -16384) ? 
              1 : keyboardEvent.get(Event::JoystickZeroUp));
          theEvent.set(Event::JoystickZeroDown, (value > 16384) ? 
              1 : keyboardEvent.get(Event::JoystickZeroDown));

          // If we're using real paddles then set paddle events as well
          if(theConsole->settings().thePaddleMode == 4)
          {
            uInt32 r = (uInt32)((1.0E6L * (value + 32767L)) / 65536);
            theEvent.set(Event::PaddleOneResistance, r);
          }
        }
      }
    }

    if(theRightJoystick)
    {
      if(((event.type == SDL_JOYBUTTONDOWN) || (event.type == SDL_JOYBUTTONUP))
          && (event.jbutton.which == 1))
      {
        button = event.jbutton.button;
        state = event.jbutton.state;
        state = (state == SDL_PRESSED) ? 1 : 0;

        if(button == 0)  // fire button
        {
          theEvent.set(Event::JoystickOneFire, state ? 
              1 : keyboardEvent.get(Event::JoystickOneFire));

          // If we're using real paddles then set paddle event as well
          if(theConsole->settings().thePaddleMode == 4)
            theEvent.set(Event::PaddleTwoFire, state);
        }
        else if(button == 1)  // booster button
        {
          theEvent.set(Event::BoosterGripOneTrigger, state ? 
              1 : keyboardEvent.get(Event::BoosterGripOneTrigger));

          // If we're using real paddles then set paddle event as well
          if(theConsole->settings().thePaddleMode == 4)
            theEvent.set(Event::PaddleThreeFire, state);
        }
      }
      else if((event.type == SDL_JOYAXISMOTION) && (event.jaxis.which == 1))
      {
        axis = event.jaxis.axis;
        value = event.jaxis.value;

        if(axis == 0)  // x-axis
        {
          theEvent.set(Event::JoystickOneLeft, (value < -16384) ? 
              1 : keyboardEvent.get(Event::JoystickOneLeft));
          theEvent.set(Event::JoystickOneRight, (value > 16384) ? 
              1 : keyboardEvent.get(Event::JoystickOneRight));

          // If we're using real paddles then set paddle events as well
          if(theConsole->settings().thePaddleMode == 4)
          {
            uInt32 r = (uInt32)((1.0E6L * (value + 32767L)) / 65536);
            theEvent.set(Event::PaddleTwoResistance, r);
          }
        }
        else if(axis == 1)  // y-axis
        {
          theEvent.set(Event::JoystickOneUp, (value < -16384) ? 
              1 : keyboardEvent.get(Event::JoystickOneUp));
          theEvent.set(Event::JoystickOneDown, (value > 16384) ? 
              1 : keyboardEvent.get(Event::JoystickOneDown));

          // If we're using real paddles then set paddle events as well
          if(theConsole->settings().thePaddleMode == 4)
          {
            uInt32 r = (uInt32)((1.0E6L * (value + 32767L)) / 65536);
            theEvent.set(Event::PaddleThreeResistance, r);
          }
        }
      }
    }
#endif
  }
}


/**
  Called when the user wants to take a snapshot of the current display.
  Images are stored in png format in the directory specified by the 'ssdir'
  argument, or in $HOME by default.
  Images are named consecutively as "NAME".png, where name is specified by
  the 'ssname' argument.  If that name exists, they are named as "Name"_x.png,
  where x starts with 1 and increases if the previous name already exists.
  All spaces in filenames are converted to underscore '_'.
  If theMultipleSnapShotFlag is false, then consecutive images are overwritten.
*/
void takeSnapshot()
{
#ifdef HAVE_PNG
  string message;

  if(!snapshot)
  {
    message = "Snapshots disabled";
    theConsole->mediaSource().showMessage(message, 120);
    return;
  }

  // Now find the correct name for the snapshot
  string path = theSnapShotDir;
  string filename;

  if(theSnapShotName == "romname")
    path = path + "/" + theConsole->properties().get("Cartridge.Name");
  else if(theSnapShotName == "md5sum")
    path = path + "/" + theConsole->properties().get("Cartridge.MD5");
  else
  {
    cerr << "ERROR: unknown name " << theSnapShotName
         << " for snapshot type" << endl;
    return;
  }

  // Replace all spaces in name with underscores
  replace(path.begin(), path.end(), ' ', '_');

  // Check whether we want multiple snapshots created
  if(theConsole->settings().theMultipleSnapShotFlag)
  {
    // Determine if the file already exists, checking each successive filename
    // until one doesn't exist
    filename = path + ".png";
    if(access(filename.c_str(), F_OK) == 0 )
    {
      ostringstream buf;
      for(uInt32 i = 1; ;++i)
      {
        buf.str("");
        buf << path << "_" << i << ".png";
        if(access(buf.str().c_str(), F_OK) == -1 )
          break;
      }
      filename = buf.str();
    }
  }
  else
    filename = path + ".png";

  // Now save the snapshot file
  snapshot->savePNG(filename, theConsole->mediaSource(), theWindowSize);

  if(access(filename.c_str(), F_OK) == 0)
  {
    message = "Snapshot saved";
    theConsole->mediaSource().showMessage(message, 120);
  }
  else
  {
    message = "Snapshot not saved";
    theConsole->mediaSource().showMessage(message, 120);
  }
#else
  string message = "Snapshots unsupported";
  theConsole->mediaSource().showMessage(message, 120);
#endif
}


/**
  Calculate the maximum window size that the current screen can hold.
  Only works in X11 for now.  If not running under X11, always return 3.
*/
uInt32 maxWindowSizeForScreen()
{
  if(!x11Available)
    return 3;

  // Otherwise, lock the screen and get the width and height
  info.info.x11.lock_func();
  theX11Display = info.info.x11.display;
  theX11Window  = info.info.x11.wmwindow;
  theX11Screen  = DefaultScreen(theX11Display);
  info.info.x11.unlock_func();

  int screenWidth  = DisplayWidth(info.info.x11.display,
      DefaultScreen(theX11Display));
  int screenHeight = DisplayHeight(info.info.x11.display,
      DefaultScreen(theX11Display));

  uInt32 multiplier = screenWidth / (theWidth * 2);
  bool found = false;

  while(!found && (multiplier > 0))
  {
    // Figure out the desired size of the window
    int width  = theWidth  * multiplier * 2;
    int height = theHeight * multiplier;

    if((width < screenWidth) && (height < screenHeight))
      found = true;
    else
      multiplier--;
  }

  if(found)
    return (multiplier > 4 ? 4 : multiplier);
  else
    return 1;
}


/**
  Display a usage message and exit the program
*/
void usage()
{
  static const char* message[] = {
    "",
    "SDL Stella version 1.4pre",
    "",
    "Usage: stella.sdl [option ...] file",
    "",
    "Valid options are:",
    "",
    "  -fps        <number>        Display the given number of frames per second",
    "  -owncmap    <0|1>           Install a private colormap",
    "  -zoom       <size>          Makes window be 'size' times normal (1 - 4)",
    "  -fullscreen <0|1>           Play the game in fullscreen mode",
    "  -grabmouse  <0|1>           Keeps the mouse in the game window",
    "  -hidecursor <0|1>           Hides the mouse cursor in the game window",
    "  -center     <0|1>           Centers the game window onscreen",
    "  -volume     <number>        Set the volume (0 - 100)",
#ifdef HAVE_JOYSTICK
    "  -paddle     <0|1|2|3|real>  Indicates which paddle the mouse should emulate",
    "                              or that real Atari 2600 paddles are being used",
#else
    "  -paddle     <0|1|2|3>       Indicates which paddle the mouse should emulate",
#endif
    "  -showinfo   <0|1>           Shows some game info",
#ifdef HAVE_PNG
    "  -ssdir      <path>          The directory to save snapshot files to",
    "  -ssname     <name>          How to name the snapshot (romname or md5sum)",
    "  -sssingle   <0|1>           Generate single snapshot instead of many",
#endif
    "  -pro        <props file>    Use the given properties file instead of stella.pro",
    "  -accurate   <0|1>           Accurate game timing (uses more CPU)",
    "",
    "  -sound      <type>          Type is one of the following:",
    "               0                Disables all sound generation",
#ifdef SOUND_ALSA
    "               alsa             ALSA version 0.9 driver",
#endif
#ifdef SOUND_OSS
    "               oss              Open Sound System driver",
#endif
#ifdef SOUND_SDL
    "               sdl              Native SDL driver",
#endif
    "",
#ifdef DEVELOPER_SUPPORT
    " DEVELOPER options (see Stella manual for details)",
    "  -Dformat                    Sets \"Display.Format\"",
    "  -Dxstart                    Sets \"Display.XStart\"",
    "  -Dwidth                     Sets \"Display.Width\"",
    "  -Dystart                    Sets \"Display.YStart\"",
    "  -Dheight                    Sets \"Display.Height\"",
    "  -Dmerge     <0|1>           Merge changed properties into properties file,",
    "                              or save into a separate file",
#endif
    0
  };

  for(uInt32 i = 0; message[i] != 0; ++i)
  {
    cout << message[i] << endl;
  }
  exit(1);
}


/**
  Setup the properties set by first checking for a user file
  "$HOME/.stella/stella.pro", then a system-wide file "/etc/stella.pro".
  Return false if neither file is found, else return true.

  @param set The properties set to setup
*/
bool setupProperties(PropertiesSet& set, Settings& settings)
{
  bool useMemList = false;

#ifdef DEVELOPER_SUPPORT
  // If the user wishes to merge any property modifications to the
  // PropertiesSet file, then the PropertiesSet file MUST be loaded
  // into memory.
  useMemList = settings.theMergePropertiesFlag;
#endif

  // Check to see if the user has specified an alternate .pro file.
  // If it exists, use it.
  if(settings.theAlternateProFile != "")
  {
    if(access(settings.theAlternateProFile.c_str(), R_OK) == 0)
    {
      set.load(settings.theAlternateProFile, &Console::defaultProperties(), useMemList);
      return true;
    }
    else
    {
      cerr << "ERROR: Couldn't find \"" << settings.theAlternateProFile
          << "\" properties file." << endl;
      return false;
    }
  }

  if(access(homePropertiesFile.c_str(), R_OK) == 0)
  {
    set.load(homePropertiesFile, &Console::defaultProperties(), useMemList);
    return true;
  }
  else if(access(systemPropertiesFile.c_str(), R_OK) == 0)
  {
    set.load(systemPropertiesFile, &Console::defaultProperties(), useMemList);
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
  if(theConsole)
    delete theConsole;

#ifdef HAVE_PNG
  if(snapshot)
    delete snapshot;
#endif

  if(rectList)
    delete rectList;

  if(sound)
  {
    sound->closeDevice();
    delete sound;
  }

  if(SDL_WasInit(SDL_INIT_EVERYTHING))
  {
#ifdef HAVE_JOYSTICK
    if(SDL_JoystickOpened(0))
      SDL_JoystickClose(theLeftJoystick);
    if(SDL_JoystickOpened(1))
      SDL_JoystickClose(theRightJoystick);
#endif

    SDL_Quit();
  }
}


/**
  Creates some directories under $HOME.
  Required directories are $HOME/.stella and $HOME/.stella/state
  Also sets up various locations for properties files, etc.

  This must be called before any other function.
*/
bool setupDirs()
{
  homeDir = getenv("HOME");
  string path = homeDir + "/.stella";

  if(access(path.c_str(), R_OK|W_OK|X_OK) != 0 )
  {
    if(mkdir(path.c_str(), 0777) != 0)
      return false;
  }

  stateDir = homeDir + "/.stella/state/";
  if(access(stateDir.c_str(), R_OK|W_OK|X_OK) != 0 )
  {
    if(mkdir(stateDir.c_str(), 0777) != 0)
      return false;
  }

  homePropertiesFile   = homeDir + "/.stella/stella.pro";
  systemPropertiesFile = "/etc/stella.pro";
  homeRCFile           = homeDir + "/.stella/stellarc";
  systemRCFile         = "/etc/stellarc";

  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int main(int argc, char* argv[])
{
  // First set up the directories where Stella will find RC and state files
  if(!setupDirs())
  {
    cerr << "ERROR: Couldn't set up config directories.\n";
    cleanup();
    return 0;
  }

  // Create some settings for the emulator
  string infile   = "";
  string outfile  = homeRCFile;
  if(access(homeRCFile.c_str(), R_OK) == 0 )
    infile = homeRCFile;
  else if(access(systemRCFile.c_str(), R_OK) == 0 )
    infile = systemRCFile;

  Settings settings(infile, outfile);

  // Handle the command line arguments
  if(!settings.handleCommandLineArgs(argc, argv))
  {
    usage();
    cleanup();
    return 0;
  }

  // Get a pointer to the file which contains the cartridge ROM
  const char* file = argv[argc - 1];

  // Open the cartridge image and read it in
  ifstream in(file); 
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
  if(!setupProperties(propertiesSet, settings))
  {
    delete[] image;
    cleanup();
    return 0;
  }

  // Create a sound object for playing audio
  if(settings.theSoundDriver == "0")
  {
    // if sound has been disabled, we still need a sound object
    sound = new Sound();
    if(settings.theShowInfoFlag)
      cout << "Sound disabled.\n";
  }
#ifdef SOUND_ALSA
  else if(settings.theSoundDriver == "alsa")
  {
    sound = new SoundALSA();
    if(settings.theShowInfoFlag)
      cout << "Using ALSA for sound.\n";
  }
#endif
#ifdef SOUND_OSS
  else if(settings.theSoundDriver == "oss")
  {
    sound = new SoundOSS();
    if(settings.theShowInfoFlag)
      cout << "Using OSS for sound.\n";
  }
#endif
#ifdef SOUND_SDL
  else if(settings.theSoundDriver == "sdl")
  {
    sound = new SoundSDL();
    if(settings.theShowInfoFlag)
      cout << "Using SDL for sound.\n";
  }
#endif
  else   // a driver that doesn't exist was requested, so disable sound
  {
    cerr << "ERROR: Sound support for "
         << settings.theSoundDriver << " not available.\n";
    sound = new Sound();
  }

  sound->setSoundVolume(settings.theDesiredVolume);

  // Get just the filename of the file containing the ROM image
  const char* filename = (!strrchr(file, '/')) ? file : strrchr(file, '/') + 1;

  // Create the 2600 game console for users or developers
#ifdef DEVELOPER_SUPPORT
  theConsole = new Console(image, size, filename, 
      settings, propertiesSet, sound->getSampleRate(),
      &settings.userDefinedProperties);
#else
  theConsole = new Console(image, size, filename, 
      settings, propertiesSet, sound->getSampleRate());
#endif

  // Free the image since we don't need it any longer
  delete[] image;

  // Setup the SDL window and joystick
  if(!setupDisplay())
  {
    cerr << "ERROR: Couldn't set up display.\n";
    cleanup();
    return 0;
  }
  if(!setupJoystick())
  {
    cerr << "ERROR: Couldn't set up joysticks.\n";
    cleanup();
    return 0;
  }

  // These variables are common to both timing options
  // and are needed to calculate the overall frames per second.
  uInt32 frameTime = 0, numberOfFrames = 0;

  if(settings.theAccurateTimingFlag)   // normal, CPU-intensive timing
  {
    // Set up accurate timing stuff
    uInt32 startTime, delta;
    uInt32 timePerFrame = (uInt32)(1000000.0 / (double)settings.theDesiredFrameRate);

    // Set the base for the timers
    frameTime = 0;

    // Main game loop
    for(;;)
    {
      // Exit if the user wants to quit
      if(theQuitIndicator)
      {
        break;
      }

      // Call handleEvents here to see if user pressed pause
      startTime = getTicks();
      handleEvents();
      if(thePauseIndicator)
      {
        updateDisplay(theConsole->mediaSource());
        SDL_Delay(10);
        continue;
      }

      theConsole->mediaSource().update();
      updateDisplay(theConsole->mediaSource());
      sound->updateSound(theConsole->mediaSource());

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
    uInt32 timePerFrame = (uInt32)(1000000.0 / (double)theConsole->settings().theDesiredFrameRate);

    // Set the base for the timers
    virtualTime = getTicks();
    frameTime = 0;

    // Main game loop
    for(;;)
    {
      // Exit if the user wants to quit
      if(theQuitIndicator)
      {
        break;
      }

      startTime = getTicks();
      handleEvents();
      if(!thePauseIndicator)
      {
        theConsole->mediaSource().update();
        sound->updateSound(theConsole->mediaSource());
      }
      updateDisplay(theConsole->mediaSource());

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

  if(settings.theShowInfoFlag)
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
//// FIXME ... put this in the eventhandler and activate on QUIT event
  settings.save();
//////////////////////////////
  cleanup();
  return 0;
}


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
