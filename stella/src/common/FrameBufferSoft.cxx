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
// $Id: FrameBufferSoft.cxx,v 1.2 2004-06-20 23:30:48 stephena Exp $
//============================================================================

#include <SDL.h>
#include <SDL_syswm.h>
#include <sstream>

#include "Console.hxx"
#include "FrameBuffer.hxx"
#include "FrameBufferSDL.hxx"
#include "FrameBufferSoft.hxx"
#include "MediaSrc.hxx"
#include "Settings.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferSoft::FrameBufferSoft()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferSoft::~FrameBufferSoft()
{
  if(myRectList)
    delete myRectList;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferSoft::createScreen()
{
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferSoft::init()
{
  // Get the desired width and height of the display
  myWidth  = myMediaSource->width() << 1;
  myHeight = myMediaSource->height();

  // Now create the software SDL screen
  Uint32 initflags = SDL_INIT_VIDEO | SDL_INIT_TIMER;
  if(SDL_Init(initflags) < 0)
    return false;

  // Check which system we are running under
  x11Available = false;
#if UNIX && (!__APPLE__)
  SDL_VERSION(&myWMInfo.version);
  if(SDL_GetWMInfo(&myWMInfo) > 0)
    if(myWMInfo.subsystem == SDL_SYSWM_X11)
      x11Available = true;
#endif

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
  setWindowAttributes();

  // Create the screen
  if(!createScreen())
    return false;
  setupPalette();

  // Make sure that theUseFullScreenFlag sets up fullscreen mode correctly
  if(myConsole->settings().getBool("fullscreen"))
  {
    grabMouse(true);
    showCursor(false);
  }
  else
  {
    // Keep mouse in game window if grabmouse is selected
    grabMouse(myConsole->settings().getBool("grabmouse"));

    // Show or hide the cursor depending on the 'hidecursor' argument
    showCursor(!myConsole->settings().getBool("hidecursor"));
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::drawMediaSource()
{
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
        SDL_FillRect(myScreen, &temp, myPalette[active.color]);

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
      SDL_FillRect(myScreen, &temp, myPalette[active.color]);
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
    SDL_FillRect(myScreen, &temp, myPalette[active.color]);
  }

  // The frame doesn't need to be completely redrawn anymore
  theRedrawEntireFrameIndicator = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::preFrameUpdate()
{
  // Start a new rectlist on each display update
  myRectList->start();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::postFrameUpdate()
{
  // Now update all the rectangles at once
  SDL_UpdateRects(myScreen, myRectList->numRects(), myRectList->rects());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::drawBoundedBox(uInt32 x, uInt32 y, uInt32 w, uInt32 h)
{
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
  SDL_FillRect(myScreen, &tmp, myPalette[myBGColor]);

  // Now draw the bounding sides
  tmp.x = x;
  tmp.y = y;
  tmp.w = w;
  tmp.h = theZoomLevel;
  SDL_FillRect(myScreen, &tmp, myPalette[myFGColor]);  // top

  tmp.x = x;
  tmp.y = y + h - theZoomLevel;
  tmp.w = w;
  tmp.h = theZoomLevel;
  SDL_FillRect(myScreen, &tmp, myPalette[myFGColor]);  // bottom

  tmp.x = x;
  tmp.y = y;
  tmp.w = theZoomLevel;
  tmp.h = h;
  SDL_FillRect(myScreen, &tmp, myPalette[myFGColor]);  // left

  tmp.x = x + w - theZoomLevel;
  tmp.y = y;
  tmp.w = theZoomLevel;
  tmp.h = h;
  SDL_FillRect(myScreen, &tmp, myPalette[myFGColor]);  // right
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::drawText(uInt32 xorig, uInt32 yorig, const string& message)
{
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
          tmp.x = ((x<<3) + z + xorig) * theZoomLevel;
          tmp.y = (y + yorig) * theZoomLevel;
          tmp.w = tmp.h = theZoomLevel;
          SDL_FillRect(myScreen, &tmp, myPalette[myFGColor]);
        }
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::drawChar(uInt32 xorig, uInt32 yorig, uInt32 c)
{
  if(c >= 256 )
    return;

  SDL_Rect tmp;

  for(uInt32 y = 0; y < 8; y++)
  {
    for(uInt32 z = 0; z < 8; z++)
    {
      if((ourFontData[(c << 3) + y] >> z) & 1)
      {
        tmp.x = (z + xorig) * theZoomLevel;
        tmp.y = (y + yorig) * theZoomLevel;
        tmp.w = tmp.h = theZoomLevel;
        SDL_FillRect(myScreen, &tmp, myPalette[myFGColor]);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::scanline(uInt32 row, uInt8* data)
{
  // Make sure no pixels are being modified
  SDL_LockSurface(myScreen);

  uInt32 bpp     = myScreen->format->BytesPerPixel;
  uInt8* start   = (uInt8*) myScreen->pixels;
  uInt32 yoffset = row * myScreen->pitch;
  uInt32 pixel = 0;
  uInt8 *p, r, g, b;

  for(Int32 x = 0; x < myScreen->w; x++)
  {
    p = (Uint8*) (start    +  // Start at top of RAM
                 (yoffset) +  // Go down 'row' lines
                 (x * bpp));  // Go in 'x' pixels

    switch(bpp)
    {
      case 1:
        pixel = *p;
        break;

      case 2:
        pixel = *(Uint16*) p;
        break;

      case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
          pixel = p[0] << 16 | p[1] << 8 | p[2];
        else
          pixel = p[0] | p[1] << 8 | p[2] << 16;
        break;

      case 4:
        pixel = *(Uint32*) p;
        break;
    }

    SDL_GetRGB(pixel, myScreen->format, &r, &g, &b);

    data[x * 3 + 0] = r;
    data[x * 3 + 1] = g;
    data[x * 3 + 2] = b;
  }

  SDL_UnlockSurface(myScreen);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RectList::RectList(Uint32 size)
{
  currentSize = size;
  currentRect = 0;

  rectArray = new SDL_Rect[currentSize];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RectList::~RectList()
{
  delete[] rectArray;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RectList::add(SDL_Rect* newRect)
{
  if(currentRect >= currentSize)
  {
    currentSize = currentSize * 2;
    SDL_Rect *temp = new SDL_Rect[currentSize];

    for(Uint32 i = 0; i < currentRect; ++i)
      temp[i] = rectArray[i];

    delete[] rectArray;
    rectArray = temp;
  }

  rectArray[currentRect].x = newRect->x;
  rectArray[currentRect].y = newRect->y;
  rectArray[currentRect].w = newRect->w;
  rectArray[currentRect].h = newRect->h;

  ++currentRect;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SDL_Rect* RectList::rects()
{
  return rectArray;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Uint32 RectList::numRects()
{
  return currentRect;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RectList::start()
{
  currentRect = 0;
}
