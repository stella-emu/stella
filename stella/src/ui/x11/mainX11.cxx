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
// Copyright (c) 1995-1999 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: mainX11.cxx,v 1.14 2002-03-17 19:37:00 stephena Exp $
//============================================================================

#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include "bspf.hxx"
#include "Console.hxx"
#include "Event.hxx"
#include "MediaSrc.hxx"
#include "PropsSet.hxx"
#include "SndUnix.hxx"
#include "System.hxx"

#ifdef HAVE_PNG
  #include "Snapshot.hxx"

  static Snapshot* snapshot;

  // The path to save snapshot files
  string theSnapShotDir = "";

  // What the snapshot should be called (romname or md5sum)
  string theSnapShotName = "";

  // Indicates whether to generate multiple snapshots or keep
  // overwriting the same file.  Set to true by default.
  bool theMultipleSnapShotFlag = true;
#endif

#ifdef LINUX_JOYSTICK
  #include <unistd.h>
  #include <fcntl.h>
  #include <linux/joystick.h>

  // File descriptors for the joystick devices
  int theLeftJoystickFd;
  int theRightJoystickFd;
#endif

#define HAVE_GETTIMEOFDAY

// Globals for X windows stuff
Display* theDisplay;
string theDisplayName = "";
int theScreen;
Visual* theVisual;
Window theWindow;
Colormap thePrivateColormap;
Cursor normalCursor;
Cursor blankCursor;
uInt32 eventMask;
Atom wm_delete_window;

// A graphic context for each of the 2600's colors
GC theGCTable[256];

// function prototypes
bool setupDisplay();
bool setupJoystick();
bool createCursors();
void cleanup();

void updateDisplay(MediaSource& mediaSource);
void handleEvents();

void doQuit();
void resizeWindow(int mode);
void centerWindow();
void showCursor(bool show);
void grabMouse(bool grab);
void toggleFullscreen();
void takeSnapshot();
void togglePause();
uInt32 maxWindowSizeForScreen();
uInt32 getTicks();

bool setupProperties(PropertiesSet& set);
void handleCommandLineArguments(int argc, char* argv[]);
void handleRCFile();
void parseRCOptions(istream& in);
void usage();


// Global event stuff
struct Switches
{
  KeySym scanCode;
  Event::Type eventCode;
};

static Switches list[] = {
  { XK_1,           Event::KeyboardZero1 },
  { XK_2,           Event::KeyboardZero2 },
  { XK_3,           Event::KeyboardZero3 },
  { XK_q,           Event::KeyboardZero4 },
  { XK_w,           Event::KeyboardZero5 },
  { XK_e,           Event::KeyboardZero6 },
  { XK_a,           Event::KeyboardZero7 },
  { XK_s,           Event::KeyboardZero8 },
  { XK_d,           Event::KeyboardZero9 },
  { XK_z,           Event::KeyboardZeroStar },
  { XK_x,           Event::KeyboardZero0 },
  { XK_c,           Event::KeyboardZeroPound },

  { XK_8,           Event::KeyboardOne1 },
  { XK_9,           Event::KeyboardOne2 },
  { XK_0,           Event::KeyboardOne3 },
  { XK_i,           Event::KeyboardOne4 },
  { XK_o,           Event::KeyboardOne5 },
  { XK_p,           Event::KeyboardOne6 },
  { XK_k,           Event::KeyboardOne7 },
  { XK_l,           Event::KeyboardOne8 },
  { XK_semicolon,   Event::KeyboardOne9 },
  { XK_comma,       Event::KeyboardOneStar },
  { XK_period,      Event::KeyboardOne0 },
  { XK_slash,       Event::KeyboardOnePound },

  { XK_Down,        Event::JoystickZeroDown },
  { XK_Up,          Event::JoystickZeroUp },
  { XK_Left,        Event::JoystickZeroLeft },
  { XK_Right,       Event::JoystickZeroRight },
  { XK_space,       Event::JoystickZeroFire }, 
  { XK_Return,      Event::JoystickZeroFire }, 
  { XK_Control_L,   Event::JoystickZeroFire }, 
  { XK_z,           Event::BoosterGripZeroTrigger },
  { XK_x,           Event::BoosterGripZeroBooster },

  { XK_w,           Event::JoystickZeroUp },
  { XK_s,           Event::JoystickZeroDown },
  { XK_a,           Event::JoystickZeroLeft },
  { XK_d,           Event::JoystickZeroRight },
  { XK_Tab,         Event::JoystickZeroFire }, 
  { XK_1,           Event::BoosterGripZeroTrigger },
  { XK_2,           Event::BoosterGripZeroBooster },

  { XK_l,           Event::JoystickOneDown },
  { XK_o,           Event::JoystickOneUp },
  { XK_k,           Event::JoystickOneLeft },
  { XK_semicolon,   Event::JoystickOneRight },
  { XK_j,           Event::JoystickOneFire }, 
  { XK_n,           Event::BoosterGripOneTrigger },
  { XK_m,           Event::BoosterGripOneBooster },

  { XK_F1,          Event::ConsoleSelect },
  { XK_F2,          Event::ConsoleReset },
  { XK_F3,          Event::ConsoleColor },
  { XK_F4,          Event::ConsoleBlackWhite },
  { XK_F5,          Event::ConsoleLeftDifficultyA },
  { XK_F6,          Event::ConsoleLeftDifficultyB },
  { XK_F7,          Event::ConsoleRightDifficultyA },
  { XK_F8,          Event::ConsoleRightDifficultyB }
};

static Event keyboardEvent;

// Default window size of 0, meaning it must be set somewhere else
uInt32 theWindowSize = 0;

// Indicates the maximum window size for the current screen
uInt32 theMaxWindowSize;

// Indicates the width and height of the game display based on properties
uInt32 theHeight;
uInt32 theWidth;

// Pointer to the console object or the null pointer
Console* theConsole;

// Event object to use
Event theEvent;

// Indicates if the user wants to quit
bool theQuitIndicator = false;

// Indicates if the emulator should be paused
bool thePauseIndicator = false;

// Indicates if the entire frame should be redrawn
bool theRedrawEntireFrameFlag = true;

// Indicates whether to use fullscreen
bool theUseFullScreenFlag = false;

// Indicates whether mouse can leave the game window
bool theGrabMouseFlag = false;

// Indicates whether to center the game window
bool theCenterWindowFlag = false;

// Indicates whether to show some game info on program exit
bool theShowInfoFlag = false;

// Indicates whether to show cursor in the game window
bool theHideCursorFlag = false;

// Indicates whether to allocate colors from a private color map
bool theUsePrivateColormapFlag = false;

// Indicates whether the game is currently in fullscreen
bool isFullscreen = false;

// Indicates whether the window is currently centered
bool isCentered = false;

// Indicates what the desired volume is
uInt32 theDesiredVolume = 75;

// Indicates what the desired frame rate is
uInt32 theDesiredFrameRate = 60;

// Indicate which paddle mode we're using:
//   0 - Mouse emulates paddle 0
//   1 - Mouse emulates paddle 1
//   2 - Mouse emulates paddle 2
//   3 - Mouse emulates paddle 3
//   4 - Use real Atari 2600 paddles
uInt32 thePaddleMode = 0;

// An alternate properties file to use
string theAlternateProFile = "";

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
  theWidth = theConsole->mediaSource().width();
  theHeight = theConsole->mediaSource().height();

  // Get the maximum size of a window for THIS screen
  // Must be called after display and screen are known, as well as
  // theWidth and theHeight
  theMaxWindowSize = maxWindowSizeForScreen();

  // If theWindowSize is not 0, then it must have been set on the commandline
  // Now we check to see if it is within bounds
  if(theWindowSize != 0)
  {
    if(theWindowSize < 1)
      theWindowSize = 1;
    else if(theWindowSize > theMaxWindowSize)
      theWindowSize = theMaxWindowSize;
  }
  else  // theWindowSize hasn't been set so we do the default
  {
    if(theMaxWindowSize < 2)
      theWindowSize = 1;
    else
      theWindowSize = 2;
  }

  // Figure out the desired size of the window
  int width = theWidth * 2 * theWindowSize;
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

  // If requested create a private colormap for the window
  if(theUsePrivateColormapFlag)
  {
    thePrivateColormap = XCreateColormap(theDisplay, theWindow, 
        theVisual, AllocNone);
  }

  XSizeHints hints;
  hints.flags = PSize | PMinSize | PMaxSize;
  hints.min_width = hints.max_width = hints.width = width;
  hints.min_height = hints.max_height = hints.height = height;

  // Set window and icon name, size hints and other properties 
  char name[512];
  sprintf(name, "Stella: \"%s\"", 
      theConsole->properties().get("Cartridge.Name").c_str());
  XmbSetWMProperties(theDisplay, theWindow, name, "stella", 0, 0, &hints, None, None);

  // Allocate colors in the default colormap
  const uInt32* palette = theConsole->mediaSource().palette();
  for(uInt32 t = 0; t < 256; t += 2)
  {
    XColor color;

    color.red = (palette[t] & 0x00ff0000) >> 8 ;
    color.green = (palette[t] & 0x0000ff00) ;
    color.blue = (palette[t] & 0x000000ff) << 8;
    color.flags = DoRed | DoGreen | DoBlue;

    if(theUsePrivateColormapFlag)
      XAllocColor(theDisplay, thePrivateColormap, &color);
    else
      XAllocColor(theDisplay, DefaultColormap(theDisplay, theScreen), &color);

    XGCValues values;
    values.foreground = color.pixel;
    theGCTable[t] = XCreateGC(theDisplay, theWindow, GCForeground, &values);
    theGCTable[t + 1] = theGCTable[t];
  }

  // Set up the delete window stuff ...
  wm_delete_window = XInternAtom(theDisplay, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(theDisplay, theWindow, &wm_delete_window, 1);

  // If requested install a private colormap for the window
  if(theUsePrivateColormapFlag)
  {
    XSetWindowColormap(theDisplay, theWindow, thePrivateColormap);
  }

  XSelectInput(theDisplay, theWindow, ExposureMask);
  XMapWindow(theDisplay, theWindow);

  // Center the window if centering is selected and not fullscreen
  if(theCenterWindowFlag)// && !theUseFullScreenFlag)
    centerWindow();

  XEvent event;
  do
  {
    XNextEvent(theDisplay, &event);
  } while (event.type != Expose);

  eventMask = ExposureMask | KeyPressMask | KeyReleaseMask | PropertyChangeMask |
              StructureNotifyMask;

  // If we're using the mouse for paddle emulation then enable mouse events
  if(((theConsole->properties().get("Controller.Left") == "Paddles") ||
      (theConsole->properties().get("Controller.Right") == "Paddles"))
    && (thePaddleMode != 4)) 
  {
    eventMask |= (PointerMotionMask | ButtonPressMask | ButtonReleaseMask);
  }

  // Keep mouse in game window if grabmouse is selected
  grabMouse(theGrabMouseFlag);

  // Show or hide the cursor depending on the 'hidecursor' argument
  showCursor(!theHideCursorFlag);

  XSelectInput(theDisplay, theWindow, eventMask);

#ifdef HAVE_PNG
  // Take care of the snapshot stuff.
  snapshot = new Snapshot();

  if(theSnapShotDir == "")
    theSnapShotDir = getenv("HOME");
  if(theSnapShotName == "")
    theSnapShotName = "romname";
#endif

  return true;
}

/**
  This routine should be called once setupDisplay is called
  to create the joystick stuff
*/
bool setupJoystick()
{
#ifdef LINUX_JOYSTICK
  // Open the joystick devices
  theLeftJoystickFd = open("/dev/js0", O_RDONLY | O_NONBLOCK);
  theRightJoystickFd = open("/dev/js1", O_RDONLY | O_NONBLOCK);
#endif

  return true;
}

/**
  This routine should be called anytime the display needs to be updated
*/
void updateDisplay(MediaSource& mediaSource)
{
  uInt8* currentFrame = mediaSource.currentFrameBuffer();
  uInt8* previousFrame = mediaSource.previousFrameBuffer();
  uInt16 screenMultiple = (uInt16)theWindowSize;

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
  for(uInt16 y = 0; y < theHeight; ++y)
  {
    // Indicates the number of current rectangles
    uInt16 currentCount = 0;

    // Look at four pixels at a time to see if anything has changed
    uInt32* current = (uInt32*)(currentFrame); 
    uInt32* previous = (uInt32*)(previousFrame);

    for(uInt16 x = 0; x < theWidth; x += 4, ++current, ++previous)
    {
      // Has something changed in this set of four pixels?
      if((*current != *previous) || theRedrawEntireFrameFlag)
      {
        uInt8* c = (uInt8*)current;
        uInt8* p = (uInt8*)previous;

        // Look at each of the bytes that make up the uInt32
        for(uInt16 i = 0; i < 4; ++i, ++c, ++p)
        {
          // See if this pixel has changed
          if((*c != *p) || theRedrawEntireFrameFlag)
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
 
    currentFrame += theWidth;
    previousFrame += theWidth;
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
  theRedrawEntireFrameFlag = false;
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
    if(event.xclient.data.l[0] == wm_delete_window)
    {
      doQuit();
      return;
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
      if((key == XK_Escape) && (event.type == KeyPress))
      {
        doQuit();
      }
      else if((key == XK_equal) && (event.type == KeyPress))
      {
        resizeWindow(1);
      }
      else if((key == XK_minus) && (event.type == KeyPress))
      {
        resizeWindow(0);
      }
      else if((key == XK_F12) && (event.type == KeyPress))
      {
        takeSnapshot();
      }
      else if((key == XK_Pause) && (event.type == KeyPress))
      {
        togglePause();
      }
      else if((key == XK_g) && (event.type == KeyPress))
      {
        // don't change grabmouse in fullscreen mode
        if(!isFullscreen)
        {
          theGrabMouseFlag = !theGrabMouseFlag;
          grabMouse(theGrabMouseFlag);
        }
      }
      else if((key == XK_h) && (event.type == KeyPress))
      {
        // don't change hidecursor in fullscreen mode
        if(!isFullscreen)
        {
          theHideCursorFlag = !theHideCursorFlag;
          showCursor(!theHideCursorFlag);
        }
      }
      else
      { 
        for(unsigned int i = 0; i < sizeof(list) / sizeof(Switches); ++i)
        { 
          if(list[i].scanCode == key)
          {
            theEvent.set(list[i].eventCode, 
                (event.type == KeyPress) ? 1 : 0);
            keyboardEvent.set(list[i].eventCode, 
                (event.type == KeyPress) ? 1 : 0);
          }
        }
      }
    }
    else if(event.type == MotionNotify)
    {
      Int32 resistance = 0;
      uInt32 width = theWidth * 2 * theWindowSize;

      int x = width - event.xmotion.x;
      resistance = (Int32)((1000000.0 * x) / width);

      // Now, set the event of the correct paddle to the calculated resistance
      if(thePaddleMode == 0)
        theEvent.set(Event::PaddleZeroResistance, resistance);
      else if(thePaddleMode == 1)
        theEvent.set(Event::PaddleOneResistance, resistance);
      else if(thePaddleMode == 2)
        theEvent.set(Event::PaddleTwoResistance, resistance);
      else if(thePaddleMode == 3)
        theEvent.set(Event::PaddleThreeResistance, resistance);
    }
    else if(event.type == ButtonPress) 
    {
      if(thePaddleMode == 0)
        theEvent.set(Event::PaddleZeroFire, 1);
      else if(thePaddleMode == 1)
        theEvent.set(Event::PaddleOneFire, 1);
      else if(thePaddleMode == 2)
        theEvent.set(Event::PaddleTwoFire, 1);
      else if(thePaddleMode == 3)
        theEvent.set(Event::PaddleThreeFire, 1);
    }
    else if(event.type == ButtonRelease)
    {
      if(thePaddleMode == 0)
        theEvent.set(Event::PaddleZeroFire, 0);
      else if(thePaddleMode == 1)
        theEvent.set(Event::PaddleOneFire, 0);
      else if(thePaddleMode == 2)
        theEvent.set(Event::PaddleTwoFire, 0);
      else if(thePaddleMode == 3)
        theEvent.set(Event::PaddleThreeFire, 0);
    }
    else if(event.type == Expose)
    {
      theRedrawEntireFrameFlag = true;
    }
    else if(event.type == UnmapNotify)
    {
      if(!thePauseIndicator)
      {
        togglePause();
      }
    }
  }

#ifdef LINUX_JOYSTICK
  // Read joystick events and modify event states
  if(theLeftJoystickFd >= 0)
  {
    struct js_event event;

    // Process each joystick event that's queued-up
    while(read(theLeftJoystickFd, &event, sizeof(struct js_event)) > 0)
    {
      if((event.type & ~JS_EVENT_INIT) == JS_EVENT_BUTTON)
      {
        if(event.number == 0)
        {
          theEvent.set(Event::JoystickZeroFire, event.value ? 
              1 : keyboardEvent.get(Event::JoystickZeroFire));

          // If we're using real paddles then set paddle event as well
          if(thePaddleMode == 4)
            theEvent.set(Event::PaddleZeroFire, event.value);
        }
        else if(event.number == 1)
        {
          theEvent.set(Event::BoosterGripZeroTrigger, event.value ? 
              1 : keyboardEvent.get(Event::BoosterGripZeroTrigger));

          // If we're using real paddles then set paddle event as well
          if(thePaddleMode == 4)
            theEvent.set(Event::PaddleOneFire, event.value);
        }
      }
      else if((event.type & ~JS_EVENT_INIT) == JS_EVENT_AXIS)
      {
        if(event.number == 0)
        {
          theEvent.set(Event::JoystickZeroLeft, (event.value < -16384) ? 
              1 : keyboardEvent.get(Event::JoystickZeroLeft));
          theEvent.set(Event::JoystickZeroRight, (event.value > 16384) ? 
              1 : keyboardEvent.get(Event::JoystickZeroRight));

          // If we're using real paddles then set paddle events as well
          if(thePaddleMode == 4)
          {
            uInt32 r = (uInt32)((1.0E6L * (event.value + 32767L)) / 65536);
            theEvent.set(Event::PaddleZeroResistance, r);
          }
        }
        else if(event.number == 1)
        {
          theEvent.set(Event::JoystickZeroUp, (event.value < -16384) ? 
              1 : keyboardEvent.get(Event::JoystickZeroUp));
          theEvent.set(Event::JoystickZeroDown, (event.value > 16384) ? 
              1 : keyboardEvent.get(Event::JoystickZeroDown));

          // If we're using real paddles then set paddle events as well
          if(thePaddleMode == 4)
          {
            uInt32 r = (uInt32)((1.0E6L * (event.value + 32767L)) / 65536);
            theEvent.set(Event::PaddleOneResistance, r);
          }
        }
      }
    }
  }

  if(theRightJoystickFd >= 0)
  {
    struct js_event event;

    // Process each joystick event that's queued-up
    while(read(theRightJoystickFd, &event, sizeof(struct js_event)) > 0)
    {
      if((event.type & ~JS_EVENT_INIT) == JS_EVENT_BUTTON)
      {
        if(event.number == 0)
        {
          theEvent.set(Event::JoystickOneFire, event.value ? 
              1 : keyboardEvent.get(Event::JoystickOneFire));

          // If we're using real paddles then set paddle event as well
          if(thePaddleMode == 4)
            theEvent.set(Event::PaddleTwoFire, event.value);
        }
        else if(event.number == 1)
        {
          theEvent.set(Event::BoosterGripOneTrigger, event.value ? 
              1 : keyboardEvent.get(Event::BoosterGripOneTrigger));

          // If we're using real paddles then set paddle event as well
          if(thePaddleMode == 4)
            theEvent.set(Event::PaddleThreeFire, event.value);
        }
      }
      else if((event.type & ~JS_EVENT_INIT) == JS_EVENT_AXIS)
      {
        if(event.number == 0)
        {
          theEvent.set(Event::JoystickOneLeft, (event.value < -16384) ? 
              1 : keyboardEvent.get(Event::JoystickOneLeft));
          theEvent.set(Event::JoystickOneRight, (event.value > 16384) ? 
              1 : keyboardEvent.get(Event::JoystickOneRight));

          // If we're using real paddles then set paddle events as well
          if(thePaddleMode == 4)
          {
            uInt32 r = (uInt32)((1.0E6L * (event.value + 32767L)) / 65536);
            theEvent.set(Event::PaddleTwoResistance, r);
          }
        }
        else if(event.number == 1)
        {
          theEvent.set(Event::JoystickOneUp, (event.value < -16384) ? 
              1 : keyboardEvent.get(Event::JoystickOneUp));
          theEvent.set(Event::JoystickOneDown, (event.value > 16384) ? 
              1 : keyboardEvent.get(Event::JoystickOneDown));

          // If we're using real paddles then set paddle events as well
          if(thePaddleMode == 4)
          {
            uInt32 r = (uInt32)((1.0E6L * (event.value + 32767L)) / 65536);
            theEvent.set(Event::PaddleThreeResistance, r);
          }
        }
      }
    }
  }
#endif
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
  indicates that the windows should decrease in size.
  Can't resize in fullscreen mode.  Will only resize up to the maximum size
  for the '-zoom' argument.
*/
void resizeWindow(int mode)
{
  if(isFullscreen)
    return;

  if(mode == 1)   // increase size
  {
    if(theWindowSize == theMaxWindowSize)
      theWindowSize = 1;
    else
      theWindowSize++;
  }
  else   // decrease size
  {
    if(theWindowSize == 1)
      theWindowSize = theMaxWindowSize;
    else
      theWindowSize--;
  }

  // Figure out the desired size of the window
  int width = theWidth * 2 * theWindowSize;
  int height = theHeight * theWindowSize;

  XWindowChanges wc;
  wc.width = width;
  wc.height = height;
  XConfigureWindow(theDisplay, theWindow, CWWidth | CWHeight, &wc);

  XSizeHints hints;
  hints.flags = PSize | PMinSize | PMaxSize;
  hints.min_width = hints.max_width = hints.width = width;
  hints.min_height = hints.max_height = hints.height = height;
  XSetWMNormalHints(theDisplay, theWindow, &hints);

  theRedrawEntireFrameFlag = true;

  // A resize probably means that the window is no longer centered
  isCentered = false;

  if(theCenterWindowFlag)
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
  x = (w - (theWidth * 2 * theWindowSize)) / 2;
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

  theConsole->mediaSource().pause(thePauseIndicator);
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
  if(!snapshot)
  {
    cerr << "Snapshot support disabled.\n";
    return;
  }

  // Now find the correct name for the snapshot
  string filename = theSnapShotDir;
  if(theSnapShotName == "romname")
    filename = filename + "/" + theConsole->properties().get("Cartridge.Name");
  else if(theSnapShotName == "md5sum")
    filename = filename + "/" + theConsole->properties().get("Cartridge.MD5");
  else
  {
    cerr << "ERROR: unknown name " << theSnapShotName
         << " for snapshot type" << endl;
    return;
  }

  // Replace all spaces in name with underscores
  replace(filename.begin(), filename.end(), ' ', '_');

  // Check whether we want multiple snapshots created
  if(theMultipleSnapShotFlag)
  {
    // Determine if the file already exists, checking each successive filename
    // until one doesn't exist
    string extFilename = filename + ".png";
    if(access(extFilename.c_str(), F_OK) == 0 )
    {
      uInt32 i;
      char buffer[1024];

      for(i = 1; ;++i)
      {
        snprintf(buffer, 1023, "%s_%d.png", filename.c_str(), i);
        if(access(buffer, F_OK) == -1 )
          break;
      }
      filename = buffer;
    }
    else
      filename = extFilename;
  }
  else
    filename = filename + ".png";

  // Now save the snapshot file
  snapshot->savePNG(filename, theConsole->mediaSource(), theWindowSize);

  if(access(filename.c_str(), F_OK) == 0)
    cerr << "Snapshot saved as " << filename << endl;
  else
    cerr << "Couldn't create snapshot " << filename << endl;
#else
  cerr << "Snapshot mode not supported.\n";
#endif
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
    int width = theWidth * 2 * multiplier;
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
  Display a usage message and exit the program
*/
void usage()
{
  static const char* message[] = {
    "",
    "X Stella version 1.2",
    "",
    "Usage: stella.x11 [option ...] file",
    "",
    "Valid options are:",
    "",
    "  -display <display>      Connect to the designated X display",
    "  -fps <number>           Display the given number of frames per second",
    "  -owncmap                Install a private colormap",
    "  -zoom <size>            Makes window be 'size' times normal (1 - 4)",
//    "  -fullscreen             Play the game in fullscreen mode",
    "  -grabmouse              Keeps the mouse in the game window",
    "  -hidecursor             Hides the mouse cursor in the game window",
    "  -center                 Centers the game window onscreen",
    "  -volume <number>        Set the volume (0 - 100)",
#ifdef LINUX_JOYSTICK
    "  -paddle <0|1|2|3|real>  Indicates which paddle the mouse should emulate",
    "                          or that real Atari 2600 paddles are being used",
#else
    "  -paddle <0|1|2|3>       Indicates which paddle the mouse should emulate",
#endif
    "  -showinfo               Shows some game info on exit",
#ifdef HAVE_PNG
    "  -ssdir <path>           The directory to save snapshot files to",
    "  -ssname <name>          How to name the snapshot (romname or md5sum)",
    "  -sssingle               Generate single snapshot instead of many",
#endif
    "  -pro <props file>       Use the given properties file instead of stella.pro",
    "",
    0
  };

  for(uInt32 i = 0; message[i] != 0; ++i)
  {
    cerr << message[i] << endl;
  }
  exit(1);
}

/**
  Setup the properties set by first checking for a user file ".stella.pro",
  then a system-wide file "/etc/stella.pro".  Return false if neither file
  is found, else return true.

  @param set The properties set to setup
*/
bool setupProperties(PropertiesSet& set)
{
  string homePropertiesFile = getenv("HOME");
  homePropertiesFile += "/.stella.pro";
  string systemPropertiesFile = "/etc/stella.pro";

  // Check to see if the user has specified an alternate .pro file.
  // If it exists, use it.
  if(theAlternateProFile != "")
  {
    if(access(theAlternateProFile.c_str(), R_OK) == 0)
    {
      set.load(theAlternateProFile, &Console::defaultProperties(), false);
      return true;
    }
    else
    {
      cerr << "ERROR: Couldn't find \"" << theAlternateProFile <<
              "\" properties file." << endl;
      return false;
    }
  }

  if(access(homePropertiesFile.c_str(), R_OK) == 0)
  {
    set.load(homePropertiesFile, &Console::defaultProperties(), false);
    return true;
  }
  else if(access(systemPropertiesFile.c_str(), R_OK) == 0)
  {
    set.load(systemPropertiesFile, &Console::defaultProperties(), false);
    return true;
  }
  else
  {
    cerr << "ERROR: Couldn't find stella.pro file." << endl;
    return false;
  }
}

/**
  Should be called to parse the command line arguments

  @param argc The count of command line arguments
  @param argv The command line arguments
*/
void handleCommandLineArguments(int argc, char* argv[])
{
  // Make sure we have the correct number of command line arguments
  if(argc < 2)
    usage();

  for(Int32 i = 1; i < (argc - 1); ++i)
  {
    // See which command line switch they're using
    if(string(argv[i]) == "-fps")
    {
      // They're setting the desired frame rate
      Int32 rate = atoi(argv[++i]);
      if((rate < 1) || (rate > 300))
      {
        rate = 60;
      }

      theDesiredFrameRate = rate;
    }
    else if(string(argv[i]) == "-paddle")
    {
      // They're trying to set the paddle emulation mode
      if(string(argv[i + 1]) == "real")
      {
        thePaddleMode = 4;
      }
      else
      {
        thePaddleMode = atoi(argv[i + 1]);
        if((thePaddleMode < 0) || (thePaddleMode > 3))
        {
          usage();
        }
      }
      ++i;
    }
    else if(string(argv[i]) == "-owncmap")
    {
      theUsePrivateColormapFlag = true;
    }
    else if(string(argv[i]) == "-display")
    {
      theDisplayName = argv[++i];
    }
    else if(string(argv[i]) == "-fullscreen")
    {
      theUseFullScreenFlag = true;
    }
    else if(string(argv[i]) == "-grabmouse")
    {
      theGrabMouseFlag = true;
    }
    else if(string(argv[i]) == "-hidecursor")
    {
      theHideCursorFlag = true;
    }
    else if(string(argv[i]) == "-center")
    {
      theCenterWindowFlag = true;
    }
    else if(string(argv[i]) == "-showinfo")
    {
      theShowInfoFlag = true;
    }
    else if(string(argv[i]) == "-zoom")
    {
      uInt32 size = atoi(argv[++i]);
      theWindowSize = size;
    }
    else if(string(argv[i]) == "-volume")
    {
      // They're setting the desired volume
      Int32 volume = atoi(argv[++i]);
      if(volume < 0)
        volume = 0;
      if(volume > 100)
        volume = 100;

      theDesiredVolume = volume;
    }
#ifdef HAVE_PNG
    else if(string(argv[i]) == "-ssdir")
    {
      theSnapShotDir = argv[++i];
    }
    else if(string(argv[i]) == "-ssname")
    {
      theSnapShotName = argv[++i];
    }
    else if(string(argv[i]) == "-sssingle")
    {
      theMultipleSnapShotFlag = false;
    }
#endif
    else if(string(argv[i]) == "-pro")
    {
      theAlternateProFile = argv[++i];
    }
    else
    {
      cout << "Undefined option " << argv[i] << endl;
    }
  }
}

/**
  Should be called to determine if an rc file exists.  First checks if there
  is a user specified file ".stellarc" and then if there is a system-wide
  file "/etc/stellarc".
*/
void handleRCFile()
{
  string homeRCFile = getenv("HOME");
  homeRCFile += "/.stellarc";

  if(access(homeRCFile.c_str(), R_OK) == 0 )
  {
    ifstream homeStream(homeRCFile.c_str());
    parseRCOptions(homeStream);
  }
  else if(access("/etc/stellarc", R_OK) == 0 )
  {
    ifstream systemStream("/etc/stellarc");
    parseRCOptions(systemStream);
  }
}

/**
  Parses each line of the given rcfile and sets options accordingly.

  @param in The file to parse for options
*/
void parseRCOptions(istream& in)
{
  string line, key, value;
  uInt32 equalPos;

  while(getline(in, line))
  {
    // Strip all whitespace and tabs from the line
    uInt32 garbage;
    while((garbage = line.find(" ")) != string::npos)
      line.erase(garbage, 1);
    while((garbage = line.find("\t")) != string::npos)
      line.erase(garbage, 1);

    // Ignore commented and empty lines
    if((line.length() == 0) || (line[0] == ';'))
      continue;

    // Search for the equal sign and discard the line if its not found
    if((equalPos = line.find("=")) == string::npos)
      continue;

    key   = line.substr(0, equalPos);
    value = line.substr(equalPos + 1, line.length() - key.length() - 1);

    // Check for absent key or value
    if((key.length() == 0) || (value.length() == 0))
      continue;

    // Now set up the options by key
    if(key == "fps")
    {
      // They're setting the desired frame rate
      uInt32 rate = atoi(value.c_str());
      if((rate < 1) || (rate > 300))
      {
        rate = 60;
      }

      theDesiredFrameRate = rate;
    }
    else if(key == "paddle")
    {
      // They're trying to set the paddle emulation mode
      uInt32 pMode;
      if(value == "real")
      {
        thePaddleMode = 4;
      }
      else
      {
        pMode = atoi(value.c_str());
        if((pMode > 0) && (pMode < 4))
          thePaddleMode = pMode;
      }
    }
    else if(key == "owncmap")
    {
      uInt32 option = atoi(value.c_str());
      if(option == 1)
        theUsePrivateColormapFlag = true;
      else if(option == 0)
        theUsePrivateColormapFlag = false;
    }
    else if(key == "display")
    {
      theDisplayName = value;
    }
    else if(key == "fullscreen")
    {
      uInt32 option = atoi(value.c_str());
      if(option == 1)
        theUseFullScreenFlag = true;
      else if(option == 0)
        theUseFullScreenFlag = false;
    }
    else if(key == "grabmouse")
    {
      uInt32 option = atoi(value.c_str());
      if(option == 1)
        theGrabMouseFlag = true;
      else if(option == 0)
        theGrabMouseFlag = false;
    }
    else if(key == "hidecursor")
    {
      uInt32 option = atoi(value.c_str());
      if(option == 1)
        theHideCursorFlag = true;
      else if(option == 0)
        theHideCursorFlag = false;
    }
    else if(key == "center")
    {
      uInt32 option = atoi(value.c_str());
      if(option == 1)
        theCenterWindowFlag = true;
      else if(option == 0)
        theCenterWindowFlag = false;
    }
    else if(key == "showinfo")
    {
      uInt32 option = atoi(value.c_str());
      if(option == 1)
        theShowInfoFlag = true;
      else if(option == 0)
        theShowInfoFlag = false;
    }
    else if(key == "zoom")
    {
      // They're setting the initial window size
      // Don't do bounds checking here, it will be taken care of later
      uInt32 size = atoi(value.c_str());
      theWindowSize = size;
    }
    else if(key == "volume")
    {
      // They're setting the desired volume
      uInt32 volume = atoi(value.c_str());
      if(volume < 0)
        volume = 0;
      if(volume > 100)
        volume = 100;

      theDesiredVolume = volume;
    }
#ifdef HAVE_PNG
    else if(key == "ssdir")
    {
      theSnapShotDir = value;
    }
    else if(key == "ssname")
    {
      theSnapShotName = value;
    }
    else if(key == "sssingle")
    {
      uInt32 option = atoi(value.c_str());
      if(option == 1)
        theMultipleSnapShotFlag = false;
      else if(option == 0)
        theMultipleSnapShotFlag = true;
    }
#endif
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

  if(normalCursor)
    XFreeCursor(theDisplay, normalCursor);
  if(blankCursor)
    XFreeCursor(theDisplay, blankCursor);

  // If we're using a private colormap then let's free it to be safe
  if(theUsePrivateColormapFlag && theDisplay)
  {
     XFreeColormap(theDisplay, thePrivateColormap);
  }

#ifdef LINUX_JOYSTICK
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
  // Load in any user defined settings from an RC file
  handleRCFile();

  // Handle the command line arguments
  handleCommandLineArguments(argc, argv);

  // Get a pointer to the file which contains the cartridge ROM
  const char* file = argv[argc - 1];

  // Open the cartridge image and read it in
  ifstream in(file); 
  if(!in)
  {
    cerr << "ERROR: Couldn't open " << file << "..." << endl;
    exit(1);
  }

  uInt8* image = new uInt8[512 * 1024];
  in.read(image, 512 * 1024);
  uInt32 size = in.gcount();
  in.close();

  // Create a properties set for us to use and set it up
  PropertiesSet propertiesSet;
  if(!setupProperties(propertiesSet))
  {
    delete[] image;
    exit(1);
  }

  // Create a sound object for use with the console
  SoundUnix sound(theDesiredVolume);

  // Get just the filename of the file containing the ROM image
  const char* filename = (!strrchr(file, '/')) ? file : strrchr(file, '/') + 1;

  // Create the 2600 game console
  theConsole = new Console(image, size, filename, 
      theEvent, propertiesSet, sound);

  // Free the image since we don't need it any longer
  delete[] image;

  // Setup X window and joysticks
  if(!setupDisplay())
  {
    cerr << "ERROR: Couldn't set up display.\n";
    cleanup();
  }
  if(!setupJoystick())
  {
    cerr << "ERROR: Couldn't set up joysticks.\n";
    cleanup();
  }

  // Set up timing stuff
  uInt32 before, delta, frameTime = 0, eventTime = 0;
  uInt32 timePerFrame = 1000000 / theDesiredFrameRate;
  uInt32 numberOfFrames = 0;

  // Main game loop
  for(;;)
  {
    // Exit if the user wants to quit
    if(theQuitIndicator)
    {
      break;
    }

    // Call handleEvents here to see if user pressed pause
    handleEvents();
    if(thePauseIndicator)
    {
      updateDisplay(theConsole->mediaSource());
      usleep(10000);
      continue;
    }

    before = getTicks();
    theConsole->mediaSource().update();
    updateDisplay(theConsole->mediaSource());
    handleEvents();

    // Now, waste time if we need to so that we are at the desired frame rate
    for(;;)
    {
      delta = getTicks() - before;

      if(delta > timePerFrame)
        break;
    }

    frameTime += (getTicks() - before);
    ++numberOfFrames;
  }

  if(theShowInfoFlag)
  {
    double executionTime = (double) frameTime / 1000000.0;
    double framesPerSecond = (double) numberOfFrames / executionTime;

    cout << endl;
    cout << numberOfFrames << " total frames drawn\n";
    cout << framesPerSecond << " frames/second\n";
    cout << theConsole->mediaSource().scanlines() << " scanlines in last frame\n";
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
#error "We need gettimeofday for the X11 version"
#endif
