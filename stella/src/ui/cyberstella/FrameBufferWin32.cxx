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
// $Id: FrameBufferWin32.cxx,v 1.1 2003-11-11 18:55:39 stephena Exp $
//============================================================================

#include <SDL.h>
#include <SDL_syswm.h>
#include <sstream>

#include "Console.hxx"
#include "FrameBuffer.hxx"
#include "FrameBufferWin32.hxx"
#include "MediaSrc.hxx"
#include "Settings.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferWin32::FrameBufferWin32()
   :  theZoomLevel(1),
      theMaxZoomLevel(1),
      isFullscreen(false)
{
cerr << "FrameBufferWin32::FrameBufferWin32()\n";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferWin32::~FrameBufferWin32()
{
cerr << "FrameBufferWin32::~FrameBufferWin32()\n";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferWin32::init()
{
cerr << "FrameBufferWin32::init()\n";
return false;
#if 0
  // Get the desired width and height of the display
  myWidth  = myMediaSource->width() << 1;
  myHeight = myMediaSource->height();

  // Now create the software SDL screen
  Uint32 initflags = SDL_INIT_VIDEO | SDL_INIT_TIMER;
  if(SDL_Init(initflags) < 0)
    return false;

  // Check which system we are running under
  x11Available = false;
/*  SDL_VERSION(&myWMInfo.version);
  if(SDL_GetWMInfo(&myWMInfo) > 0)
    if(myWMInfo.subsystem == SDL_SYSWM_X11)
      x11Available = true;
*/
  // Get the maximum size of a window for THIS screen
  theMaxZoomLevel = maxWindowSizeForScreen();

  // Check to see if window size will fit in the screen
  if((uInt32)myConsole->settings().getInt("zoom") > theMaxZoomLevel)
    theZoomLevel = theMaxZoomLevel;
  else
    theZoomLevel = myConsole->settings().getInt("zoom");

  mySDLFlags = SDL_SWSURFACE;
  mySDLFlags |= myConsole->settings().getBool("fullscreen") ? SDL_FULLSCREEN : 0;

  // Set up the rectangle list to be used in the dirty update
  myRectList = new RectList();
  if(!myRectList)
  {
    cerr << "ERROR: Unable to get memory for SDL rects" << endl;
    return false;
  }

  // Set the window title and icon
  ostringstream name;
  name << "Stella: \"" << myConsole->properties().get("Cartridge.Name") << "\"";
  SDL_WM_SetCaption(name.str().c_str(), "stella");

  // Create the screen
  if(!createScreen())
    return false;
  setupPalette(1.0);

  // Make sure that theUseFullScreenFlag sets up fullscreen mode correctly
  theGrabMouseIndicator  = myConsole->settings().getBool("grabmouse");
  theHideCursorIndicator = myConsole->settings().getBool("hidecursor");
  if(myConsole->settings().getBool("fullscreen"))
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
  if(myConsole->settings().getBool("center") &&
     !myConsole->settings().getBool("fullscreen"))
    centerScreen();

  return true;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::drawMediaSource()
{
cerr << "FrameBufferWin32::drawMediaSource()\n";
#if 0
  uInt8* currentFrame   = myMediaSource->currentFrameBuffer();
  uInt8* previousFrame  = myMediaSource->previousFrameBuffer();
  uInt16 screenMultiple = (uInt16) theZoomLevel;

  uInt32 width  = myMediaSource->width();
  uInt32 height = myMediaSource->height();

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
        SDL_Rect temp;

        temp.x = active.x * screenMultiple << 1;
        temp.y = active.y * screenMultiple;
        temp.w = active.width  * screenMultiple << 1;
        temp.h = active.height * screenMultiple;

        myRectList->add(&temp);
        SDL_FillRect(myScreen, &temp, palette[active.color]);

        ++activeIndex;
      }
    }

    // Flush any remaining active rectangles
    for(uInt16 s = activeIndex; s < activeCount; ++s)
    {
      Rectangle& active = activeRectangles[s];

      SDL_Rect temp;
      temp.x = active.x * screenMultiple << 1;
      temp.y = active.y * screenMultiple;
      temp.w = active.width  * screenMultiple << 1;
      temp.h = active.height * screenMultiple;

      myRectList->add(&temp);
      SDL_FillRect(myScreen, &temp, palette[active.color]);
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

    SDL_Rect temp;
    temp.x = active.x * screenMultiple << 1;
    temp.y = active.y * screenMultiple;
    temp.w = active.width  * screenMultiple << 1;
    temp.h = active.height * screenMultiple;

    myRectList->add(&temp);
    SDL_FillRect(myScreen, &temp, palette[active.color]);
  }

  // The frame doesn't need to be completely redrawn anymore
  theRedrawEntireFrameIndicator = false;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::preFrameUpdate()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::postFrameUpdate()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferWin32::createScreen()
{
return false;
#if 0
  int w = myWidth  * theZoomLevel;
  int h = myHeight * theZoomLevel;

  myScreen = SDL_SetVideoMode(w, h, 0, mySDLFlags);
  if(myScreen == NULL)
  {
    cerr << "ERROR: Unable to open SDL window: " << SDL_GetError() << endl;
    return false;
  }

  theRedrawEntireFrameIndicator = true;
  return true;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::setupPalette(float shade)
{
#if 0
  const uInt32* gamePalette = myMediaSource->palette();
  for(uInt32 i = 0; i < 256; ++i)
  {
    Uint8 r, g, b;

    r = (Uint8) (((gamePalette[i] & 0x00ff0000) >> 16) * shade);
    g = (Uint8) (((gamePalette[i] & 0x0000ff00) >> 8) * shade);
    b = (Uint8) ((gamePalette[i] & 0x000000ff) * shade);

    switch(myScreen->format->BitsPerPixel)
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
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::pause(bool status)
{
#if 0
  myPauseStatus = status;

  // Shade the palette to 75% normal value in pause mode
  if(myPauseStatus)
    setupPalette(0.75);
  else
    setupPalette(1.0);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::toggleFullscreen()
{
#if 0
  isFullscreen = !isFullscreen;
  if(isFullscreen)
    mySDLFlags |= SDL_FULLSCREEN;
  else
    mySDLFlags &= ~SDL_FULLSCREEN;

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

    if(myConsole->settings().getBool("center"))
      centerScreen();
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::resize(int mode)
{
#if 0
  // reset size to that given in properties
  // this is a special case of allowing a resize while in fullscreen mode
  if(mode == 0)
  {
    myWidth  = myMediaSource->width() << 1;
    myHeight = myMediaSource->height();
  }
  else if(mode == 1)   // increase size
  {
    if(isFullscreen)
      return;

    if(theZoomLevel == theMaxZoomLevel)
      theZoomLevel = 1;
    else
      theZoomLevel++;
  }
  else if(mode == -1)   // decrease size
  {
    if(isFullscreen)
      return;

    if(theZoomLevel == 1)
      theZoomLevel = theMaxZoomLevel;
    else
      theZoomLevel--;
  }

  if(!createScreen())
    return;

  // A resize may mean that the window is no longer centered
  isCentered = false;

  if(myConsole->settings().getBool("center"))
    centerScreen();

  // Now update the settings
  ostringstream tmp;
  tmp << theZoomLevel;
  myConsole->settings().set("zoom", tmp.str());
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::showCursor(bool show)
{
#if 0
  if(isFullscreen)
    return;

  if(show)
    SDL_ShowCursor(SDL_ENABLE);
  else
    SDL_ShowCursor(SDL_DISABLE);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::grabMouse(bool grab)
{
#if 0
  if(isFullscreen)
    return;

  if(grab)
    SDL_WM_GrabInput(SDL_GRAB_ON);
  else
    SDL_WM_GrabInput(SDL_GRAB_OFF);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBufferWin32::maxWindowSizeForScreen()
{
return 1;
#if 0
  if(!x11Available)
    return 1;
/*  FIXME
  // Otherwise, lock the screen and get the width and height
  myWMInfo.info.x11.lock_func();
  Display* theX11Display = myWMInfo.info.x11.display;
  myWMInfo.info.x11.unlock_func();

  int screenWidth  = DisplayWidth(theX11Display, DefaultScreen(theX11Display));
  int screenHeight = DisplayHeight(theX11Display, DefaultScreen(theX11Display));

  uInt32 multiplier = screenWidth / myWidth;
  bool found = false;

  while(!found && (multiplier > 0))
  {
    // Figure out the desired size of the window
    int width  = myWidth  * multiplier;
    int height = myHeight * multiplier;

    if((width < screenWidth) && (height < screenHeight))
      found = true;
    else
      multiplier--;
  }

  if(found)
    return multiplier;
  else
    return 1;
*/
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::drawBoundedBox(uInt32 x, uInt32 y, uInt32 w, uInt32 h)
{
cerr << "FrameBufferWin32::drawBoundedBox()\n";
#if 0
  SDL_Rect tmp;

  // Scale all values to the current window size
  x *= theZoomLevel;
  y *= theZoomLevel;
  w *= theZoomLevel;
  h *= theZoomLevel;

  // First draw the underlying box
  tmp.x = x;
  tmp.y = y;
  tmp.w = w;
  tmp.h = h;
  myRectList->add(&tmp);
  SDL_FillRect(myScreen, &tmp, palette[bg]);

  // Now draw the bounding sides
  tmp.x = x;
  tmp.y = y;
  tmp.w = w;
  tmp.h = theZoomLevel;
  SDL_FillRect(myScreen, &tmp, palette[fg]);  // top

  tmp.x = x;
  tmp.y = y + h - theZoomLevel;
  tmp.w = w;
  tmp.h = theZoomLevel;
  SDL_FillRect(myScreen, &tmp, palette[fg]);  // bottom

  tmp.x = x;
  tmp.y = y;
  tmp.w = theZoomLevel;
  tmp.h = h;
  SDL_FillRect(myScreen, &tmp, palette[fg]);  // left

  tmp.x = x + w - theZoomLevel;
  tmp.y = y;
  tmp.w = theZoomLevel;
  tmp.h = h;
  SDL_FillRect(myScreen, &tmp, palette[fg]);  // right
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::drawText(uInt32 xorig, uInt32 yorig, const string& message)
{
cerr << "FrameBufferWin32::drawText()\n";
#if 0
  SDL_Rect tmp;

  uInt8 length = message.length();
  for(uInt32 x = 0; x < length; x++)
  {
    for(uInt32 y = 0; y < 8; y++)
    {
      for(uInt32 z = 0; z < 8; z++)
      {
        char letter = message[x];
        if((ourFontData[(letter << 3) + y] >> z) & 1)
        {
//          myFrameBuffer[(y + yorig)*myWidth + (x<<3) + z + xorig] = 0xF0F0F0;
          tmp.x = ((x<<3) + z + xorig) * theZoomLevel;
          tmp.y = (y + yorig) * theZoomLevel;
          tmp.w = tmp.h = theZoomLevel;
          SDL_FillRect(myScreen, &tmp, palette[fg]);
// FIXME - this can be a lot more efficient
        }
      }
    }
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::drawChar(uInt32 xorig, uInt32 yorig, uInt32 c)
{
cerr << "FrameBufferWin32::drawChar()\n";
#if 0
  if(c >= 256 )
    return;

  SDL_Rect tmp;

  for(uInt32 y = 0; y < 8; y++)
  {
    for(uInt32 z = 0; z < 8; z++)
    {
      if((ourFontData[(c << 3) + y] >> z) & 1)
      {
//        myFrameBuffer[(y + yorig)*myWidth + z + xorig] = 0xF0F0F0;
        tmp.x = (z + xorig) * theZoomLevel;
        tmp.y = (y + yorig) * theZoomLevel;
        tmp.w = tmp.h = theZoomLevel;
        myRectList->add(&tmp);
        SDL_FillRect(myScreen, &tmp, palette[fg]);
// FIXME - this can be a lot more efficient
      }
    }
  }
#endif
}
