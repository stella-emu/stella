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
// $Id: mainX11.cxx,v 1.30 2002-11-10 19:43:17 stephena Exp $
//============================================================================

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>

#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

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
#include "System.hxx"
#include "Settings.hxx"
#include "SoundX11.hxx"

#ifdef HAVE_PNG
  #include "Snapshot.hxx"
#endif

#ifdef HAVE_JOYSTICK
  #include <unistd.h>
  #include <fcntl.h>
  #include <linux/joystick.h>
#endif

#define MESSAGE_INTERVAL 2

// A graphic context for each of the 2600's colors
static GC theGCTable[256];

// function prototypes
static bool setupDisplay();
static bool setupJoystick();
static bool createCursors();
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

static void loadState();
static void saveState();
static void changeState(int direction);

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

#ifdef HAVE_PNG
  static Snapshot* snapshot;
#endif

#ifdef HAVE_JOYSTICK
  // File descriptors for the joystick devices
  static int theLeftJoystickFd;
  static int theRightJoystickFd;
#endif

// Global event stuff
struct Switches
{
  KeySym scanCode;
  Event::Type eventCode;
  string message;
};

static Switches list[] = {
  { XK_1,           Event::KeyboardZero1,            "" },
  { XK_2,           Event::KeyboardZero2,            "" },
  { XK_3,           Event::KeyboardZero3,            "" },
  { XK_q,           Event::KeyboardZero4,            "" },
  { XK_w,           Event::KeyboardZero5,            "" },
  { XK_e,           Event::KeyboardZero6,            "" },
  { XK_a,           Event::KeyboardZero7,            "" },
  { XK_s,           Event::KeyboardZero8,            "" },
  { XK_d,           Event::KeyboardZero9,            "" },
  { XK_z,           Event::KeyboardZeroStar,         "" },
  { XK_x,           Event::KeyboardZero0,            "" },
  { XK_c,           Event::KeyboardZeroPound,        "" },

  { XK_8,           Event::KeyboardOne1,            "" },
  { XK_9,           Event::KeyboardOne2,            "" },
  { XK_0,           Event::KeyboardOne3,            "" },
  { XK_i,           Event::KeyboardOne4,            "" },
  { XK_o,           Event::KeyboardOne5,            "" },
  { XK_p,           Event::KeyboardOne6,            "" },
  { XK_k,           Event::KeyboardOne7,            "" },
  { XK_l,           Event::KeyboardOne8,            "" },
  { XK_semicolon,   Event::KeyboardOne9,            "" },
  { XK_comma,       Event::KeyboardOneStar,         "" },
  { XK_period,      Event::KeyboardOne0,            "" },
  { XK_slash,       Event::KeyboardOnePound,        "" },

  { XK_Down,        Event::JoystickZeroDown,        "" },
  { XK_Up,          Event::JoystickZeroUp,          "" },
  { XK_Left,        Event::JoystickZeroLeft,        "" },
  { XK_Right,       Event::JoystickZeroRight,       "" },
  { XK_space,       Event::JoystickZeroFire,        "" },
  { XK_Return,      Event::JoystickZeroFire,        "" },
  { XK_Control_L,   Event::JoystickZeroFire,        "" },
  { XK_z,           Event::BoosterGripZeroTrigger,  "" },
  { XK_x,           Event::BoosterGripZeroBooster,  "" },

  { XK_w,           Event::JoystickZeroUp,          "" },
  { XK_s,           Event::JoystickZeroDown,        "" },
  { XK_a,           Event::JoystickZeroLeft,        "" },
  { XK_d,           Event::JoystickZeroRight,       "" },
  { XK_Tab,         Event::JoystickZeroFire,        "" },
  { XK_1,           Event::BoosterGripZeroTrigger,  "" },
  { XK_2,           Event::BoosterGripZeroBooster,  "" },

  { XK_l,           Event::JoystickOneDown,         "" },
  { XK_o,           Event::JoystickOneUp,           "" },
  { XK_k,           Event::JoystickOneLeft,         "" },
  { XK_semicolon,   Event::JoystickOneRight,        "" },
  { XK_j,           Event::JoystickOneFire,         "" },
  { XK_n,           Event::BoosterGripOneTrigger,   "" },
  { XK_m,           Event::BoosterGripOneBooster,   "" },

  { XK_F1,          Event::ConsoleSelect,           "" },
  { XK_F2,          Event::ConsoleReset,            "" },
  { XK_F3,          Event::ConsoleColor,            "Color Mode" },
  { XK_F4,          Event::ConsoleBlackWhite,       "BW Mode" },
  { XK_F5,          Event::ConsoleLeftDifficultyA,  "Left Difficulty A" },
  { XK_F6,          Event::ConsoleLeftDifficultyB,  "Left Difficulty B" },
  { XK_F7,          Event::ConsoleRightDifficultyA, "Right Difficulty A" },
  { XK_F8,          Event::ConsoleRightDifficultyB, "Right Difficulty B" }
};

// Event objects to use
static Event theEvent;
static Event keyboardEvent;

// Pointer to the console object or the null pointer
static Console* theConsole = (Console*) NULL;

// Pointer to the settings object or the null pointer
static Settings* settings = (Settings*) NULL;

// Indicates if the user wants to quit
static bool theQuitIndicator = false;

// Indicates if the emulator should be paused
static bool thePauseIndicator = false;

// Indicates if the entire frame should be redrawn
static bool theRedrawEntireFrameIndicator = true;

// Indicates whether the game is currently in fullscreen
static bool isFullscreen = false;

// Indicates whether the window is currently centered
static bool isCentered = false;

// Indicates the current state to use for state saving
static uInt32 currentState = 0;

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
  settings->theWidth  = theConsole->mediaSource().width();
  settings->theHeight = theConsole->mediaSource().height();

  // Get the maximum size of a window for THIS screen
  // Must be called after display and screen are known, as well as
  // theWidth and theHeight
  settings->theMaxWindowSize = maxWindowSizeForScreen();

  // If theWindowSize is not 0, then it must have been set on the commandline
  // Now we check to see if it is within bounds
  if(settings->theWindowSize != 0)
  {
    if(settings->theWindowSize < 1)
      settings->theWindowSize = 1;
    else if(settings->theWindowSize > settings->theMaxWindowSize)
      settings->theWindowSize = settings->theMaxWindowSize;
  }
  else  // theWindowSize hasn't been set so we do the default
  {
    if(settings->theMaxWindowSize < 2)
      settings->theWindowSize = 1;
    else
      settings->theWindowSize = 2;
  }

  // Figure out the desired size of the window
  int width  = settings->theWidth  * settings->theWindowSize * 2;
  int height = settings->theHeight * settings->theWindowSize;

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
  if(settings->theUsePrivateColormapFlag)
  {
    XSetWindowColormap(theDisplay, theWindow, thePrivateColormap);
  }

  XSelectInput(theDisplay, theWindow, ExposureMask);
  XMapWindow(theDisplay, theWindow);

  // Center the window if centering is selected and not fullscreen
  if(settings->theCenterWindowFlag)// && !theUseFullScreenFlag)
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
    && (settings->thePaddleMode != 4)) 
  {
    eventMask |= (PointerMotionMask | ButtonPressMask | ButtonReleaseMask);
  }

  // Keep mouse in game window if grabmouse is selected
  grabMouse(settings->theGrabMouseFlag);

  // Show or hide the cursor depending on the 'hidecursor' argument
  showCursor(!settings->theHideCursorFlag);

  XSelectInput(theDisplay, theWindow, eventMask);

#ifdef HAVE_PNG
  // Take care of the snapshot stuff.
  snapshot = new Snapshot();

  if(settings->theSnapShotDir == "")
    settings->theSnapShotDir = getenv("HOME");
  if(settings->theSnapShotName == "")
    settings->theSnapShotName = "romname";
#endif

  return true;
}

/**
  This routine should be called once setupDisplay is called
  to create the joystick stuff
*/
bool setupJoystick()
{
#ifdef HAVE_JOYSTICK
  // Open the joystick devices
  theLeftJoystickFd = open("/dev/js0", O_RDONLY | O_NONBLOCK);
  theRightJoystickFd = open("/dev/js1", O_RDONLY | O_NONBLOCK);
#endif

  return true;
}

/**
  Set up the palette for a screen of any depth.
*/
void setupPalette()
{
  // If we're using a private colormap then let's free it to be safe
  if(settings->theUsePrivateColormapFlag && theDisplay)
  {
    if(thePrivateColormap)
      XFreeColormap(theDisplay, thePrivateColormap);

     thePrivateColormap = XCreateColormap(theDisplay, theWindow, 
       theVisual, AllocNone);
  }

  // Make the palette be 75% as bright if pause is selected
  float shade = 1.0;
  if(thePauseIndicator)
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

    if(settings->theUsePrivateColormapFlag)
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
  uInt16 screenMultiple = (uInt16) settings->theWindowSize;

  uInt32 width  = settings->theWidth;
  uInt32 height = settings->theHeight;

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
      else if((key == XK_F9) && (event.type == KeyPress))
      {
        saveState();
      }
      else if((key == XK_F10) && (event.type == KeyPress))
      {
        if(event.xkey.state & ShiftMask)
          changeState(0);
        else
          changeState(1);
      }
      else if((key == XK_F11) && (event.type == KeyPress))
      {
        loadState();
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
          settings->theGrabMouseFlag = !settings->theGrabMouseFlag;
          grabMouse(settings->theGrabMouseFlag);
        }
      }
      else if((key == XK_h) && (event.type == KeyPress))
      {
        // don't change hidecursor in fullscreen mode
        if(!isFullscreen)
        {
          settings->theHideCursorFlag = !settings->theHideCursorFlag;
          showCursor(!settings->theHideCursorFlag);
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
          string newPropertiesFile = getenv("HOME");
          newPropertiesFile = newPropertiesFile + "/" + \
            theConsole->properties().get("Cartridge.Name") + ".pro";
          theConsole->saveProperties(newPropertiesFile);
        }
      }
#endif
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

            if((event.type == KeyPress) && (list[i].message != ""))
              theConsole->mediaSource().showMessage(list[i].message,
                  MESSAGE_INTERVAL * settings->theDesiredFrameRate);
          }
        }
      }
    }
    else if(event.type == MotionNotify)
    {
      Int32 resistance = 0;
      uInt32 width = settings->theWidth * settings->theWindowSize * 2;

      int x = width - event.xmotion.x;
      resistance = (Int32)((1000000.0 * x) / width);

      // Now, set the event of the correct paddle to the calculated resistance
      if(settings->thePaddleMode == 0)
        theEvent.set(Event::PaddleZeroResistance, resistance);
      else if(settings->thePaddleMode == 1)
        theEvent.set(Event::PaddleOneResistance, resistance);
      else if(settings->thePaddleMode == 2)
        theEvent.set(Event::PaddleTwoResistance, resistance);
      else if(settings->thePaddleMode == 3)
        theEvent.set(Event::PaddleThreeResistance, resistance);
    }
    else if(event.type == ButtonPress) 
    {
      if(settings->thePaddleMode == 0)
        theEvent.set(Event::PaddleZeroFire, 1);
      else if(settings->thePaddleMode == 1)
        theEvent.set(Event::PaddleOneFire, 1);
      else if(settings->thePaddleMode == 2)
        theEvent.set(Event::PaddleTwoFire, 1);
      else if(settings->thePaddleMode == 3)
        theEvent.set(Event::PaddleThreeFire, 1);
    }
    else if(event.type == ButtonRelease)
    {
      if(settings->thePaddleMode == 0)
        theEvent.set(Event::PaddleZeroFire, 0);
      else if(settings->thePaddleMode == 1)
        theEvent.set(Event::PaddleOneFire, 0);
      else if(settings->thePaddleMode == 2)
        theEvent.set(Event::PaddleTwoFire, 0);
      else if(settings->thePaddleMode == 3)
        theEvent.set(Event::PaddleThreeFire, 0);
    }
    else if(event.type == Expose)
    {
      theRedrawEntireFrameIndicator = true;
    }
    else if(event.type == UnmapNotify)
    {
      if(!thePauseIndicator)
      {
        togglePause();
      }
    }
  }

#ifdef HAVE_JOYSTICK
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
          if(settings->thePaddleMode == 4)
            theEvent.set(Event::PaddleZeroFire, event.value);
        }
        else if(event.number == 1)
        {
          theEvent.set(Event::BoosterGripZeroTrigger, event.value ? 
              1 : keyboardEvent.get(Event::BoosterGripZeroTrigger));

          // If we're using real paddles then set paddle event as well
          if(settings->thePaddleMode == 4)
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
          if(settings->thePaddleMode == 4)
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
          if(settings->thePaddleMode == 4)
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
          if(settings->thePaddleMode == 4)
            theEvent.set(Event::PaddleTwoFire, event.value);
        }
        else if(event.number == 1)
        {
          theEvent.set(Event::BoosterGripOneTrigger, event.value ? 
              1 : keyboardEvent.get(Event::BoosterGripOneTrigger));

          // If we're using real paddles then set paddle event as well
          if(settings->thePaddleMode == 4)
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
          if(settings->thePaddleMode == 4)
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
          if(settings->thePaddleMode == 4)
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
    settings->theWidth         = theConsole->mediaSource().width();
    settings->theHeight        = theConsole->mediaSource().height();
    settings->theMaxWindowSize = maxWindowSizeForScreen();
  }
  else if(mode == 1)   // increase size
  {
    if(isFullscreen)
      return;

    if(settings->theWindowSize == settings->theMaxWindowSize)
      settings->theWindowSize = 1;
    else
      settings->theWindowSize++;
  }
  else if(mode == 0)   // decrease size
  {
    if(isFullscreen)
      return;

    if(settings->theWindowSize == 1)
      settings->theWindowSize = settings->theMaxWindowSize;
    else
      settings->theWindowSize--;
  }

  // Figure out the desired size of the window
  int width  = settings->theWidth  * settings->theWindowSize * 2;
  int height = settings->theHeight * settings->theWindowSize;

  XWindowChanges wc;
  wc.width = width;
  wc.height = height;
  XConfigureWindow(theDisplay, theWindow, CWWidth | CWHeight, &wc);

  XSizeHints hints;
  hints.flags = PSize | PMinSize | PMaxSize;
  hints.min_width = hints.max_width = hints.width = width;
  hints.min_height = hints.max_height = hints.height = height;
  XSetWMNormalHints(theDisplay, theWindow, &hints);

  theRedrawEntireFrameIndicator = true;

  // A resize probably means that the window is no longer centered
  isCentered = false;

  if(settings->theCenterWindowFlag)
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
  x = (w - (settings->theWidth * settings->theWindowSize * 2)) / 2;
  y = (h - (settings->theHeight * settings->theWindowSize)) / 2;

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
  Saves state of the current game in the current slot.
*/
void saveState()
{
  ostringstream buf;
  string md5 = theConsole->properties().get("Cartridge.MD5");
  buf << getenv("HOME") << "/.stella/state/" << md5 << ".st" << currentState;
  string filename = buf.str();

  // Do a state save using the System
  int result = theConsole->system().saveState(filename, md5);

  // Print appropriate message
  buf.str("");
  if(result == 1)
    buf << "State " << currentState << " saved";
  else if(result == 2)
    buf << "Error saving state " << currentState;
  else if(result == 3)
    buf << "Invalid state " << currentState << " file";

  string message = buf.str();
  theConsole->mediaSource().showMessage(message, MESSAGE_INTERVAL *
    settings->theDesiredFrameRate);
}

/**
  Changes the current state slot.
*/
void changeState(int direction)
{
  if(direction == 1)   // increase current state slot
  {
    if(currentState == 9)
      currentState = 0;
    else
      ++currentState;
  }
  else   // decrease current state slot
  {
    if(currentState == 0)
      currentState = 9;
    else
      --currentState;
  }

  // Print appropriate message
  ostringstream buf;
  buf << "Changed to slot " << currentState;
  string message = buf.str();
  theConsole->mediaSource().showMessage(message, MESSAGE_INTERVAL *
    settings->theDesiredFrameRate);
}

/**
  Loads state from the current slot for the current game.
*/
void loadState()
{
  ostringstream buf;
  string md5 = theConsole->properties().get("Cartridge.MD5");
  buf << getenv("HOME") << "/.stella/state/" << md5 << ".st" << currentState;
  string filename = buf.str();

  // Do a state load using the System
  int result = theConsole->system().loadState(filename, md5);

  // Print appropriate message
  buf.str("");
  if(result == 1)
    buf << "State " << currentState << " loaded";
  else if(result == 2)
    buf << "Error loading state " << currentState;
  else if(result == 3)
    buf << "Invalid state " << currentState << " file";

  string message = buf.str();
  theConsole->mediaSource().showMessage(message, MESSAGE_INTERVAL *
    settings->theDesiredFrameRate);
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
  string path = settings->theSnapShotDir;
  string filename;

  if(settings->theSnapShotName == "romname")
    path = path + "/" + theConsole->properties().get("Cartridge.Name");
  else if(settings->theSnapShotName == "md5sum")
    path = path + "/" + theConsole->properties().get("Cartridge.MD5");
  else
  {
    cerr << "ERROR: unknown name " << settings->theSnapShotName
         << " for snapshot type" << endl;
    return;
  }

  // Replace all spaces in name with underscores
  replace(path.begin(), path.end(), ' ', '_');

  // Check whether we want multiple snapshots created
  if(settings->theMultipleSnapShotFlag)
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
  snapshot->savePNG(filename, theConsole->mediaSource(), settings->theWindowSize);

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

  uInt32 multiplier = screenWidth / (settings->theWidth * 2);
  bool found = false;

  while(!found && (multiplier > 0))
  {
    // Figure out the desired size of the window
    int width  = settings->theWidth  * multiplier * 2;
    int height = settings->theHeight * multiplier;

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
    "  -fps        <number>       Display the given number of frames per second",
    "  -owncmap    <0|1>          Install a private colormap",
    "  -zoom       <size>         Makes window be 'size' times normal (1 - 4)",
//    "  -fullscreen <0|1>          Play the game in fullscreen mode",
    "  -grabmouse  <0|1>          Keeps the mouse in the game window",
    "  -hidecursor <0|1>          Hides the mouse cursor in the game window",
    "  -center     <0|1>          Centers the game window onscreen",
    "  -volume     <number>       Set the volume (0 - 100)",
#ifdef HAVE_JOYSTICK
    "  -paddle     <0|1|2|3|real> Indicates which paddle the mouse should emulate",
    "                             or that real Atari 2600 paddles are being used",
#else
    "  -paddle     <0|1|2|3>      Indicates which paddle the mouse should emulate",
#endif
    "  -showinfo   <0|1>          Shows some game info on exit",
#ifdef HAVE_PNG
    "  -ssdir      <path>         The directory to save snapshot files to",
    "  -ssname     <name>         How to name the snapshot (romname or md5sum)",
    "  -sssingle   <0|1>          Generate single snapshot instead of many",
#endif
    "  -pro        <props file>   Use the given properties file instead of stella.pro",
    "  -accurate   <0|1>          Accurate game timing (uses more CPU)",
    "",
#ifdef DEVELOPER_SUPPORT
    " DEVELOPER options (see Stella manual for details)",
    "  -Dformat                    Sets \"Display.Format\"",
    "  -Dxstart                    Sets \"Display.XStart\"",
    "  -Dwidth                     Sets \"Display.Width\"",
    "  -Dystart                    Sets \"Display.YStart\"",
    "  -Dheight                    Sets \"Display.Height\"",
#endif
    0
  };

  for(uInt32 i = 0; message[i] != 0; ++i)
  {
    cerr << message[i] << endl;
  }
  exit(1);
}

/**
  Setup the properties set by first checking for a user file
  "$HOME/.stella/stella.pro", then a system-wide file "/etc/stella.pro".
  Return false if neither file is found, else return true.

  @param set The properties set to setup
*/
bool setupProperties(PropertiesSet& set)
{
  string homePropertiesFile = getenv("HOME");
  homePropertiesFile += "/.stella/stella.pro";
  string systemPropertiesFile = "/etc/stella.pro";

  // Check to see if the user has specified an alternate .pro file.
  // If it exists, use it.
  if(settings->theAlternateProFile != "")
  {
    if(access(settings->theAlternateProFile.c_str(), R_OK) == 0)
    {
      set.load(settings->theAlternateProFile, 
          &Console::defaultProperties(), false);
      return true;
    }
    else
    {
      cerr << "ERROR: Couldn't find \"" << settings->theAlternateProFile <<
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
    set.load("", &Console::defaultProperties(), false);
    return true;
  }
}

/**
  Should be called to determine if an rc file exists.  First checks if there
  is a user specified file "$HOME/.stella/stellarc" and then if there is a
  system-wide file "/etc/stellarc".
*/
void handleRCFile()
{
  string homeRCFile = getenv("HOME");
  homeRCFile += "/.stella/stellarc";

  if(access(homeRCFile.c_str(), R_OK) == 0 )
  {
    ifstream homeStream(homeRCFile.c_str());
    settings->handleRCFile(homeStream);
  }
  else if(access("/etc/stellarc", R_OK) == 0 )
  {
    ifstream systemStream("/etc/stellarc");
    settings->handleRCFile(systemStream);
  }
}

/**
  Does general cleanup in case any operation failed (or at end of program).
*/
void cleanup()
{
  if(theConsole)
    delete theConsole;

  if(settings)
    delete settings;

#ifdef HAVE_PNG
  if(snapshot)
    delete snapshot;
#endif

  if(normalCursor)
    XFreeCursor(theDisplay, normalCursor);
  if(blankCursor)
    XFreeCursor(theDisplay, blankCursor);

  // If we're using a private colormap then let's free it to be safe
  if(settings->theUsePrivateColormapFlag && theDisplay)
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

/**
  Creates some directories under $HOME.
  Required directories are $HOME/.stella and $HOME/.stella/state
*/
bool setupDirs()
{
  string path;

  path = getenv("HOME");
  path += "/.stella";

  if(access(path.c_str(), R_OK|W_OK|X_OK) != 0 )
  {
    if(mkdir(path.c_str(), 0777) != 0)
      return false;
  }

  path += "/state";
  if(access(path.c_str(), R_OK|W_OK|X_OK) != 0 )
  {
    if(mkdir(path.c_str(), 0777) != 0)
      return false;
  }

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
  settings = new Settings();
  if(!settings)
  {
    cerr << "ERROR: Couldn't create settings." << endl;
    cleanup();
    return 0;
  }

  // Load in any user defined settings from an RC file
  handleRCFile();

  // Handle the command line arguments
  if(!settings->handleCommandLineArgs(argc, argv))
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
  if(!setupProperties(propertiesSet))
  {
    delete[] image;
    cleanup();
    return 0;
  }

  // Create a sound object for playing audio
  SoundX11 sound;
  sound.setSoundVolume(settings->theDesiredVolume);

  // Get just the filename of the file containing the ROM image
  const char* filename = (!strrchr(file, '/')) ? file : strrchr(file, '/') + 1;

  // Create the 2600 game console for users or developers
#ifdef DEVELOPER_SUPPORT
  theConsole = new Console(image, size, filename, 
      theEvent, propertiesSet, sound.getSampleRate(),
      &settings->userDefinedProperties);
#else
  theConsole = new Console(image, size, filename, 
      theEvent, propertiesSet, sound.getSampleRate());
#endif

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

  if(settings->theAccurateTimingFlag)   // normal, CPU-intensive timing
  {
    // Set up timing stuff
    uInt32 startTime, delta;
    uInt32 timePerFrame = 
        (uInt32)(1000000.0 / (double)settings->theDesiredFrameRate);

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
      handleEvents();
      if(thePauseIndicator)
      {
        updateDisplay(theConsole->mediaSource());
        usleep(10000);
        continue;
      }

      startTime = getTicks();
      theConsole->mediaSource().update();
      sound.updateSound(theConsole->mediaSource());
      updateDisplay(theConsole->mediaSource());
      handleEvents();

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
        (uInt32)(1000000.0 / (double)settings->theDesiredFrameRate);

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
      if(!thePauseIndicator)
      {
        theConsole->mediaSource().update();
      }
      sound.updateSound(theConsole->mediaSource());
      updateDisplay(theConsole->mediaSource());
      handleEvents();

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

  if(settings->theShowInfoFlag)
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

