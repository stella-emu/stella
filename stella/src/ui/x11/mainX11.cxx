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
// $Id: mainX11.cxx,v 1.41 2003-09-12 18:08:54 stephena Exp $
//============================================================================

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>

#ifdef HAVE_GETTIMEOFDAY
  #include <time.h>
  #include <sys/time.h>
#endif

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include "bspf.hxx"
#include "Console.hxx"
#include "Event.hxx"
#include "StellaEvent.hxx"
#include "EventHandler.hxx"
#include "Frontend.hxx"
#include "MediaSrc.hxx"
#include "PropsSet.hxx"
#include "Sound.hxx"
#include "Settings.hxx"

#ifdef SOUND_ALSA
  #include "SoundALSA.hxx"
#endif

#ifdef SOUND_OSS
  #include "SoundOSS.hxx"
#endif

#ifdef HAVE_JOYSTICK
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <fcntl.h>
  #include <linux/joystick.h>
#endif

#ifdef UNIX
  #include "FrontendUNIX.hxx"
  #include "SettingsUNIX.hxx"
#endif

// function prototypes
// FIXME the following will be placed in a Display class eventually ...
// A graphic context for each of the 2600's colors
static GC theGCTable[256];
static bool setupDisplay();
static bool createCursors();
static void setupPalette();
static void updateDisplay(MediaSource& mediaSource);
static void resizeWindow(int mode);
static void centerWindow();
static void showCursor(bool show);
static void grabMouse(bool grab);
static void toggleFullscreen();
static uInt32 maxWindowSizeForScreen();

// Globals for X windows stuff
static Display* theDisplay = (Display*) NULL;
static string theDisplayName = "";
static int theScreen = 0;
static Visual* theVisual = (Visual*) NULL;
static Window theWindow = 0;
static Colormap thePrivateColormap = 0;
static Cursor normalCursor = 0;
static Cursor blankCursor = 0;
static uInt32 eventMask;
static Atom wm_delete_window;
static uInt32 theWidth, theHeight, theMaxWindowSize, theWindowSize;
////////////////////////////////////////////

static void cleanup();
static bool setupJoystick();
static void handleEvents();

static uInt32 getTicks();
static bool setupProperties(PropertiesSet& set);
static void handleRCFile();
static void usage();

#ifdef HAVE_JOYSTICK
  // File descriptors for the joystick devices
  static int theLeftJoystickFd;
  static int theRightJoystickFd;
  static uInt32 theLeftJoystickNumber;
  static uInt32 theRightJoystickNumber;
//  static uInt32 thePaddleNumber;
#endif

// Pointer to the console object or the null pointer
static Console* theConsole = (Console*) NULL;

// Pointer to the sound object or the null pointer
static Sound* theSound = (Sound*) NULL;

// Pointer to the frontend object or the null pointer
static Frontend* theFrontend = (Frontend*) NULL;

// Pointer to the settings object or the null pointer
#ifdef UNIX
  static SettingsUNIX* theSettings = (SettingsUNIX*) NULL;
#endif

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

struct Switches
{
  KeySym scanCode;
  StellaEvent::KeyCode keyCode;
};

// Place the most used keys first to speed up access
static Switches keyList[] = {
    { XK_F1,          StellaEvent::KCODE_F1         },
    { XK_F2,          StellaEvent::KCODE_F2         },
    { XK_F3,          StellaEvent::KCODE_F3         },
    { XK_F4,          StellaEvent::KCODE_F4         },
    { XK_F5,          StellaEvent::KCODE_F5         },
    { XK_F6,          StellaEvent::KCODE_F6         },
    { XK_F7,          StellaEvent::KCODE_F7         },
    { XK_F8,          StellaEvent::KCODE_F8         },
    { XK_F9,          StellaEvent::KCODE_F9         },
    { XK_F10,         StellaEvent::KCODE_F10        },
    { XK_F11,         StellaEvent::KCODE_F11        },
    { XK_F12,         StellaEvent::KCODE_F12        },

    { XK_Up,          StellaEvent::KCODE_UP         },
    { XK_Down,        StellaEvent::KCODE_DOWN       },
    { XK_Left,        StellaEvent::KCODE_LEFT       },
    { XK_Right,       StellaEvent::KCODE_RIGHT      },
    { XK_space,       StellaEvent::KCODE_SPACE      },
    { XK_Control_L,   StellaEvent::KCODE_CTRL       },
    { XK_Control_R,   StellaEvent::KCODE_CTRL       },
    { XK_Alt_L,       StellaEvent::KCODE_ALT        },
    { XK_Alt_R,       StellaEvent::KCODE_ALT        },

    { XK_a,           StellaEvent::KCODE_a          },
    { XK_b,           StellaEvent::KCODE_b          },
    { XK_c,           StellaEvent::KCODE_c          },
    { XK_d,           StellaEvent::KCODE_d          },
    { XK_e,           StellaEvent::KCODE_e          },
    { XK_f,           StellaEvent::KCODE_f          },
    { XK_g,           StellaEvent::KCODE_g          },
    { XK_h,           StellaEvent::KCODE_h          },
    { XK_i,           StellaEvent::KCODE_i          },
    { XK_j,           StellaEvent::KCODE_j          },
    { XK_k,           StellaEvent::KCODE_k          },
    { XK_l,           StellaEvent::KCODE_l          },
    { XK_m,           StellaEvent::KCODE_m          },
    { XK_n,           StellaEvent::KCODE_n          },
    { XK_o,           StellaEvent::KCODE_o          },
    { XK_p,           StellaEvent::KCODE_p          },
    { XK_q,           StellaEvent::KCODE_q          },
    { XK_r,           StellaEvent::KCODE_r          },
    { XK_s,           StellaEvent::KCODE_s          },
    { XK_t,           StellaEvent::KCODE_t          },
    { XK_u,           StellaEvent::KCODE_u          },
    { XK_v,           StellaEvent::KCODE_v          },
    { XK_w,           StellaEvent::KCODE_w          },
    { XK_x,           StellaEvent::KCODE_x          },
    { XK_y,           StellaEvent::KCODE_y          },
    { XK_z,           StellaEvent::KCODE_z          },

    { XK_0,           StellaEvent::KCODE_0          },
    { XK_1,           StellaEvent::KCODE_1          },
    { XK_2,           StellaEvent::KCODE_2          },
    { XK_3,           StellaEvent::KCODE_3          },
    { XK_4,           StellaEvent::KCODE_4          },
    { XK_5,           StellaEvent::KCODE_5          },
    { XK_6,           StellaEvent::KCODE_6          },
    { XK_7,           StellaEvent::KCODE_7          },
    { XK_8,           StellaEvent::KCODE_8          },
    { XK_9,           StellaEvent::KCODE_9          },

    { XK_KP_0,        StellaEvent::KCODE_KP0        },
    { XK_KP_1,        StellaEvent::KCODE_KP1        },
    { XK_KP_2,        StellaEvent::KCODE_KP2        },
    { XK_KP_3,        StellaEvent::KCODE_KP3        },
    { XK_KP_4,        StellaEvent::KCODE_KP4        },
    { XK_KP_5,        StellaEvent::KCODE_KP5        },
    { XK_KP_6,        StellaEvent::KCODE_KP6        },
    { XK_KP_7,        StellaEvent::KCODE_KP7        },
    { XK_KP_8,        StellaEvent::KCODE_KP8        },
    { XK_KP_9,        StellaEvent::KCODE_KP9        },
    { XK_KP_Decimal,  StellaEvent::KCODE_KP_PERIOD  },
    { XK_KP_Divide,   StellaEvent::KCODE_KP_DIVIDE  },
    { XK_KP_Multiply, StellaEvent::KCODE_KP_MULTIPLY},
    { XK_KP_Subtract, StellaEvent::KCODE_KP_MINUS   },
    { XK_KP_Add,      StellaEvent::KCODE_KP_PLUS    },
    { XK_KP_Enter,    StellaEvent::KCODE_KP_ENTER   },
    { XK_KP_Equal,    StellaEvent::KCODE_KP_EQUALS  },

    { XK_BackSpace,   StellaEvent::KCODE_BACKSPACE  },
    { XK_Tab,         StellaEvent::KCODE_TAB        },
    { XK_Return,      StellaEvent::KCODE_RETURN     },
    { XK_Pause,       StellaEvent::KCODE_PAUSE      },
    { XK_Escape,      StellaEvent::KCODE_ESCAPE     },
    { XK_comma,       StellaEvent::KCODE_COMMA      },
    { XK_period,      StellaEvent::KCODE_PERIOD     },
    { XK_slash,       StellaEvent::KCODE_SLASH      },
    { XK_backslash,   StellaEvent::KCODE_BACKSLASH  },
    { XK_semicolon,   StellaEvent::KCODE_SEMICOLON  },
    { XK_apostrophe,  StellaEvent::KCODE_QUOTE      },
    { XK_grave,       StellaEvent::KCODE_BACKQUOTE  },
    { XK_bracketleft, StellaEvent::KCODE_LEFTBRACKET},
    { XK_bracketright,StellaEvent::KCODE_RIGHTBRACKET}
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

  uInt32 ticks = now.tv_sec * 1000000 + now.tv_usec;

  return ticks;
}
#else
  #error We need gettimeofday for the X11 version!!!
#endif


/**
  This routine should be called once the console is created to setup
  the X11 connection and open a window for us to use.  Return false if any
  operation fails, otherwise return true.
*/
bool setupDisplay()
{
  // Open a connection to the X server
  if(theDisplayName == "")
    theDisplay = XOpenDisplay(NULL);
  else
    theDisplay = XOpenDisplay(theDisplayName.c_str());

  // Verify that the connection was made
  if(theDisplay == NULL)
  {
    cerr << "ERROR: Couldn't open X Windows display...\n";
    return false;
  }

  theScreen = DefaultScreen(theDisplay);
  theVisual = DefaultVisual(theDisplay, theScreen);
  Window rootWindow = RootWindow(theDisplay, theScreen);

  // Get the desired width and height of the display
  theWidth  = theConsole->mediaSource().width();
  theHeight = theConsole->mediaSource().height();

  // Get the maximum size of a window for THIS screen
  // Must be called after display and screen are known, as well as
  // theWidth and theHeight
  theMaxWindowSize = maxWindowSizeForScreen();

  // Check to see if window size will fit in the screen
  if(theSettings->theZoomLevel > theMaxWindowSize)
    theWindowSize = theMaxWindowSize;
  else
    theWindowSize = theSettings->theZoomLevel;

  // Figure out the desired size of the window
  int width  = theWidth  * theWindowSize * 2;
  int height = theHeight * theWindowSize;

  theWindow = XCreateSimpleWindow(theDisplay, rootWindow, 0, 0,
      width, height, CopyFromParent, CopyFromParent, 
      BlackPixel(theDisplay, theScreen));
  if(!theWindow)
    return false;

  // Create normal and blank cursors.  This must be called AFTER
  // theDisplay and theWindow are defined
  if(!createCursors())
  {
    cerr << "ERROR: Couldn't create cursors.\n";
    return false;
  }

  XSizeHints hints;
  hints.flags = PSize | PMinSize | PMaxSize;
  hints.min_width = hints.max_width = hints.width = width;
  hints.min_height = hints.max_height = hints.height = height;

  // Set window and icon name, size hints and other properties 
  ostringstream name;
  name << "Stella: \"" << theConsole->properties().get("Cartridge.Name") << "\"";
  XmbSetWMProperties(theDisplay, theWindow, name.str().c_str(), "stella", 0, 0,
      &hints, None, None);

  // Set up the palette for the screen
  setupPalette();

  // Set up the delete window stuff ...
  wm_delete_window = XInternAtom(theDisplay, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(theDisplay, theWindow, &wm_delete_window, 1);

  // If requested install a private colormap for the window
  if(theSettings->theUsePrivateColormapFlag)
  {
    XSetWindowColormap(theDisplay, theWindow, thePrivateColormap);
  }

  XSelectInput(theDisplay, theWindow, ExposureMask);
  XMapWindow(theDisplay, theWindow);

  // Center the window if centering is selected and not fullscreen
  if(theSettings->theCenterWindowFlag)// && !theUseFullScreenFlag)
    centerWindow();

  XEvent event;
  do
  {
    XNextEvent(theDisplay, &event);
  } while (event.type != Expose);

  eventMask = ExposureMask | KeyPressMask | KeyReleaseMask | 
      PropertyChangeMask | StructureNotifyMask;

  // If we're using the mouse for paddle emulation then enable mouse events
  if(((theConsole->properties().get("Controller.Left") == "Paddles") ||
      (theConsole->properties().get("Controller.Right") == "Paddles"))
    && (theSettings->thePaddleMode != 4)) 
  {
    eventMask |= (PointerMotionMask | ButtonPressMask | ButtonReleaseMask);
  }

  theGrabMouseIndicator  = theSettings->theGrabMouseFlag;
  theHideCursorIndicator = theSettings->theHideCursorFlag;

  // Keep mouse in game window if grabmouse is selected
  grabMouse(theGrabMouseIndicator);
  // Show or hide the cursor depending on the 'hidecursor' argument
  showCursor(!theHideCursorIndicator);

  XSelectInput(theDisplay, theWindow, eventMask);

  return true;
}

/**
  This routine should be called once the display is created
  to create the joystick stuff
*/
bool setupJoystick()
{
#ifdef HAVE_JOYSTICK
  ostringstream joyname;

  theLeftJoystickNumber  = theSettings->theLeftJoystickNumber;
  theRightJoystickNumber = theSettings->theRightJoystickNumber;

  joyname.str("");
  joyname << "/dev/js" << theLeftJoystickNumber;
  if((theLeftJoystickFd = open(joyname.str().c_str(), O_RDONLY | O_NONBLOCK)) >= 0)
  {
    if(theSettings->theShowInfoFlag)
      cout << "Left joystick set to " << joyname.str() << "." << endl;
  }
  else
  {
    if(theSettings->theShowInfoFlag)
      cout << "Left joystick not present, use keyboard instead.\n";
  }

  joyname.str("");
  joyname << "/dev/js" << theRightJoystickNumber;
  if((theRightJoystickFd = open(joyname.str().c_str(), O_RDONLY | O_NONBLOCK)) >= 0)
  {
    if(theSettings->theShowInfoFlag)
      cout << "Right joystick set to " << joyname.str() << "." << endl;
  }
  else
  {
    if(theSettings->theShowInfoFlag)
      cout << "Right joystick not present, use keyboard instead.\n";
  }
#endif

  return true;
}

/**
  Set up the palette for a screen of any depth.
*/
void setupPalette()
{
  // If we're using a private colormap then let's free it to be safe
  if(theSettings->theUsePrivateColormapFlag && theDisplay)
  {
    if(thePrivateColormap)
      XFreeColormap(theDisplay, thePrivateColormap);

     thePrivateColormap = XCreateColormap(theDisplay, theWindow, 
       theVisual, AllocNone);
  }

  // Make the palette be 75% as bright if pause is selected
  float shade = 1.0;
  if(theFrontend->pause())
    shade = 0.75;

  // Allocate colors in the default colormap
  const uInt32* palette = theConsole->mediaSource().palette();
  for(uInt32 t = 0; t < 256; ++t)
  {
    XColor color;

    color.red = (short unsigned int)(((palette[t] & 0x00ff0000) >> 8) * shade);
    color.green = (short unsigned int)((palette[t] & 0x0000ff00) * shade);
    color.blue = (short unsigned int)(((palette[t] & 0x000000ff) << 8) * shade);
    color.flags = DoRed | DoGreen | DoBlue;

    if(theSettings->theUsePrivateColormapFlag)
      XAllocColor(theDisplay, thePrivateColormap, &color);
    else
      XAllocColor(theDisplay, DefaultColormap(theDisplay, theScreen), &color);

    XGCValues values;
    values.foreground = color.pixel;

    if(theGCTable[t])
      XFreeGC(theDisplay, theGCTable[t]);

    theGCTable[t] = XCreateGC(theDisplay, theWindow, GCForeground, &values);
  }

  theRedrawEntireFrameIndicator = true;
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
        XFillRectangle(theDisplay, theWindow, theGCTable[active.color],
           active.x * 2 * screenMultiple, active.y * screenMultiple, 
           active.width * 2 * screenMultiple, active.height * screenMultiple);

        ++activeIndex;
      }
    }

    // Flush any remaining active rectangles
    for(uInt16 s = activeIndex; s < activeCount; ++s)
    {
      Rectangle& active = activeRectangles[s];

      XFillRectangle(theDisplay, theWindow, theGCTable[active.color],
         active.x * 2 * screenMultiple, active.y * screenMultiple, 
         active.width * 2 * screenMultiple, active.height * screenMultiple);
    }

    // We can now make the current rectangles into the active rectangles
    Rectangle* tmp = currentRectangles;
    currentRectangles = activeRectangles;
    activeRectangles = tmp;
    activeCount = currentCount;
 
    currentFrame  += width;
    previousFrame += width;
  }

  // Flush any rectangles that are still active
  for(uInt16 t = 0; t < activeCount; ++t)
  {
    Rectangle& active = activeRectangles[t];

    XFillRectangle(theDisplay, theWindow, theGCTable[active.color],
       active.x * 2 * screenMultiple, active.y * screenMultiple, 
       active.width * 2 * screenMultiple, active.height * screenMultiple);
  }

  // The frame doesn't need to be completely redrawn anymore
  theRedrawEntireFrameIndicator = false;
}

/**
  This routine should be called regularly to handle events
*/
void handleEvents()
{
  XEvent event;

  // Handle the WM_DELETE_WINDOW message outside the event loop
  if(XCheckTypedWindowEvent(theDisplay, theWindow, ClientMessage, &event))
  {
    if((unsigned long)event.xclient.data.l[0] == wm_delete_window)
    {
      theConsole->eventHandler().sendEvent(Event::Quit, 1);
    }
  }

  while(XCheckWindowEvent(theDisplay, theWindow, eventMask, &event))
  {
    char buffer[20];
    KeySym key;
    XComposeStatus compose;

    if((event.type == KeyPress) || (event.type == KeyRelease))
    {
      XLookupString(&event.xkey, buffer, 20, &key, &compose);
      if((key == XK_equal) && (event.type == KeyPress))
      {
        resizeWindow(1);
      }
      else if((key == XK_minus) && (event.type == KeyPress))
      {
        resizeWindow(0);
      }
// FIXME - change x to Ctrl-x
      else if((key == XK_g) && (event.type == KeyPress))
      {
        // don't change grabmouse in fullscreen mode
        if(!isFullscreen)
        {
          theGrabMouseIndicator = !theGrabMouseIndicator;
          grabMouse(theGrabMouseIndicator);
        }
      }
      else if((key == XK_h) && (event.type == KeyPress))
      {
        // don't change hidecursor in fullscreen mode
        if(!isFullscreen)
        {
          theHideCursorIndicator = !theHideCursorIndicator;
          showCursor(!theHideCursorIndicator);
        }
      }
#ifdef DEVELOPER_SUPPORT
      else if((key == XK_f) && (event.type == KeyPress)) // Alt-f switches between NTSC and PAL
      {
        if(event.xkey.state & Mod1Mask)
        {
          theConsole->toggleFormat();

          // update the palette
          setupPalette();
        }
      }
      else if((key == XK_End) && (event.type == KeyPress))    // End decreases XStart
      {                                                       // Alt-End decreases Width
        if(event.xkey.state & Mod1Mask)
          theConsole->changeWidth(0);
        else
          theConsole->changeXStart(0);

        // Make sure changes to the properties are reflected onscreen
        resizeWindow(-1);
      }
      else if((key == XK_Home) && (event.type == KeyPress))   // Home increases XStart
      {                                                       // Alt-Home increases Width
        if(event.xkey.state & Mod1Mask)
          theConsole->changeWidth(1);
        else
          theConsole->changeXStart(1);

        // Make sure changes to the properties are reflected onscreen
        resizeWindow(-1);
      }
      else if((key == XK_Next) && (event.type == KeyPress))   // PageDown decreases YStart
      {                                                       // Alt-PageDown decreases Height
        if(event.xkey.state & Mod1Mask)
          theConsole->changeHeight(0);
        else
          theConsole->changeYStart(0);

        // Make sure changes to the properties are reflected onscreen
        resizeWindow(-1);
      }
      else if((key == XK_Prior) && (event.type == KeyPress)) // PageUp increases YStart
      {                                                         // Alt-PageUp increases Height
        if(event.xkey.state & Mod1Mask)
          theConsole->changeHeight(1);
        else
          theConsole->changeYStart(1);

        // Make sure changes to the properties are reflected onscreen
        resizeWindow(-1);
      }
      else if((key == XK_s) && (event.type == KeyPress))   // Alt-s saves properties to a file
      {
        if(event.xkey.state & Mod1Mask)
        {
          if(theSettings->theMergePropertiesFlag)  // Attempt to merge with propertiesSet
          {
            theConsole->saveProperties(theConsole->frontend().userPropertiesFilename(), true);
          }
          else  // Save to file in home directory
          {
            string newPropertiesFile = theConsole->frontend().userHomeDir() + "/" + \
              theConsole->properties().get("Cartridge.Name") + ".pro";
            replace(newPropertiesFile.begin(), newPropertiesFile.end(), ' ', '_');
            theConsole->saveProperties(newPropertiesFile);
          }
        }
      }
#endif
      else
      { 
        int state = (event.type == KeyPress) ? 1 : 0;
        for(unsigned int i = 0; i < sizeof(keyList) / sizeof(Switches); ++i)
        {
          if(keyList[i].scanCode == key)
            theConsole->eventHandler().sendKeyEvent(keyList[i].keyCode, state);
        }
      }
    }
    else if(event.type == MotionNotify)
    {
      Int32 resistance = 0;
      uInt32 width = theWidth * theWindowSize * 2;
      Event::Type type = Event::LastType;

      int x = width - event.xmotion.x;
      resistance = (Int32)((1000000.0 * x) / width);

      // Now, set the event of the correct paddle to the calculated resistance
      if(theSettings->thePaddleMode == 0)
        type = Event::PaddleZeroResistance;
      else if(theSettings->thePaddleMode == 1)
        type = Event::PaddleOneResistance;
      else if(theSettings->thePaddleMode == 2)
        type = Event::PaddleTwoResistance;
      else if(theSettings->thePaddleMode == 3)
        type = Event::PaddleThreeResistance;

      theConsole->eventHandler().sendEvent(type, resistance);
    }
    else if(event.type == ButtonPress || event.type == ButtonRelease)
    {
      Event::Type type = Event::LastType;
      Int32 value;

      value = (event.type == ButtonPress) ? 1 : 0;

      if(theSettings->thePaddleMode == 0)
        type = Event::PaddleZeroFire;
      else if(theSettings->thePaddleMode == 1)
        type = Event::PaddleOneFire;
      else if(theSettings->thePaddleMode == 2)
        type = Event::PaddleTwoFire;
      else if(theSettings->thePaddleMode == 3)
        type = Event::PaddleThreeFire;

      theConsole->eventHandler().sendEvent(type, value);
    }
    else if(event.type == Expose)
    {
      theRedrawEntireFrameIndicator = true;
    }
    else if(event.type == UnmapNotify)
    {
      if(!theFrontend->pause())
      {
        theConsole->eventHandler().sendEvent(Event::Pause, 1);
      }
    }
  }

#ifdef HAVE_JOYSTICK
  // Read joystick events and modify event states
  StellaEvent::JoyStick stick;
  StellaEvent::JoyCode code;
  uInt32 state;
  uInt8 axis;
  Int32 value;

  if(theLeftJoystickFd >= 0)
  {
    struct js_event event;
    stick = joyList[theLeftJoystickNumber];

    // Process each joystick event that's queued-up
    while(read(theLeftJoystickFd, &event, sizeof(struct js_event)) > 0)
    {
      if((event.type & ~JS_EVENT_INIT) == JS_EVENT_BUTTON)
      {
        if(event.number >= StellaEvent::LastJCODE)
          return;

        code  = joyButtonList[event.number];
        state = event.value;

        theConsole->eventHandler().sendJoyEvent(stick, code, state);
      }
      else if((event.type & ~JS_EVENT_INIT) == JS_EVENT_AXIS)
      {
        axis  = event.number;
        value = event.value;

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
    }
  }

  if(theRightJoystickFd >= 0)
  {
    struct js_event event;
    stick = joyList[theRightJoystickNumber];

    // Process each joystick event that's queued-up
    while(read(theRightJoystickFd, &event, sizeof(struct js_event)) > 0)
    {
      if((event.type & ~JS_EVENT_INIT) == JS_EVENT_BUTTON)
      {
        if(event.number >= StellaEvent::LastJCODE)
          return;

        code  = joyButtonList[event.number];
        state = event.value;

        theConsole->eventHandler().sendJoyEvent(stick, code, state);
      }
      else if((event.type & ~JS_EVENT_INIT) == JS_EVENT_AXIS)
      {
        axis  = event.number;
        value = event.value;

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
    }
  }
#endif
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
    theWidth  = theConsole->mediaSource().width();
    theHeight = theConsole->mediaSource().height();
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

  // Figure out the desired size of the window
  int width  = theWidth  * theWindowSize * 2;
  int height = theHeight * theWindowSize;

  XSizeHints hints;
  hints.flags = PSize | PMinSize | PMaxSize;
  hints.min_width = hints.max_width = hints.width = width;
  hints.min_height = hints.max_height = hints.height = height;
  XSetWMNormalHints(theDisplay, theWindow, &hints);

  XWindowChanges wc;
  wc.width = width;
  wc.height = height;
  XConfigureWindow(theDisplay, theWindow, CWWidth | CWHeight, &wc);

  theRedrawEntireFrameIndicator = true;

  // A resize probably means that the window is no longer centered
  isCentered = false;

  if(theSettings->theCenterWindowFlag)
    centerWindow();
}

/**
  Centers the game window onscreen.
*/
void centerWindow()
{
  if(isFullscreen || isCentered)
    return;

  int x, y, w, h;

  w = DisplayWidth(theDisplay, theScreen);
  h = DisplayHeight(theDisplay, theScreen);
  x = (w - (theWidth * theWindowSize * 2)) / 2;
  y = (h - (theHeight * theWindowSize)) / 2;

  XWindowChanges wc;
  wc.x = x;
  wc.y = y;
  XConfigureWindow(theDisplay, theWindow, CWX | CWY, &wc);

  XSizeHints hints;
  hints.flags = PPosition;
  hints.x = x;
  hints.y = y;
  XSetWMNormalHints(theDisplay, theWindow, &hints);

  isCentered = true;
  theRedrawEntireFrameIndicator = true;
}

/**
  Toggles between fullscreen and window mode.  Grabmouse and hidecursor
  activated when in fullscreen mode.
*/
void toggleFullscreen()
{
  cerr << "Fullscreen mode not supported.\n";
}


/**
  Shows or hides the cursor based on the given boolean value.
*/
void showCursor(bool show)
{
  if(!normalCursor || !blankCursor)
    return;

  if(show)
    XDefineCursor(theDisplay, theWindow, normalCursor);
  else
    XDefineCursor(theDisplay, theWindow, blankCursor);
}

/**
  Grabs or ungrabs the mouse based on the given boolean value.
*/
void grabMouse(bool grab)
{
  if(grab)
  {
   int result = XGrabPointer(theDisplay, theWindow, True, 0, GrabModeAsync,
                 GrabModeAsync, theWindow, None, CurrentTime);

    if(result != GrabSuccess)
      cerr << "Couldn't grab mouse!!\n";
  }
  else
    XUngrabPointer(theDisplay, CurrentTime);
}

/**
  Creates a normal and blank cursor which may be needed if hidecursor or
  fullscreen is toggled.  Return false on any errors, otherwise true.
*/
bool createCursors()
{
  if(!theDisplay || !theWindow)
    return false;

  Pixmap cursormask;
  XGCValues xgc;
  XColor dummycolour;
  GC gc;

  // First create the blank cursor
  cursormask = XCreatePixmap(theDisplay, theWindow, 1, 1, 1);
  xgc.function = GXclear;
  gc = XCreateGC(theDisplay, cursormask, GCFunction, &xgc);
  XFillRectangle(theDisplay, cursormask, gc, 0, 0, 1, 1);
  dummycolour.pixel = 0;
  dummycolour.red = 0;
  dummycolour.flags = 04;
  blankCursor = XCreatePixmapCursor(theDisplay, cursormask, cursormask,
                                    &dummycolour, &dummycolour, 0, 0);
  if(!blankCursor)
    return false;

  XFreeGC(theDisplay, gc);
  XFreePixmap(theDisplay, cursormask);

  // Now create the normal cursor
  normalCursor = XCreateFontCursor(theDisplay, XC_left_ptr);
  if(!normalCursor)
    return false;

  return true;
}


/**
  Calculate the maximum window size that the current screen can hold
*/
uInt32 maxWindowSizeForScreen()
{
  int screenWidth  = DisplayWidth(theDisplay, theScreen);
  int screenHeight = DisplayHeight(theDisplay, theScreen);

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
    return multiplier;
  else
    return 1;
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

#ifdef DEVELOPER_SUPPORT
  // If the user wishes to merge any property modifications to the
  // PropertiesSet file, then the PropertiesSet file MUST be loaded
  // into memory.
  useMemList = theSettings->theMergePropertiesFlag;
#endif

  // Check to see if the user has specified an alternate .pro file.
  if(theSettings->theAlternateProFile != "")
  {
    set.load(theSettings->theAlternateProFile, &Console::defaultProperties(), useMemList);
    return true;
  }

  if(theFrontend->userPropertiesFilename() != "")
  {
    set.load(theFrontend->userPropertiesFilename(),
             &Console::defaultProperties(), useMemList);
    return true;
  }
  else if(theFrontend->systemPropertiesFilename() != "")
  {
    set.load(theFrontend->systemPropertiesFilename(),
             &Console::defaultProperties(), useMemList);
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

  if(theFrontend)
    delete theFrontend;

  if(theConsole)
    delete theConsole;

  if(theSound)
  {
    theSound->closeDevice();
    delete theSound;
  }

  if(normalCursor)
    XFreeCursor(theDisplay, normalCursor);
  if(blankCursor)
    XFreeCursor(theDisplay, blankCursor);

  // If we're using a private colormap then let's free it to be safe
  if(theSettings->theUsePrivateColormapFlag && theDisplay)
  {
     XFreeColormap(theDisplay, thePrivateColormap);
  }

#ifdef HAVE_JOYSTICK
  // Close the joystick devices
  if(theLeftJoystickFd)
    close(theLeftJoystickFd);
  if(theRightJoystickFd)
    close(theRightJoystickFd);
#endif
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int main(int argc, char* argv[])
{
  // First set up the frontend to communicate with the emulation core
#ifdef UNIX
  theFrontend = new FrontendUNIX();
#endif
  if(!theFrontend)
  {
    cerr << "ERROR: Couldn't set up the frontend.\n";
    cleanup();
    return 0;
  }

  // Create some settings for the emulator
  string infile   = "";
  string outfile  = theFrontend->userConfigFilename();
  if(theFrontend->userConfigFilename() != "")
    infile = theFrontend->userConfigFilename();
  else if(theFrontend->systemConfigFilename() != "")
    infile = theFrontend->systemConfigFilename();

#ifdef UNIX
  theSettings = new SettingsUNIX(infile, outfile);
#endif
  if(!theSettings)
  {
    cleanup();
    return 0;
  }

  // Take care of commandline arguments
  if(!theSettings->handleCommandLineArgs(argc, argv))
  {
    string message = "Stella for X11 version 1.4\n\nUsage: stella.x11 [option ...] file";
    theSettings->usage(message);
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
  if(!setupProperties(propertiesSet))
  {
    delete[] image;
    cleanup();
    return 0;
  }

  // Create a sound object for playing audio
  if(theSettings->theSoundDriver == "0")
  {
    // if sound has been disabled, we still need a sound object
    theSound = new Sound();
    if(theSettings->theShowInfoFlag)
      cout << "Sound disabled.\n";
  }
#ifdef SOUND_ALSA
  else if(theSettings->theSoundDriver == "alsa")
  {
    theSound = new SoundALSA();
    if(theSettings->theShowInfoFlag)
      cout << "Using ALSA for sound.\n";
  }
#endif
#ifdef SOUND_OSS
  else if(theSettings->theSoundDriver == "oss")
  {
    theSound = new SoundOSS();
    if(theSettings->theShowInfoFlag)
      cout << "Using OSS for sound.\n";
  }
#endif
  else   // a driver that doesn't exist was requested, so disable sound
  {
    cerr << "ERROR: Sound support for "
         << theSettings->theSoundDriver << " not available.\n";
    theSound = new Sound();
  }

  theSound->setSoundVolume(theSettings->theDesiredVolume);

  // Get just the filename of the file containing the ROM image
  const char* filename = (!strrchr(file, '/')) ? file : strrchr(file, '/') + 1;

  // Create the 2600 game console
  theConsole = new Console(image, size, filename, *theSettings, propertiesSet,
                           *theFrontend, theSound->getSampleRate());

  // Free the image since we don't need it any longer
  delete[] image;

  // Setup X window and joysticks
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

  if(theSettings->theAccurateTimingFlag)   // normal, CPU-intensive timing
  {
    // Set up timing stuff
    uInt32 startTime, delta;
    uInt32 timePerFrame = 
        (uInt32)(1000000.0 / (double)theSettings->theDesiredFrameRate);

    // Set the base for the timers
    frameTime = 0;

    // Main game loop
    for(;;)
    {
      // Exit if the user wants to quit
      if(theFrontend->quit())
      {
        break;
      }

      // Call handleEvents here to see if user pressed pause
      startTime = getTicks();
      handleEvents();
      if(theFrontend->pause())
      {
        updateDisplay(theConsole->mediaSource());
        usleep(10000);
        continue;
      }

      theConsole->mediaSource().update();
      theSound->updateSound(theConsole->mediaSource());
      updateDisplay(theConsole->mediaSource());

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
    // Set up timing stuff
    uInt32 startTime, virtualTime, currentTime;
    uInt32 timePerFrame = 
        (uInt32)(1000000.0 / (double)theSettings->theDesiredFrameRate);

    // Set the base for the timers
    virtualTime = getTicks();
    frameTime = 0;

    // Main game loop
    for(;;)
    {
      // Exit if the user wants to quit
      if(theFrontend->quit())
      {
        break;
      }

      startTime = getTicks();
      handleEvents();
      if(!theFrontend->pause())
      {
        theConsole->mediaSource().update();
        theSound->updateSound(theConsole->mediaSource());
      }
      updateDisplay(theConsole->mediaSource());

      currentTime = getTicks();
      virtualTime += timePerFrame;
      if(currentTime < virtualTime)
      {
        usleep(virtualTime - currentTime);
      }

      currentTime = getTicks() - startTime;
      frameTime += currentTime;
      ++numberOfFrames;
    }
  }

  if(theSettings->theShowInfoFlag)
  {
    double executionTime = (double) frameTime / 1000000.0;
    double framesPerSecond = (double) numberOfFrames / executionTime;

    cout << endl;
    cout << numberOfFrames << " total frames drawn\n";
    cout << framesPerSecond << " frames/second\n";
    cout << endl;
    cout << "Cartridge Name: " 
        << theConsole->properties().get("Cartridge.Name");
    cout << endl;
    cout << "Cartridge MD5:  "
        << theConsole->properties().get("Cartridge.MD5");
    cout << endl << endl;
  }

  // Cleanup time ...
  cleanup();
  return 0;
}
