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
// $Id: mainSDL.cxx,v 1.18 2002-03-28 23:11:20 stephena Exp $
//============================================================================

#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include <SDL.h>
#include <SDL_syswm.h>

#include "bspf.hxx"
#include "Console.hxx"
#include "Event.hxx"
#include "MediaSrc.hxx"
#include "PropsSet.hxx"
#include "System.hxx"
#include "SndUnix.hxx"
#include "RectList.hxx"
#include "Settings.hxx"

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

#define HAVE_GETTIMEOFDAY 1

// function prototypes
static bool setupDisplay();
static bool setupJoystick();
static bool createScreen(int width, int height);
static void recalculate8BitPalette();
static void setupPalette();
static void cleanup();

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
static SDL_Surface* screen;
static Uint32 palette[256];
static int bpp;
static Display* theX11Display;
static Window theX11Window;
static int theX11Screen;
static int mouseX = 0;
static bool x11Available = false;
static SDL_SysWMinfo info;
static int sdlflags;
static RectList* rectList;
static SDL_Joystick* theLeftJoystick;
static SDL_Joystick* theRightJoystick;

#ifdef HAVE_PNG
  static Snapshot* snapshot;
#endif

struct Switches
{
  SDLKey scanCode;
  Event::Type eventCode;
};

static Switches list[] = {
    { SDLK_1,           Event::KeyboardZero1 },
    { SDLK_2,           Event::KeyboardZero2 },
    { SDLK_3,           Event::KeyboardZero3 },
    { SDLK_q,           Event::KeyboardZero4 },
    { SDLK_w,           Event::KeyboardZero5 },
    { SDLK_e,           Event::KeyboardZero6 },
    { SDLK_a,           Event::KeyboardZero7 },
    { SDLK_s,           Event::KeyboardZero8 },
    { SDLK_d,           Event::KeyboardZero9 },
    { SDLK_z,           Event::KeyboardZeroStar },
    { SDLK_x,           Event::KeyboardZero0 },
    { SDLK_c,           Event::KeyboardZeroPound },

    { SDLK_8,           Event::KeyboardOne1 },
    { SDLK_9,           Event::KeyboardOne2 },
    { SDLK_0,           Event::KeyboardOne3 },
    { SDLK_i,           Event::KeyboardOne4 },
    { SDLK_o,           Event::KeyboardOne5 },
    { SDLK_p,           Event::KeyboardOne6 },
    { SDLK_k,           Event::KeyboardOne7 },
    { SDLK_l,           Event::KeyboardOne8 },
    { SDLK_SEMICOLON,   Event::KeyboardOne9 },
    { SDLK_COMMA,       Event::KeyboardOneStar },
    { SDLK_PERIOD,      Event::KeyboardOne0 },
    { SDLK_SLASH,       Event::KeyboardOnePound },

    { SDLK_UP,          Event::JoystickZeroUp },
    { SDLK_DOWN,        Event::JoystickZeroDown },
    { SDLK_LEFT,        Event::JoystickZeroLeft },
    { SDLK_RIGHT,       Event::JoystickZeroRight },
    { SDLK_SPACE,       Event::JoystickZeroFire }, 
    { SDLK_RETURN,      Event::JoystickZeroFire }, 
    { SDLK_LCTRL,       Event::JoystickZeroFire }, 
    { SDLK_z,           Event::BoosterGripZeroTrigger },
    { SDLK_x,           Event::BoosterGripZeroBooster },

    { SDLK_w,           Event::JoystickZeroUp },
    { SDLK_s,           Event::JoystickZeroDown },
    { SDLK_a,           Event::JoystickZeroLeft },
    { SDLK_d,           Event::JoystickZeroRight },
    { SDLK_TAB,         Event::JoystickZeroFire }, 
    { SDLK_1,           Event::BoosterGripZeroTrigger },
    { SDLK_2,           Event::BoosterGripZeroBooster },

    { SDLK_o,           Event::JoystickOneUp },
    { SDLK_l,           Event::JoystickOneDown },
    { SDLK_k,           Event::JoystickOneLeft },
    { SDLK_SEMICOLON,   Event::JoystickOneRight },
    { SDLK_j,           Event::JoystickOneFire }, 
    { SDLK_n,           Event::BoosterGripOneTrigger },
    { SDLK_m,           Event::BoosterGripOneBooster },

    { SDLK_F1,          Event::ConsoleSelect },
    { SDLK_F2,          Event::ConsoleReset },
    { SDLK_F3,          Event::ConsoleColor },
    { SDLK_F4,          Event::ConsoleBlackWhite },
    { SDLK_F5,          Event::ConsoleLeftDifficultyA },
    { SDLK_F6,          Event::ConsoleLeftDifficultyB },
    { SDLK_F7,          Event::ConsoleRightDifficultyA },
    { SDLK_F8,          Event::ConsoleRightDifficultyB }
  };

// Event objects to use
static Event theEvent;
static Event keyboardEvent;

// Pointer to the console object or the null pointer
static Console* theConsole;

// Pointer to the settings object or the null pointer
static Settings* settings;

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


/**
  This routine should be called once the console is created to setup
  the SDL window for us to use.  Return false if any operation fails,
  otherwise return true.
*/
bool setupDisplay()
{
  Uint32 initflags = SDL_INIT_VIDEO | SDL_INIT_JOYSTICK;
  if(SDL_Init(initflags) < 0)
    return false;

  // Check which system we are running under
  x11Available = false;
  SDL_VERSION(&info.version);
  if(SDL_GetWMInfo(&info) > 0)
    if(info.subsystem == SDL_SYSWM_X11)
      x11Available = true;

  sdlflags = SDL_HWSURFACE;
  sdlflags |= settings->theUseFullScreenFlag ? SDL_FULLSCREEN : 0;
  sdlflags |= settings->theUsePrivateColormapFlag ? SDL_HWPALETTE : 0;

  // Get the desired width and height of the display
  settings->theWidth = theConsole->mediaSource().width();
  settings->theHeight = theConsole->mediaSource().height();

  // Get the maximum size of a window for THIS screen
  // Must be called after display and screen are known, as well as
  // theWidth and theHeight
  // Defaults to 3 on systems without X11, maximum of 4 on any system.
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

#ifdef HAVE_PNG
  // Take care of the snapshot stuff.
  snapshot = new Snapshot();

  if(settings->theSnapShotDir == "")
    settings->theSnapShotDir = getenv("HOME");
  if(settings->theSnapShotName == "")
    settings->theSnapShotName = "romname";
#endif

  // Set up the rectangle list to be used in updateDisplay
  rectList = new RectList();
  if(!rectList)
  {
    cerr << "ERROR: Unable to get memory for SDL rects" << endl;
    return false;
  }

  // Set the window title and icon
  char name[512];
  sprintf(name, "Stella: \"%s\"", 
      theConsole->properties().get("Cartridge.Name").c_str());
  SDL_WM_SetCaption(name, "stella");

  // Figure out the desired size of the window
  int width  = settings->theWidth  * settings->theWindowSize * 2;
  int height = settings->theHeight * settings->theWindowSize;

  // Create the screen
  if(!createScreen(width, height))
    return false;
  setupPalette();

  // Make sure that theUseFullScreenFlag sets up fullscreen mode correctly
  if(settings->theUseFullScreenFlag)
  {
    grabMouse(true);
    showCursor(false);
    isFullscreen = true;
  }
  else
  {
    // Keep mouse in game window if grabmouse is selected
    grabMouse(settings->theGrabMouseFlag);

    // Show or hide the cursor depending on the 'hidecursor' argument
    showCursor(!settings->theHideCursorFlag);
  }

  // Center the window if centering is selected and not fullscreen
  if(settings->theCenterWindowFlag && !settings->theUseFullScreenFlag)
    centerWindow();

  return true;
}


/**
  This routine should be called once setupDisplay is called
  to create the joystick stuff.
*/
bool setupJoystick()
{
  if(SDL_NumJoysticks() <= 0)
  {
    cout << "No joysticks present, use the keyboard.\n";
    theLeftJoystick = theRightJoystick = 0;
    return true;
  }

  if((theLeftJoystick = SDL_JoystickOpen(0)) != NULL)
    cout << "Left joystick is a " << SDL_JoystickName(0) <<
      " with " << SDL_JoystickNumButtons(theLeftJoystick) << " buttons.\n";
  else
    cout << "Left joystick not present, use keyboard instead.\n";

  if((theRightJoystick = SDL_JoystickOpen(1)) != NULL)
    cout << "Right joystick is a " << SDL_JoystickName(1) <<
      " with " << SDL_JoystickNumButtons(theRightJoystick) << " buttons.\n";
  else
    cout << "Right joystick not present, use keyboard instead.\n";

  return true;
}


/**
  This routine is called whenever the screen needs to be recreated.
  It updates the global screen variable.  When this happens, the
  8-bit palette needs to be recalculated.
*/
bool createScreen(int w, int h)
{
  screen = SDL_SetVideoMode(w, h, 0, sdlflags);
  if(screen == NULL)
  {
    cerr << "ERROR: Unable to open SDL window: " << SDL_GetError() << endl;
    return false;
  }

  bpp = screen->format->BitsPerPixel;
  if(bpp == 8)
    recalculate8BitPalette();

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
}


/**
  Set up the palette for a screen with > 8 bits
*/
void setupPalette()
{
  if(bpp == 8)
    return;

  const uInt32* gamePalette = theConsole->mediaSource().palette();
  for(uInt32 i = 0; i < 256; ++i)
  {
    Uint8 r, g, b;

    r = (Uint8) ((gamePalette[i] & 0x00ff0000) >> 16);
    g = (Uint8) ((gamePalette[i] & 0x0000ff00) >> 8);
    b = (Uint8) (gamePalette[i] & 0x000000ff);

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
  of the screen.
*/
void resizeWindow(int mode)
{
  if(isFullscreen)
    return;

  if(mode == 1)   // increase size
  {
    if(settings->theWindowSize == settings->theMaxWindowSize)
      settings->theWindowSize = 1;
    else
      settings->theWindowSize++;
  }
  else   // decrease size
  {
    if(settings->theWindowSize == 1)
      settings->theWindowSize = settings->theMaxWindowSize;
    else
      settings->theWindowSize--;
  }

  // Figure out the desired size of the window
  int width  = settings->theWidth  * settings->theWindowSize * 2;
  int height = settings->theHeight * settings->theWindowSize;

  if(!createScreen(width, height))
    return;

  theRedrawEntireFrameIndicator = true;

  // A resize may mean that the window is no longer centered
  isCentered = false;

  if(settings->theCenterWindowFlag)
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
}


/**
  Toggles between fullscreen and window mode.  Grabmouse and hidecursor
  activated when in fullscreen mode.
*/
void toggleFullscreen()
{
  int width  = settings->theWidth  * settings->theWindowSize * 2;
  int height = settings->theHeight * settings->theWindowSize;

  isFullscreen = !isFullscreen;
  if(isFullscreen)
    sdlflags |= SDL_FULLSCREEN;
  else
    sdlflags &= ~SDL_FULLSCREEN;

  if(!createScreen(width, height))
    return;

  if(isFullscreen)  // now in fullscreen mode
  {
    grabMouse(true);
    showCursor(false);
  }
  else    // now in windowed mode
  {
    grabMouse(settings->theGrabMouseFlag);
    showCursor(!settings->theHideCursorFlag);

    if(settings->theCenterWindowFlag)
        centerWindow();
  }

  theRedrawEntireFrameIndicator = true;
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
  uInt16 screenMultiple = (uInt16) settings->theWindowSize;

  uInt32 width  = settings->theWidth;
  uInt32 height = settings->theHeight;

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
          settings->theGrabMouseFlag = !settings->theGrabMouseFlag;
          grabMouse(settings->theGrabMouseFlag);
        }
      }
      else if(key == SDLK_h)
      {
        // don't change hidecursor in fullscreen mode
        if(!isFullscreen)
        {
          settings->theHideCursorFlag = !settings->theHideCursorFlag;
          showCursor(!settings->theHideCursorFlag);
        }
      }
      else // check all the other keys
      {
        for(unsigned int i = 0; i < sizeof(list) / sizeof(Switches); ++i)
        { 
          if(list[i].scanCode == key)
          {
            theEvent.set(list[i].eventCode, 1);
            keyboardEvent.set(list[i].eventCode, 1);
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
          theEvent.set(list[i].eventCode, 0);
          keyboardEvent.set(list[i].eventCode, 0);
        }
      }
    }
    else if(event.type == SDL_MOUSEMOTION)
    {
      int resistance = 0, x = 0;
      float fudgeFactor = 1000000.0;
      Int32 width   = settings->theWidth * settings->theWindowSize * 2;

      // Grabmouse and hidecursor introduce some lag into the mouse movement,
      // so we need to fudge the numbers a bit
      if(settings->theGrabMouseFlag && settings->theHideCursorFlag)
      {
        mouseX = (int)((float)mouseX + (float)event.motion.xrel
                 * 1.5 * (float) settings->theWindowSize);
      }
      else
      {
        mouseX = mouseX + event.motion.xrel * settings->theWindowSize;
      }

      // Check to make sure mouseX is within the game window
      if(mouseX < 0)
        mouseX = 0;
      else if(mouseX > width)
        mouseX = width;
  
      x = width - mouseX;
      resistance = (Int32)((fudgeFactor * x) / width);

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
    else if(event.type == SDL_MOUSEBUTTONDOWN)
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
    else if(event.type == SDL_MOUSEBUTTONUP)
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
          if(settings->thePaddleMode == 4)
            theEvent.set(Event::PaddleZeroFire, state);
        }
        else if(button == 1)  // booster button
        {
          theEvent.set(Event::BoosterGripZeroTrigger, state ? 
              1 : keyboardEvent.get(Event::BoosterGripZeroTrigger));

          // If we're using real paddles then set paddle event as well
          if(settings->thePaddleMode == 4)
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
          if(settings->thePaddleMode == 4)
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
          if(settings->thePaddleMode == 4)
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
          if(settings->thePaddleMode == 4)
            theEvent.set(Event::PaddleTwoFire, state);
        }
        else if(button == 1)  // booster button
        {
          theEvent.set(Event::BoosterGripOneTrigger, state ? 
              1 : keyboardEvent.get(Event::BoosterGripOneTrigger));

          // If we're using real paddles then set paddle event as well
          if(settings->thePaddleMode == 4)
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
          if(settings->thePaddleMode == 4)
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
          if(settings->thePaddleMode == 4)
          {
            uInt32 r = (uInt32)((1.0E6L * (value + 32767L)) / 65536);
            theEvent.set(Event::PaddleThreeResistance, r);
          }
        }
      }
    }
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
  if(!snapshot)
  {
    cerr << "Snapshot support disabled.\n";
    return;
  }

  // Now find the correct name for the snapshot
  string filename = settings->theSnapShotDir;
  if(settings->theSnapShotName == "romname")
    filename = filename + "/" + theConsole->properties().get("Cartridge.Name");
  else if(settings->theSnapShotName == "md5sum")
    filename = filename + "/" + theConsole->properties().get("Cartridge.MD5");
  else
  {
    cerr << "ERROR: unknown name " << settings->theSnapShotName
         << " for snapshot type" << endl;
    return;
  }

  // Replace all spaces in name with underscores
  replace(filename.begin(), filename.end(), ' ', '_');

  // Check whether we want multiple snapshots created
  if(settings->theMultipleSnapShotFlag)
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
    "SDL Stella version 1.2",
    "",
    "Usage: stella.sdl [option ...] file",
    "",
    "Valid options are:",
    "",
    "  -fps <number>           Display the given number of frames per second",
    "  -owncmap                Install a private colormap",
    "  -zoom <size>            Makes window be 'size' times normal (1 - 4)",
    "  -fullscreen             Play the game in fullscreen mode",
    "  -grabmouse              Keeps the mouse in the game window",
    "  -hidecursor             Hides the mouse cursor in the game window",
    "  -center                 Centers the game window onscreen",
    "  -volume <number>        Set the volume (0 - 100)",
    "  -paddle <0|1|2|3|real>  Indicates which paddle the mouse should emulate",
    "                          or that real Atari 2600 paddles are being used",
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
  if(settings->theAlternateProFile != "")
  {
    if(access(settings->theAlternateProFile.c_str(), R_OK) == 0)
    {
      set.load(settings->theAlternateProFile, &Console::defaultProperties(), false);
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

  if(rectList)
    delete rectList;

  if(SDL_WasInit(SDL_INIT_EVERYTHING))
  {
    if(SDL_JoystickOpened(0))
      SDL_JoystickClose(theLeftJoystick);
    if(SDL_JoystickOpened(1))
      SDL_JoystickClose(theRightJoystick);

    SDL_Quit();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int main(int argc, char* argv[])
{
  // First create some settings for the emulator
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
  in.read(image, 512 * 1024);
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

  // Create a sound object for use with the console
  SoundUnix sound(settings->theDesiredVolume);

  // Get just the filename of the file containing the ROM image
  const char* filename = (!strrchr(file, '/')) ? file : strrchr(file, '/') + 1;

  // Create the 2600 game console
  theConsole = new Console(image, size, filename, 
      theEvent, propertiesSet, sound);

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

#ifdef EXPERIMENTAL_TIMING
  // Set up timing stuff
  uInt32 startTime, frameTime, virtualTime, currentTime;
  uInt32 numberOfFrames = 0;
  uInt32 timePerFrame = (uInt32) (1000000.0 / (double) theDesiredFrameRate);

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
    updateDisplay(theConsole->mediaSource());
    handleEvents();

    currentTime = getTicks();
    virtualTime += timePerFrame;
    if(currentTime < virtualTime)
    {
      SDL_Delay((virtualTime - currentTime)/1000);
    }

    currentTime = getTicks() - startTime;
    frameTime += currentTime;
    ++numberOfFrames;

//    cerr << "FPS = " << (double) numberOfFrames / ((double) frameTime / 1000000.0) << endl;
  }
#else
  // Set up timing stuff
  uInt32 startTime, frameTime, delta;
  uInt32 numberOfFrames = 0;
  uInt32 timePerFrame = (uInt32) (1000000.0 / (double) settings->theDesiredFrameRate);

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
      SDL_Delay(10);
      continue;
    }

    startTime = getTicks();
    theConsole->mediaSource().update();
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
#endif

  if(settings->theShowInfoFlag)
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

  return (uInt32) (now.tv_sec * 1000000 + now.tv_usec);
}
#else
inline uInt32 getTicks()
{
  return (uInt32) SDL_GetTicks() * 1000;
}
#endif
