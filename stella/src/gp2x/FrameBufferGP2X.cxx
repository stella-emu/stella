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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FrameBufferGP2X.cxx,v 1.7 2006-12-05 14:53:26 stephena Exp $
//============================================================================

#include <SDL.h>

#include "Console.hxx"
#include "MediaSrc.hxx"
#include "OSystem.hxx"
#include "Font.hxx"
#include "GuiUtils.hxx"
#include "RectList.hxx"
#include "FrameBufferGP2X.hxx"

// Comment out entire line to test new rendering code
#define DIRTY_RECTS 1


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferGP2X::FrameBufferGP2X(OSystem* osystem)
  : FrameBuffer(osystem),
    myDirtyFlag(true),
    myRectList(NULL),
    myOverlayRectList(NULL)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferGP2X::~FrameBufferGP2X()
{
  delete myRectList;
  delete myOverlayRectList;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGP2X::initSubsystem()
{
  // Set up the rectangle list to be used in the dirty update
  delete myRectList;
  myRectList = new RectList();
  delete myOverlayRectList;
  myOverlayRectList = new RectList();

  if(!myRectList || !myOverlayRectList)
  {
    cerr << "ERROR: Unable to get memory for SDL rects" << endl;
    return false;
  }

  // Create the screen
  return createScreen();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::setAspectRatio()
{
  // Aspect ratio correction not yet available in software mode
  theAspectRatio = 1.0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::setScaler(Scaler scaler)
{
  // Not supported, we always use 1x zoom
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGP2X::createScreen()
{
  myScreenDim.x = myScreenDim.y = 0;

  myScreenDim.w = myBaseDim.w;
  myScreenDim.h = myBaseDim.h;

  // In software mode, the image and screen dimensions are always the same
  myImageDim = myScreenDim;

  myScreen = SDL_SetVideoMode(myScreenDim.w, myScreenDim.h, 16, mySDLFlags);
  if(myScreen == NULL)
  {
    cerr << "ERROR: Unable to open SDL window: " << SDL_GetError() << endl;
    return false;
  }
  myPitch = myScreen->pitch/2;

  // Erase old rects, since they've probably been scaled for
  // a different sized screen
  myRectList->start();
  myOverlayRectList->start();

  myDirtyFlag = true;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::drawMediaSource()
{
  MediaSource& mediasrc = myOSystem->console().mediaSource();

  uInt8* currentFrame  = mediasrc.currentFrameBuffer();
  uInt8* previousFrame = mediasrc.previousFrameBuffer();
  uInt32 width         = mediasrc.width();
  uInt32 height        = mediasrc.height();
  uInt16* buffer       = (uInt16*) myScreen->pixels;

  uInt32 bufofsY    = 0;
  uInt32 screenofsY = 0;

  if(!myUsePhosphor)
  {
#ifdef DIRTY_RECTS
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
        if((*current != *previous) || theRedrawTIAIndicator)
        {
          uInt8* c = (uInt8*)current;
          uInt8* p = (uInt8*)previous;

          // Look at each of the bytes that make up the uInt32
          for(uInt16 i = 0; i < 4; ++i, ++c, ++p)
          {
            // See if this pixel has changed
            if((*c != *p) || theRedrawTIAIndicator)
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

          temp.x = active.x << 1;
          temp.y = active.y;
          temp.w = active.width << 1;
          temp.h = active.height;

          myRectList->add(&temp);
          SDL_FillRect(myScreen, &temp, myDefPalette[active.color]);

          ++activeIndex;
        }
      }

      // Flush any remaining active rectangles
      for(uInt16 s = activeIndex; s < activeCount; ++s)
      {
        Rectangle& active = activeRectangles[s];
        SDL_Rect temp;
        temp.x = active.x << 1;
        temp.y = active.y;
        temp.w = active.width << 1;
        temp.h = active.height;

        myRectList->add(&temp);
        SDL_FillRect(myScreen, &temp, myDefPalette[active.color]);
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
      temp.x = active.x << 1;
      temp.y = active.y;
      temp.w = active.width << 1;
      temp.h = active.height;

      myRectList->add(&temp);
      SDL_FillRect(myScreen, &temp, myDefPalette[active.color]);
    }
#else
    for(uInt32 y = 0; y < height; ++y )
    {
      uInt32 pos = screenofsY;
      for(uInt32 x = 0; x < width; ++x )
      {
        const uInt32 bufofs = bufofsY + x;
        uInt8 v = currentFrame[bufofs];
        uInt8 w = previousFrame[bufofs];

        if(v != w || theRedrawTIAIndicator)
        {
          // If we ever get to this point, we know the current and previous
          // buffers differ.  In that case, make sure the changes are
          // are drawn in postFrameUpdate()
          myDirtyFlag = true;
          buffer[pos] = buffer[pos+1] = (uInt16) myDefPalette[v];
        }
        pos += 2;
      }
      bufofsY    += width;
      screenofsY += myPitch;
    }
#endif
  }
  else
  {
    // Phosphor mode always implies a dirty update,
    // so we don't care about theRedrawTIAIndicator
    myDirtyFlag = true;

    for(uInt32 y = 0; y < height; ++y )
    {
      uInt32 pos = screenofsY;
      for(uInt32 x = 0; x < width; ++x )
      {
        const uInt32 bufofs = bufofsY + x;
        uInt8 v = currentFrame[bufofs];
        uInt8 w = previousFrame[bufofs];

        buffer[pos++] = (uInt16) myAvgPalette[v][w];
        buffer[pos++] = (uInt16) myAvgPalette[v][w];
      }
      bufofsY    += width;
      screenofsY += myPitch;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::preFrameUpdate()
{
  // Start a new rectlist on each display update
  myRectList->start();

  // Add all previous overlay rects, then erase
  SDL_Rect* dirtyOverlayRects = myOverlayRectList->rects();
  for(unsigned int i = 0; i < myOverlayRectList->numRects(); ++i)
    myRectList->add(&dirtyOverlayRects[i]);
  myOverlayRectList->start();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::postFrameUpdate()
{
  if(myDirtyFlag)
  {
    SDL_Flip(myScreen);
    myRectList->start();
    myDirtyFlag = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::scanline(uInt32 row, uInt8* data)
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

    pixel = *(Uint16*) p;
    SDL_GetRGB(pixel, myScreen->format, &r, &g, &b);

    data[x * 3 + 0] = r;
    data[x * 3 + 1] = g;
    data[x * 3 + 2] = b;
  }

  SDL_UnlockSurface(myScreen);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::toggleFilter()
{
  // Not supported
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::hLine(uInt32 x, uInt32 y, uInt32 x2, int color)
{
  SDL_Rect tmp;

  // Horizontal line
  tmp.x = x;
  tmp.y = y;
  tmp.w = (x2 - x + 1);
  tmp.h = 1;
  SDL_FillRect(myScreen, &tmp, myDefPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::vLine(uInt32 x, uInt32 y, uInt32 y2, int color)
{
  SDL_Rect tmp;

  // Vertical line
  tmp.x = x;
  tmp.y = y;
  tmp.w = 1;
  tmp.h = (y2 - y + 1);
  SDL_FillRect(myScreen, &tmp, myDefPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                               int color)
{
  SDL_Rect tmp;

  // Fill the rectangle
  tmp.x = x;
  tmp.y = y;
  tmp.w = w;
  tmp.h = h;
  SDL_FillRect(myScreen, &tmp, myDefPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::drawChar(const GUI::Font* FONT, uInt8 chr,
                               uInt32 xorig, uInt32 yorig, int color)
{
  GUI::Font* font = (GUI::Font*)FONT;
  const FontDesc& desc = font->desc();

  // If this character is not included in the font, use the default char.
  if(chr < desc.firstchar || chr >= desc.firstchar + desc.size)
  {
    if (chr == ' ')
      return;
    chr = desc.defaultchar;
  }

  const Int32 w = font->getCharWidth(chr);
  const Int32 h = font->getFontHeight();
  chr -= desc.firstchar;
  const uInt16* tmp = desc.bits + (desc.offset ? desc.offset[chr] : (chr * h));

  SDL_Rect rect;
  for(int y = 0; y < h; y++)
  {
    const uInt16 buffer = *tmp++;
    uInt16 mask = 0x8000;

    for(int x = 0; x < w; x++, mask >>= 1)
    {
      if ((buffer & mask) != 0)
      {
        rect.x = x + xorig;
        rect.y = y + yorig;
        rect.w = rect.h = 1;
        SDL_FillRect(myScreen, &rect, myDefPalette[color]);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::drawBitmap(uInt32* bitmap, Int32 xorig, Int32 yorig,
                                 int color, Int32 h)
{
  SDL_Rect rect;
  for(int y = 0; y < h; y++)
  {
    uInt32 mask = 0xF0000000;

    for(int x = 0; x < 8; x++, mask >>= 4)
    {
      if(bitmap[y] & mask)
      {
        rect.x = x + xorig;
        rect.y = y + yorig;
        rect.w = rect.h = 1;
        SDL_FillRect(myScreen, &rect, myDefPalette[color]);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::translateCoords(Int32* x, Int32* y)
{
  // Coordinates don't change
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h)
{
  // We may entirely remove the RectList, since it might be faster to just
  // flip the screen (since it's a hardware buffer)
  myDirtyFlag = true;

  // Add a dirty rect to the overlay rectangle list
  // They will actually be added to the main rectlist in preFrameUpdate()
  // TODO - intelligent merging of rectangles, to avoid overlap
  SDL_Rect temp;
  temp.x = x;
  temp.y = y;
  temp.w = w;
  temp.h = h;

  myOverlayRectList->add(&temp);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::enablePhosphor(bool enable, int blend)
{
  myUsePhosphor   = enable;
  myPhosphorBlend = blend;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::cls()
{
  if(myScreen)
  {
    SDL_FillRect(myScreen, NULL, 0);
    SDL_UpdateRect(myScreen, 0, 0, 0, 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::showCursor(bool show)
{
  // Never show the cursor
  SDL_ShowCursor(SDL_DISABLE);
}
