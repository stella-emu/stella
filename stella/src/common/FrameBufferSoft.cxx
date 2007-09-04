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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FrameBufferSoft.cxx,v 1.74 2007-09-04 00:47:00 stephena Exp $
//============================================================================

#include <sstream>
#include <SDL.h>

#include "bspf.hxx"

#include "Console.hxx"
#include "Font.hxx"
#include "MediaSrc.hxx"
#include "OSystem.hxx"
#include "RectList.hxx"
#include "Settings.hxx"
#include "Surface.hxx"

#include "FrameBufferSoft.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferSoft::FrameBufferSoft(OSystem* osystem)
  : FrameBuffer(osystem),
    myZoomLevel(1),
    myRenderType(kSoftZoom_16),
    myDirtyFlag(false),
    myInUIMode(false),
    myRectList(NULL)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferSoft::~FrameBufferSoft()
{
  delete myRectList;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferSoft::initSubsystem(VideoMode mode)
{
  // Set up the rectangle list to be used in the dirty update
  delete myRectList;
  myRectList = new RectList();

  if(!myRectList)
  {
    cerr << "ERROR: Unable to get memory for SDL rects" << endl;
    return false;
  }

  // Create the screen
  return setVidMode(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FrameBufferSoft::about() const
{
  ostringstream buf;

  buf << "Video rendering: Software mode" << endl
      << "  Color: " << (int)myFormat->BitsPerPixel << " bit" << endl
      << "  Rmask = " << hex << setw(4) << (int)myFormat->Rmask
      << ", Rshift = "<< dec << setw(2) << (int)myFormat->Rshift
      << ", Rloss = " << dec << setw(2) << (int)myFormat->Rloss << endl
      << "  Gmask = " << hex << setw(4) << (int)myFormat->Gmask
      << ", Gshift = "<< dec << setw(2) << (int)myFormat->Gshift
      << ", Gloss = " << dec << setw(2) << (int)myFormat->Gloss << endl
      << "  Bmask = " << hex << setw(4) << (int)myFormat->Bmask
      << ", Bshift = "<< dec << setw(2) << (int)myFormat->Bshift
      << ", Bloss = " << dec << setw(2) << (int)myFormat->Bloss << endl;

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferSoft::setVidMode(VideoMode mode)
{
  myScreenDim.x = myScreenDim.y = 0;
  myScreenDim.w = mode.screen_w;
  myScreenDim.h = mode.screen_h;

  myImageDim.x = mode.image_x;
  myImageDim.y = mode.image_y;
  myImageDim.w = mode.image_w;
  myImageDim.h = mode.image_h;

  myZoomLevel = mode.zoom;

  // Make sure to clear the screen
  if(myScreen)
  {
    SDL_FillRect(myScreen, NULL, 0);
    SDL_UpdateRect(myScreen, 0, 0, 0, 0);
  }
  myScreen = SDL_SetVideoMode(myScreenDim.w, myScreenDim.h, 0, mySDLFlags);
  if(myScreen == NULL)
  {
    cerr << "ERROR: Unable to open SDL window: " << SDL_GetError() << endl;
    return false;
  }
  myFormat = myScreen->format;
  myBytesPerPixel = myFormat->BytesPerPixel;

  // Make sure drawMediaSource() knows which renderer to use
  stateChanged(myOSystem->eventHandler().state());
  myBaseOffset = myImageDim.y * myPitch + myImageDim.x;

  // Erase old rects, since they've probably been scaled for
  // a different sized screen
  myRectList->start();

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::drawMediaSource()
{
  MediaSource& mediasrc = myOSystem->console().mediaSource();

  uInt8* currentFrame   = mediasrc.currentFrameBuffer();
  uInt8* previousFrame  = mediasrc.previousFrameBuffer();

  uInt32 width  = mediasrc.width();
  uInt32 height = mediasrc.height();

  switch(myRenderType)
  {
    case kSoftZoom_16:
    {
      SDL_LockSurface(myScreen);
      uInt16* buffer    = (uInt16*)myScreen->pixels + myBaseOffset;
      uInt32 bufofsY    = 0;
      uInt32 screenofsY = 0;
      for(uInt32 y = 0; y < height; ++y)
      {
        uInt32 ystride = myZoomLevel;
        while(ystride--)
        {
          uInt32 pos = screenofsY;
          for(uInt32 x = 0; x < width; ++x)
          {
            const uInt32 bufofs = bufofsY + x;
            uInt32 xstride = myZoomLevel;

            uInt8 v = currentFrame[bufofs];
            uInt8 w = previousFrame[bufofs];

            if(v != w || theRedrawTIAIndicator)
            {
              while(xstride--)
              {
                buffer[pos++] = (uInt16) myDefPalette[v];
                buffer[pos++] = (uInt16) myDefPalette[v];
              }
              myDirtyFlag = true;
            }
            else
              pos += xstride + xstride;
          }
          screenofsY += myPitch;
        }
        bufofsY += width;
      }
      SDL_UnlockSurface(myScreen);
      break;  // kSoftZoom_16
    }

    case kSoftZoom_24:
    {
      SDL_LockSurface(myScreen);
      uInt8* buffer     = (uInt8*)myScreen->pixels + myBaseOffset;
      uInt32 bufofsY    = 0;
      uInt32 screenofsY = 0;
      for(uInt32 y = 0; y < height; ++y)
      {
        uInt32 ystride = myZoomLevel;
        while(ystride--)
        {
          uInt32 pos = screenofsY;
          for(uInt32 x = 0; x < width; ++x)
          {
            const uInt32 bufofs = bufofsY + x;
            uInt32 xstride = myZoomLevel;

            uInt8 v = currentFrame[bufofs];
            uInt8 w = previousFrame[bufofs];

            if(v != w || theRedrawTIAIndicator)
            {
              uInt32 pixel = myDefPalette[v];
              uInt8 r = (pixel & myFormat->Rmask) >> myFormat->Rshift;
              uInt8 g = (pixel & myFormat->Gmask) >> myFormat->Gshift;
              uInt8 b = (pixel & myFormat->Bmask) >> myFormat->Bshift;

              while(xstride--)
              {
                buffer[pos++] = r;  buffer[pos++] = g;  buffer[pos++] = b;
                buffer[pos++] = r;  buffer[pos++] = g;  buffer[pos++] = b;
              }
              myDirtyFlag = true;
            }
            else  // try to eliminate multply whereever possible
              pos += xstride + xstride + xstride + xstride + xstride + xstride;
          }
          screenofsY += myPitch;
        }
        bufofsY += width;
      }
      SDL_UnlockSurface(myScreen);
      break;  // kSoftZoom_24
    }

    case kSoftZoom_32:
    {
      SDL_LockSurface(myScreen);
      uInt32* buffer    = (uInt32*)myScreen->pixels + myBaseOffset;
      uInt32 bufofsY    = 0;
      uInt32 screenofsY = 0;
      for(uInt32 y = 0; y < height; ++y)
      {
        uInt32 ystride = myZoomLevel;
        while(ystride--)
        {
          uInt32 pos = screenofsY;
          for(uInt32 x = 0; x < width; ++x)
          {
            const uInt32 bufofs = bufofsY + x;
            uInt32 xstride = myZoomLevel;

            uInt8 v = currentFrame[bufofs];
            uInt8 w = previousFrame[bufofs];

            if(v != w || theRedrawTIAIndicator)
            {
              while(xstride--)
              {
                buffer[pos++] = (uInt32) myDefPalette[v];
                buffer[pos++] = (uInt32) myDefPalette[v];
              }
              myDirtyFlag = true;
            }
            else
              pos += xstride + xstride;
          }
          screenofsY += myPitch;
        }
        bufofsY += width;
      }
      SDL_UnlockSurface(myScreen);
      break;  // kSoftZoom_32
    }

    case kPhosphor_16:
    {
      SDL_LockSurface(myScreen);
      uInt16* buffer    = (uInt16*)myScreen->pixels + myBaseOffset;
      uInt32 bufofsY    = 0;
      uInt32 screenofsY = 0;
      for(uInt32 y = 0; y < height; ++y)
      {
        uInt32 ystride = myZoomLevel;
        while(ystride--)
        {
          uInt32 pos = screenofsY;
          for(uInt32 x = 0; x < width; ++x)
          {
            const uInt32 bufofs = bufofsY + x;
            uInt32 xstride = myZoomLevel;

            uInt8 v = currentFrame[bufofs];
            uInt8 w = previousFrame[bufofs];

            while(xstride--)
            {
              buffer[pos++] = (uInt16) myAvgPalette[v][w];
              buffer[pos++] = (uInt16) myAvgPalette[v][w];
            }
          }
          screenofsY += myPitch;
        }
        bufofsY += width;
      }
      SDL_UnlockSurface(myScreen);
      myDirtyFlag = true;
      break;  // kPhosphor_16
    }

    case kPhosphor_24:
    {
      SDL_LockSurface(myScreen);
      uInt8* buffer     = (uInt8*)myScreen->pixels + myBaseOffset;
      uInt32 bufofsY    = 0;
      uInt32 screenofsY = 0;
      for(uInt32 y = 0; y < height; ++y)
      {
        uInt32 ystride = myZoomLevel;
        while(ystride--)
        {
          uInt32 pos = screenofsY;
          for(uInt32 x = 0; x < width; ++x)
          {
            const uInt32 bufofs = bufofsY + x;
            uInt32 xstride = myZoomLevel;

            uInt8 v = currentFrame[bufofs];
            uInt8 w = previousFrame[bufofs];
            uInt32 pixel = myAvgPalette[v][w];
            uInt8 r = (pixel & myFormat->Rmask) >> myFormat->Rshift;
            uInt8 g = (pixel & myFormat->Gmask) >> myFormat->Gshift;
            uInt8 b = (pixel & myFormat->Bmask) >> myFormat->Bshift;

            while(xstride--)
            {
              buffer[pos++] = r;  buffer[pos++] = g;  buffer[pos++] = b;
              buffer[pos++] = r;  buffer[pos++] = g;  buffer[pos++] = b;
            }
          }
          screenofsY += myPitch;
        }
        bufofsY += width;
      }
      SDL_UnlockSurface(myScreen);
      myDirtyFlag = true;
      break;  // kPhosphor_24
    }

    case kPhosphor_32:
    {
      SDL_LockSurface(myScreen);
      uInt32* buffer    = (uInt32*)myScreen->pixels + myBaseOffset;
      uInt32 bufofsY    = 0;
      uInt32 screenofsY = 0;
      for(uInt32 y = 0; y < height; ++y)
      {
        uInt32 ystride = myZoomLevel;
        while(ystride--)
        {
          uInt32 pos = screenofsY;
          for(uInt32 x = 0; x < width; ++x)
          {
            const uInt32 bufofs = bufofsY + x;
            uInt32 xstride = myZoomLevel;

            uInt8 v = currentFrame[bufofs];
            uInt8 w = previousFrame[bufofs];

            while(xstride--)
            {
              buffer[pos++] = (uInt32) myAvgPalette[v][w];
              buffer[pos++] = (uInt32) myAvgPalette[v][w];
            }
          }
          screenofsY += myPitch;
        }
        bufofsY += width;
      }
      SDL_UnlockSurface(myScreen);
      myDirtyFlag = true;
      break;  // kPhosphor_32
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::preFrameUpdate()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::postFrameUpdate()
{
/*
cerr << "FrameBufferSoft::postFrameUpdate()" << endl
	<< "  myInUIMode:             " << myInUIMode << endl
	<< "  myRectList->numRects(): " << myRectList->numRects() << endl
	<< "  myDirtyFlag:            " << myDirtyFlag << endl
	<< endl;
*/
  if(myInUIMode && myRectList->numRects() > 0)
  {
    SDL_UpdateRects(myScreen, myRectList->numRects(), myRectList->rects());
  }
  else if(myDirtyFlag || myRectList->numRects() > 0)
  {
    SDL_Flip(myScreen);
    myDirtyFlag = false;
  }
  myRectList->start();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::scanline(uInt32 row, uInt8* data) const
{
  // Make sure no pixels are being modified
  SDL_LockSurface(myScreen);

  uInt32 pixel = 0;
  uInt8 *p, r, g, b;

  // Row will be offset by the amount the actual image is shifted down
  row += myImageDim.y;
  for(Int32 x = 0; x < myScreen->w; ++x)
  {
    p = (Uint8*) ((uInt8*)myScreen->pixels +              // Start at top of RAM
                 (row * myScreen->pitch) +                // Go down 'row' lines
                 ((x + myImageDim.x) * myBytesPerPixel)); // Go in 'x' pixels

    switch(myBytesPerPixel)
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
void FrameBufferSoft::toggleFilter()
{
  // No filter added yet ...
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::hLine(uInt32 x, uInt32 y, uInt32 x2, int color)
{
  SDL_Rect tmp;

  // Horizontal line
  tmp.x = myImageDim.x + x * myZoomLevel;
  tmp.y = myImageDim.y + y * myZoomLevel;
  tmp.w = (x2 - x + 1) * myZoomLevel;
  tmp.h = myZoomLevel;
  SDL_FillRect(myScreen, &tmp, myDefPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::vLine(uInt32 x, uInt32 y, uInt32 y2, int color)
{
  SDL_Rect tmp;

  // Vertical line
  tmp.x = myImageDim.x + x * myZoomLevel;
  tmp.y = myImageDim.y + y * myZoomLevel;
  tmp.w = myZoomLevel;
  tmp.h = (y2 - y + 1) * myZoomLevel;
  SDL_FillRect(myScreen, &tmp, myDefPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, int color)
{
  SDL_Rect tmp;

  // Fill the rectangle
  tmp.x = myImageDim.x + x * myZoomLevel;
  tmp.y = myImageDim.y + y * myZoomLevel;
  tmp.w = w * myZoomLevel;
  tmp.h = h * myZoomLevel;
  SDL_FillRect(myScreen, &tmp, myDefPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::drawChar(const GUI::Font* font, uInt8 chr,
                               uInt32 xorig, uInt32 yorig, int color)
{
  // If this character is not included in the font, use the default char.
  const FontDesc& desc = font->desc();
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

  // Scale the origins to the current zoom
  xorig *= myZoomLevel;
  yorig *= myZoomLevel;

  SDL_LockSurface(myScreen);

  int screenofsY = 0;
  switch(myBytesPerPixel)
  {
    case 2:
    {
      // Get buffer position where upper-left pixel of the character will be drawn
      uInt16* buffer = (uInt16*) myScreen->pixels + myBaseOffset + yorig * myPitch + xorig;
      for(int y = h; y; --y)
      {
        const uInt16 fontbuf = *tmp++;
        int ystride = myZoomLevel;
        while(ystride--)
        {
          uInt16 mask = 0x8000;
          int pos = screenofsY;
          for(int x = 0; x < w; x++, mask >>= 1)
          {
            int xstride = myZoomLevel;
            if((fontbuf & mask) != 0)
              while(xstride--)
                buffer[pos++] = myDefPalette[color];
            else
              pos += xstride;
          }
          screenofsY += myPitch;
        }
      }
      break;
    }
    case 3:
    {
      // Get buffer position where upper-left pixel of the character will be drawn
      uInt8* buffer = (uInt8*) myScreen->pixels + myBaseOffset + yorig * myPitch + xorig;
      uInt32 pixel = myDefPalette[color];
      uInt8 r = (pixel & myFormat->Rmask) >> myFormat->Rshift;
      uInt8 g = (pixel & myFormat->Gmask) >> myFormat->Gshift;
      uInt8 b = (pixel & myFormat->Bmask) >> myFormat->Bshift;

      for(int y = h; y; --y)
      {
        const uInt16 fontbuf = *tmp++;
        int ystride = myZoomLevel;
        while(ystride--)
        {
          uInt16 mask = 0x8000;
          int pos = screenofsY;
          for(int x = 0; x < w; x++, mask >>= 1)
          {
            int xstride = myZoomLevel;
            if((fontbuf & mask) != 0)
            {
              while(xstride--)
              {
                buffer[pos++] = r;  buffer[pos++] = g;  buffer[pos++] = b;
              }
            }
            else
              pos += xstride + xstride + xstride;
          }
          screenofsY += myPitch;
        }
      }
      break;
    }
    case 4:
    {
      // Get buffer position where upper-left pixel of the character will be drawn
      uInt32* buffer = (uInt32*) myScreen->pixels + myBaseOffset + yorig * myPitch + xorig;
      for(int y = h; y; --y)
      {
        const uInt16 fontbuf = *tmp++;
        int ystride = myZoomLevel;
        while(ystride--)
        {
          uInt16 mask = 0x8000;
          int pos = screenofsY;
          for(int x = 0; x < w; x++, mask >>= 1)
          {
            int xstride = myZoomLevel;
            if((fontbuf & mask) != 0)
              while(xstride--)
                buffer[pos++] = myDefPalette[color];
            else
              pos += xstride;
          }
          screenofsY += myPitch;
        }
      }
      break;
    }
    default:
      break;
  }
  SDL_UnlockSurface(myScreen);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::drawBitmap(uInt32* bitmap, Int32 xorig, Int32 yorig,
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
        rect.x = myImageDim.x + (x + xorig) * myZoomLevel;
        rect.y = myImageDim.y + (y + yorig) * myZoomLevel;
        rect.w = rect.h = myZoomLevel;
        SDL_FillRect(myScreen, &rect, myDefPalette[color]);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::drawSurface(const GUI::Surface* surface, Int32 x, Int32 y)
{
  SDL_Rect clip;
  clip.x = x * myZoomLevel + myImageDim.x;
  clip.y = y * myZoomLevel + myImageDim.y;
  
  SDL_BlitSurface(surface->myData, 0, myScreen, &clip);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::bytesToSurface(GUI::Surface* surface, int row,
                                     uInt8* data) const
{
  SDL_Surface* s = surface->myData;
  int rowbytes = s->w * 3;
  row *= myZoomLevel;

  switch(myBytesPerPixel)
  {
    case 2:
    {
      uInt16* pixels = (uInt16*) s->pixels;
      int surfbytes = s->pitch/2;
      pixels += (row * surfbytes);
      uInt8* pixel_ptr = (uInt8*)pixels;

      // Calculate a scanline of zoomed surface data
      for(int c = 0; c < rowbytes/myZoomLevel; c += 3)
      {
        uInt32 pixel = SDL_MapRGB(s->format, data[c], data[c+1], data[c+2]);
        uInt32 xstride = myZoomLevel;
        while(xstride--)
          *pixels++ = pixel;
      }

      // Now duplicate the scanlines (we've already done the first one)
      uInt32 ystride = myZoomLevel-1;
      while(ystride--)
      {
        memcpy(pixel_ptr + s->pitch, pixel_ptr, s->pitch);
        pixel_ptr += s->pitch;
      }
      break;
    }

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::translateCoords(Int32& x, Int32& y) const
{
  x = (x - myImageDim.x) / myZoomLevel;
  y = (y - myImageDim.y) / myZoomLevel;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h)
{
  // Add a dirty rect to the UI rectangle list
  // TODO - intelligent merging of rectangles, to avoid overlap
  SDL_Rect temp;
  temp.x = myImageDim.x + x * myZoomLevel;
  temp.y = myImageDim.y + y * myZoomLevel;
  temp.w = w * myZoomLevel;
  temp.h = h * myZoomLevel;

  myRectList->add(&temp);

//  cerr << "addDirtyRect():  "
//       << "x=" << temp.x << ", y=" << temp.y << ", w=" << temp.w << ", h=" << temp.h << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::enablePhosphor(bool enable, int blend)
{
  myUsePhosphor   = enable;
  myPhosphorBlend = blend;

  stateChanged(myOSystem->eventHandler().state());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::stateChanged(EventHandler::State state)
{
  if(!myScreen)
    return;

  myInUIMode = (state == EventHandler::S_LAUNCHER ||
                state == EventHandler::S_DEBUGGER);

  // Make sure drawMediaSource() knows which renderer to use
  switch(myBytesPerPixel)
  {
    case 2:  // 16-bit
      myPitch = myScreen->pitch/2;
      if(myUsePhosphor)
        myRenderType = kPhosphor_16;
      else
        myRenderType = kSoftZoom_16;
      break;
    case 3:  // 24-bit
      myPitch = myScreen->pitch;
      if(myUsePhosphor)
        myRenderType = kPhosphor_24;
      else
        myRenderType = kSoftZoom_24;
      break;
    case 4:  // 32-bit
      myPitch = myScreen->pitch/4;
      if(myUsePhosphor)
        myRenderType = kPhosphor_32;
      else
        myRenderType = kSoftZoom_32;
      break;
    default:
      myRenderType = kSoftZoom_16; // What else should we do here?
      break;
  }

  // Have the changes take effect
  myOSystem->eventHandler().refreshDisplay();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GUI::Surface* FrameBufferSoft::createSurface(int width, int height) const
{
  SDL_Surface* data =
    SDL_CreateRGBSurface(SDL_SWSURFACE, width*myZoomLevel, height*myZoomLevel,
                         myBytesPerPixel << 3, myFormat->Rmask, myFormat->Gmask,
                         myFormat->Bmask, myFormat->Amask);

  return data ? new GUI::Surface(width, height, data) : NULL;
}
