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
// $Id: mainX11.cxx,v 1.1.1.1 2001-12-27 19:54:36 bwmott Exp $
//============================================================================

#include <assert.h>
#include <fstream.h>
#include <iostream.h>
#include <strstream.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <string>

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include "bspf.hxx"
#include "Console.hxx"
#include "DefProps.hxx"
#include "Event.hxx"
#include "MediaSrc.hxx"
#include "PropsSet.hxx"
#include "SndUnix.hxx"
#include "System.hxx"

#ifdef LINUX_JOYSTICK
  #include <unistd.h>
  #include <fcntl.h>
  #include <linux/joystick.h>

  // File descriptors for the joystick devices
  int theLeftJoystickFd;
  int theRightJoystickFd;
#endif

// Globals for X windows stuff
Display* theDisplay;
string theDisplayName = "";
int theScreen;
Visual* theVisual;
Window theWindow;
Colormap thePrivateColormap;
bool theUsePrivateColormapFlag = false;

// A graphic context for each of the 2600's colors
GC theGCTable[256];

// Enumeration of the possible window sizes
enum WindowSize { Small = 1, Medium = 2, Large = 3 };

// Indicates the current size of the window
WindowSize theWindowSize = Medium;

// Indicates the width and height of the game display based on properties
uInt32 theHeight;
uInt32 theWidth;

// Pointer to the console object or the null pointer
Console* theConsole;

// Event object to use
Event theEvent;

// Indicates if the entire frame should be redrawn
bool theRedrawEntireFrameFlag = true;

// Indicates if the user wants to quit
bool theQuitIndicator = false;

// Indicates what the desired frame rate is
uInt32 theDesiredFrameRate = 60;

// Indicate which paddle mode we're using:
//   0 - Mouse emulates paddle 0
//   1 - Mouse emulates paddle 1
//   2 - Mouse emulates paddle 2
//   3 - Mouse emulates paddle 3
//   4 - Use real Atari 2600 paddles
uInt32 thePaddleMode = 0;

/**
  This routine should be called once the console is created to setup
  the X11 connection and open a window for us to use
*/
void setupX11()
{
  // Get the desired width and height of the display
  theHeight = theConsole->mediaSource().height();
  theWidth = theConsole->mediaSource().width();

  // Figure out the desired size of the window
  int width = theWidth;
  int height = theHeight;
  if(theWindowSize == Small)
  {
    width *= 2;
  }
  else if(theWindowSize == Medium)
  {
    width *= 4;
    height *= 2;
  }
  else
  {
    width *= 6;
    height *= 3;
  }

  // Open a connection to the X server
  if(theDisplayName == "")
    theDisplay = XOpenDisplay(NULL);
  else
    theDisplay = XOpenDisplay(theDisplayName.c_str());

  // Verify that the connection was made
  if(theDisplay == NULL)
  {
    cerr << "ERROR: Cannot open X Windows display...\n";
    exit(1);
  }

  theScreen = DefaultScreen(theDisplay);
  theVisual = DefaultVisual(theDisplay, theScreen);
  Window rootWindow = RootWindow(theDisplay, theScreen);

  theWindow = XCreateSimpleWindow(theDisplay, rootWindow, 0, 0,
      width, height, CopyFromParent, CopyFromParent, 
      BlackPixel(theDisplay, theScreen));

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

  XSetStandardProperties(theDisplay, theWindow, name, name, None, 0, 0, &hints);

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

  // If requested install a private colormap for the window
  if(theUsePrivateColormapFlag)
  {
    XSetWindowColormap(theDisplay, theWindow, thePrivateColormap);
  }

  XSelectInput(theDisplay, theWindow, ExposureMask);
  XMapWindow(theDisplay, theWindow);

  XEvent event;
  do
  {
    XNextEvent(theDisplay, &event);
  } while (event.type != Expose);

  uInt32 mask = ExposureMask | KeyPressMask | KeyReleaseMask;

  // If we're using the mouse for paddle emulation then enable mouse events
  if(((theConsole->properties().get("Controller.Left") == "Paddles") ||
      (theConsole->properties().get("Controller.Right") == "Paddles"))
    && (thePaddleMode != 4)) 
  {
    mask |= (PointerMotionMask | ButtonPressMask | ButtonReleaseMask);
  }

  XSelectInput(theDisplay, theWindow, mask);
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

  XEvent event;

  while(XCheckWindowEvent(theDisplay, theWindow, 
      ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask |
      ButtonReleaseMask | PointerMotionMask, &event))
  {
    char buffer[20];
    KeySym key;
    XComposeStatus compose;

    if((event.type == KeyPress) || (event.type == KeyRelease))
    {
      XLookupString(&event.xkey, buffer, 20, &key, &compose);
      if((key == XK_Escape) && (event.type == KeyPress))
      {
        theQuitIndicator = true;
      }
      else if((key == XK_equal) && (event.type == KeyPress))
      {
        if(theWindowSize == Small)
          theWindowSize = Medium;
        else if(theWindowSize == Medium)
          theWindowSize = Large;
        else
          theWindowSize = Small;

        // Figure out the desired size of the window
        int width = theWidth;
        int height = theHeight;
        if(theWindowSize == Small)
        {
          width *= 2;
        }
        else if(theWindowSize == Medium)
        {
          width *= 4;
          height *= 2;
        }
        else
        {
          width *= 6;
          height *= 3;
        }

        XSizeHints hints;
        hints.flags = PSize | PMinSize | PMaxSize;
        hints.min_width = hints.max_width = hints.width = width;
        hints.min_height = hints.max_height = hints.height = height;
        XSetStandardProperties(theDisplay, theWindow, 0, 0, None, 0, 0, &hints);
        XResizeWindow(theDisplay, theWindow, width, height); 

        theRedrawEntireFrameFlag = true;
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

      if(theWindowSize == Small)
      {
        int x = (theWidth * 2) - event.xmotion.x;
        resistance = (Int32)((1000000.0 * x) / (theWidth * 2));
      }
      else if(theWindowSize == Medium)
      {
        int x = (theWidth * 4) - event.xmotion.x;
        resistance = (Int32)((1000000.0 * x) / (theWidth * 4));
      }
      else
      {
        int x = (theWidth * 6) - event.xmotion.x;
        resistance = (Int32)((1000000.0 * x) / (theWidth * 6));
      }

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
  Display a usage message and exit the program
*/
void usage()
{
  static const char* message[] = {
    "",
    "Usage: xstella [option ...] file",
    "",
    "Valid options are:",
    "",
    "  -display <display>      Connect to the designated X display",
    "  -fps <number>           Display the given number of frames per second",
    "  -owncmap                Install a private colormap",
#ifdef LINUX_JOYSTICK
    "  -paddle <0|1|2|3|real>  Indicates which paddle the mouse should emulate",
    "                          or that real Atari 2600 paddles are being used",
#else
    "  -paddle <0|1|2|3>       Indicates which paddle the mouse should emulate",
#endif
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
  Setup the properties set by loading builtin defaults and then a
  set of user specific ones from the file $HOME/.stella.pro

  @param set The properties set to setup
*/
void setupProperties(PropertiesSet& set)
{
  // Try to load the file $HOME/.stella.pro file
  string filename = getenv("HOME");
  filename += "/.stella.pro";

  // See if we can open the file $HOME/.stella.pro
  ifstream stream(filename.c_str()); 
  if(stream)
  {
    // File was opened so load properties from it
    set.load(stream, &Console::defaultProperties());
  }
  else
  {
    // Couldn't open the file so use the builtin properties file
    strstream builtin;
    for(const char** p = defaultPropertiesFile(); *p != 0; ++p)
    {
      builtin << *p << endl;
    }

    set.load(builtin, &Console::defaultProperties());
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
  if((argc < 2) || (argc > 9))
  {
    usage();
  }

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
    else
    {
      usage();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int main(int argc, char* argv[])
{
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
  PropertiesSet propertiesSet("Cartridge.Name");
  setupProperties(propertiesSet);

  // Create a sound object for use with the console
  SoundUnix sound;

  // Get just the filename of the file containing the ROM image
  const char* filename = (!strrchr(file, '/')) ? file : strrchr(file, '/') + 1;

  // Create the 2600 game console
  theConsole = new Console(image, size, filename, 
      theEvent, propertiesSet, sound);

  // Free the image since we don't need it any longer
  delete[] image;

#ifdef LINUX_JOYSTICK
  // Open the joystick devices
  theLeftJoystickFd = open("/dev/js0", O_RDONLY | O_NONBLOCK);
  theRightJoystickFd = open("/dev/js1", O_RDONLY | O_NONBLOCK);
#endif

  // Setup X windows
  setupX11();

  // Get the starting time in case we need to print statistics
  timeval startingTime;
  gettimeofday(&startingTime, 0);

  uInt32 numberOfFrames = 0;
  for( ; ; ++numberOfFrames)
  {
    // Exit if the user wants to quit
    if(theQuitIndicator)
    {
      break;
    }

    // Remember the current time before we start drawing the frame
    timeval before;
    gettimeofday(&before, 0);

    // Draw the frame and handle events
    theConsole->mediaSource().update();
    updateDisplay(theConsole->mediaSource());
    handleEvents();

    // Now, waste time if we need to so that we are at the desired frame rate
    timeval after;
    for(;;)
    {
      gettimeofday(&after, 0);

      uInt32 delta = (uInt32)((after.tv_sec - before.tv_sec) * 1000000 +
          (after.tv_usec - before.tv_usec));

      if(delta > (1000000 / theDesiredFrameRate))
      {
        break;
      }
    }
  }

  timeval endingTime;
  gettimeofday(&endingTime, 0);
  double executionTime = (endingTime.tv_sec - startingTime.tv_sec) +
          ((endingTime.tv_usec - startingTime.tv_usec) / 1000000.0);
  double framesPerSecond = numberOfFrames / executionTime;

  cout << endl;
  cout << numberOfFrames << " total frames drawn\n";
  cout << framesPerSecond << " frames/second\n";
  cout << theConsole->mediaSource().scanlines() << " scanlines in last frame\n";
  cout << endl;

  delete theConsole;

  // If we're using a private colormap then let's free it to be safe
  if(theUsePrivateColormapFlag)
  {
     XFreeColormap(theDisplay, thePrivateColormap);
  }
}

