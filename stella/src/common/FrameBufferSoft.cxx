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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FrameBufferSoft.cxx,v 1.11 2005-03-13 03:38:39 stephena Exp $
//============================================================================

#include <SDL.h>
#include <SDL_syswm.h>
#include <sstream>

#include "Console.hxx"
#include "FrameBuffer.hxx"
#include "FrameBufferSoft.hxx"
#include "MediaSrc.hxx"
#include "Settings.hxx"
#include "OSystem.hxx"
#include "StellaFont.hxx"
#include "GuiUtils.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferSoft::FrameBufferSoft(OSystem* osystem)
    : FrameBuffer(osystem)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferSoft::~FrameBufferSoft()
{
  if(myRectList)
    delete myRectList;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferSoft::initSubsystem()
{
  mySDLFlags |= SDL_SWSURFACE;

  // Set up the rectangle list to be used in the dirty update
  myRectList = new RectList();
  if(!myRectList)
  {
    cerr << "ERROR: Unable to get memory for SDL rects" << endl;
    return false;
  }

  // Get the maximum size of a window for the desktop
  theMaxZoomLevel = maxWindowSizeForScreen();

  // Check to see if window size will fit in the screen
  if((uInt32)myOSystem->settings().getInt("zoom") > theMaxZoomLevel)
    theZoomLevel = theMaxZoomLevel;
  else
    theZoomLevel = myOSystem->settings().getInt("zoom");

  // Create the screen
  if(!createScreen())
    return false;
  setupPalette();

  // Show some info
  if(myOSystem->settings().getBool("showinfo"))
    cout << "Video rendering: Software mode" << endl << endl;

  // Precompute the GUI palette
  // We abuse the concept of 'enum' by referring directly to the integer values
  for(uInt8 i = 0; i < 5; i++)
    myGUIPalette[i] = mapRGB(myGUIColors[i][0], myGUIColors[i][1], myGUIColors[i][2]);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferSoft::createScreen()
{
  myDimensions.x = myDimensions.y = 0;
  myDimensions.w = myWidth  * theZoomLevel;
  myDimensions.h = myHeight * theZoomLevel;

  myScreen = SDL_SetVideoMode(myDimensions.w, myDimensions.h, 0, mySDLFlags);
  if(myScreen == NULL)
  {
    cerr << "ERROR: Unable to open SDL window: " << SDL_GetError() << endl;
    return false;
  }

  theRedrawEntireFrameIndicator = true;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::toggleFilter()
{
  // No filter added yet ...
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::drawMediaSource()
{
  MediaSource& mediasrc = myOSystem->console().mediaSource();

  uInt8* currentFrame   = mediasrc.currentFrameBuffer();
  uInt8* previousFrame  = mediasrc.previousFrameBuffer();
  uInt16 screenMultiple = (uInt16) theZoomLevel;

  uInt32 width  = mediasrc.width();
  uInt32 height = mediasrc.height();

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

/*
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
          SDL_FillRect(myScreen, &tmp, myPalette[10]);
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
        SDL_FillRect(myScreen, &tmp, myPalette[10]);
      }
    }
  }
}
*/

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
void FrameBufferSoft::hLine(uInt32 x, uInt32 y, uInt32 x2, OverlayColor color)
{
  SDL_Rect tmp;

  // Horizontal line
  tmp.x = x * theZoomLevel;
  tmp.y = y * theZoomLevel;
  tmp.w = (x2 - x + 1) * theZoomLevel;
  tmp.h = theZoomLevel;
  SDL_FillRect(myScreen, &tmp, myGUIPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::vLine(uInt32 x, uInt32 y, uInt32 y2, OverlayColor color)
{
  SDL_Rect tmp;

  // Vertical line
  tmp.x = x * theZoomLevel;
  tmp.y = y * theZoomLevel;
  tmp.w = theZoomLevel;
  tmp.h = (y2 - y + 1) * theZoomLevel;
  SDL_FillRect(myScreen, &tmp, myGUIPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::blendRect(int x, int y, int w, int h,
                                OverlayColor color, int level)
{
// FIXME - make this do alpha-blending
  SDL_Rect tmp;

  // Fill the rectangle
  tmp.x = x * theZoomLevel;
  tmp.y = y * theZoomLevel;
  tmp.w = w * theZoomLevel;
  tmp.h = h * theZoomLevel;
  myRectList->add(&tmp);
  SDL_FillRect(myScreen, &tmp, myGUIPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                               OverlayColor color)
{
  SDL_Rect tmp;

  // Fill the rectangle
  tmp.x = x * theZoomLevel;
  tmp.y = y * theZoomLevel;
  tmp.w = w * theZoomLevel;
  tmp.h = h * theZoomLevel;
  myRectList->add(&tmp);
  SDL_FillRect(myScreen, &tmp, myGUIPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::frameRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                                OverlayColor color)
{
  cerr << "FrameBufferSoft::frameRect()\n";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::drawChar(uInt8 chr, uInt32 xorig, uInt32 yorig,
                               OverlayColor color)
{
  // If this character is not included in the font, use the default char.
  if(chr < myFont->desc().firstchar ||
     chr >= myFont->desc().firstchar + myFont->desc().size)
  {
    if (chr == ' ')
      return;
    chr = myFont->desc().defaultchar;
  }

  const Int32 w = myFont->getCharWidth(chr);
  const Int32 h = myFont->getFontHeight();
  chr -= myFont->desc().firstchar;
  const uInt16* tmp = myFont->desc().bits + (myFont->desc().offset ?
                      myFont->desc().offset[chr] : (chr * h));

  SDL_Rect rect;
  for(int y = 0; y < h; y++)
  {
    const uInt16 buffer = *tmp++;
    uInt16 mask = 0x8000;
//    if(ty + y < 0 || ty + y >= dst->h)
//      continue;

    for(int x = 0; x < w; x++, mask >>= 1)
    {
//      if (tx + x < 0 || tx + x >= dst->w)
//        continue;
      if ((buffer & mask) != 0)
      {
        rect.x = (x + xorig) * theZoomLevel;
        rect.y = (y + yorig) * theZoomLevel;
        rect.w = rect.h = theZoomLevel;
        SDL_FillRect(myScreen, &rect, myGUIPalette[color]);
      }
    }
  }
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
