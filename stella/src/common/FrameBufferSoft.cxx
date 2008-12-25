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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FrameBufferSoft.cxx,v 1.83 2008-12-25 23:05:16 stephena Exp $
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
    myRenderType(kSoftZoom_16),
    myTiaDirty(false),
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
bool FrameBufferSoft::initSubsystem(VideoMode& mode)
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
bool FrameBufferSoft::setVidMode(VideoMode& mode)
{
  // Make sure to clear the screen
  if(myScreen)
  {
    SDL_FillRect(myScreen, NULL, 0);
    SDL_UpdateRect(myScreen, 0, 0, 0, 0);
  }
  myScreen = SDL_SetVideoMode(mode.screen_w, mode.screen_h, 0, mySDLFlags);
  if(myScreen == NULL)
  {
    cerr << "ERROR: Unable to open SDL window: " << SDL_GetError() << endl;
    return false;
  }
  myFormat = myScreen->format;
  myBytesPerPixel = myFormat->BytesPerPixel;

  // If software mode can open the given screen, it will always be in the
  // requested format, or not at all; we only update mode when the screen
  // is successfully created
  mode.screen_w = myScreen->w;
  mode.screen_h = myScreen->h;
  myZoomLevel = mode.gfxmode.zoom;

// FIXME - look at gfxmode directly

  // Make sure drawMediaSource() knows which renderer to use
  stateChanged(myOSystem->eventHandler().state());
  myBaseOffset = mode.image_y * myPitch + mode.image_x;

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

            if(v != w || myRedrawEntireFrame)
            {
              while(xstride--)
              {
                buffer[pos++] = (uInt16) myDefPalette[v];
                buffer[pos++] = (uInt16) myDefPalette[v];
              }
              myTiaDirty = true;
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

            if(v != w || myRedrawEntireFrame)
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
              myTiaDirty = true;
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

            if(v != w || myRedrawEntireFrame)
            {
              while(xstride--)
              {
                buffer[pos++] = (uInt32) myDefPalette[v];
                buffer[pos++] = (uInt32) myDefPalette[v];
              }
              myTiaDirty = true;
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
      myTiaDirty = true;
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
      myTiaDirty = true;
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
      myTiaDirty = true;
      break;  // kPhosphor_32
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::postFrameUpdate()
{
  if(myTiaDirty && !myInUIMode)
  {
    SDL_Flip(myScreen);
    myTiaDirty = false;
  }
  else if(myRectList->numRects() > 0)
  {
    SDL_UpdateRects(myScreen, myRectList->numRects(), myRectList->rects());
  }
  myRectList->start();
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
      myPitch = myScreen->pitch >> 1;
      myRenderType = myUsePhosphor ? kPhosphor_16 : kSoftZoom_16;
      break;
    case 3:  // 24-bit
      myPitch = myScreen->pitch;
      myRenderType = myUsePhosphor ? kPhosphor_24 : kSoftZoom_24;
      break;
    case 4:  // 32-bit
      myPitch = myScreen->pitch >> 2;
      myRenderType = myUsePhosphor ? kPhosphor_32 : kSoftZoom_32;
      break;
  }

  // Have the changes take effect
  myOSystem->eventHandler().refreshDisplay();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurface* FrameBufferSoft::createSurface(int w, int h, bool isBase) const
{
  SDL_Surface* surface = isBase ? myScreen :
      SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, myFormat->BitsPerPixel,
                           myFormat->Rmask, myFormat->Gmask, myFormat->Bmask,
                           myFormat->Amask);

  return new FBSurfaceSoft(*this, surface, w, h, isBase);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::scanline(uInt32 row, uInt8* data) const
{
  // Make sure no pixels are being modified
  SDL_LockSurface(myScreen);

  uInt32 pixel = 0;
  uInt8 *p, r, g, b;

  // Row will be offset by the amount the actual image is shifted down
  const GUI::Rect& image = imageRect();
  row += image.y();
  for(Int32 x = 0; x < myScreen->w; ++x)
  {
    p = (Uint8*) ((uInt8*)myScreen->pixels +              // Start at top of RAM
                 (row * myScreen->pitch) +                // Go down 'row' lines
                 ((x + image.x()) * myBytesPerPixel));    // Go in 'x' pixels

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
//  FBSurfaceSoft implementation follows ...
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceSoft::FBSurfaceSoft(const FrameBufferSoft& buffer, SDL_Surface* surface,
                             uInt32 w, uInt32 h, bool isBase)
  : myFB(buffer),
    mySurface(surface),
    myWidth(w),
    myHeight(h),
    myIsBaseSurface(isBase),
    mySurfaceIsDirty(false),
    myBaseOffset(0),
    myPitch(0),
    myXOrig(0),
    myYOrig(0),
    myXOffset(0),
    myYOffset(0)
{
  recalc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceSoft::~FBSurfaceSoft()
{
  if(!myIsBaseSurface)
    SDL_FreeSurface(mySurface);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSoft::hLine(uInt32 x, uInt32 y, uInt32 x2, int color)
{
  // Horizontal line
  SDL_Rect tmp;
  tmp.x = x + myXOffset;
  tmp.y = y + myYOffset;
  tmp.w = x2 - x + 1;
  tmp.h = 1;
  SDL_FillRect(mySurface, &tmp, myFB.myDefPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSoft::vLine(uInt32 x, uInt32 y, uInt32 y2, int color)
{
  // Vertical line
  SDL_Rect tmp;
  tmp.x = x + myXOffset;
  tmp.y = y + myYOffset;
  tmp.w = 1;
  tmp.h = y2 - y + 1;
  SDL_FillRect(mySurface, &tmp, myFB.myDefPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSoft::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, int color)
{
  // Fill the rectangle
  SDL_Rect tmp;
  tmp.x = x + myXOffset;
  tmp.y = y + myYOffset;
  tmp.w = w;
  tmp.h = h;
  SDL_FillRect(mySurface, &tmp, myFB.myDefPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSoft::drawChar(const GUI::Font* font, uInt8 chr,
                             uInt32 tx, uInt32 ty, int color)
{
#if 0
  const FontDesc& desc = font->desc();

  // If this character is not included in the font, use the default char.
  if(chr < desc.firstchar || chr >= desc.firstchar + desc.size)
  {
    if (chr == ' ') return;
    chr = desc.defaultchar;
  }

  const Int32 w = font->getCharWidth(chr);
  const Int32 h = font->getFontHeight();
  chr -= desc.firstchar;
  const uInt32* tmp = desc.bits + (desc.offset ? desc.offset[chr] : (chr * h));

  SDL_LockSurface(mySurface);

  switch(myFB.myBytesPerPixel)
  {
    case 2:
    {
      // Get buffer position where upper-left pixel of the character will be drawn
      uInt16* buffer = (uInt16*) mySurface->pixels + myBaseOffset + ty * myPitch + tx;
      for(int y = 0; y < h; ++y)
      {
        const uInt32 ptr = *tmp++;
        if(ptr)
        {
          uInt32 mask = 0x80000000;
          for(int x = 0; x < w; ++x, mask >>= 1)
            if(ptr & mask)
              buffer[x] = (uInt16) myFB.myDefPalette[color];
        }
        buffer += myFB.myPitch;
      }
      break;
    }
    case 3:
    {
#if 0
      // Get buffer position where upper-left pixel of the character will be drawn
      uInt8* buffer = (uInt8*) surface->pixels + myBaseOffset + yorig * myPitch + xorig;
      uInt32 pixel = myDefPalette[color];
      uInt8 r = (pixel & myFormat->Rmask) >> myFormat->Rshift;
      uInt8 g = (pixel & myFormat->Gmask) >> myFormat->Gshift;
      uInt8 b = (pixel & myFormat->Bmask) >> myFormat->Bshift;

      for(int y = h; y; --y)
      {
        const uInt32 fontbuf = *tmp++;
        int ystride = myZoomLevel;
        while(ystride--)
        {
          if(fontbuf)
          {
            uInt32 mask = 0x80000000;
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
          }
          screenofsY += myPitch;
        }
      }
#endif
      break;
    }
    case 4:
    {
      // Get buffer position where upper-left pixel of the character will be drawn
      uInt32* buffer = (uInt32*) mySurface->pixels + myBaseOffset + ty * myPitch + tx;
      for(int y = 0; y < h; ++y)
      {
        const uInt32 ptr = *tmp++;
        if(ptr)
        {
          uInt32 mask = 0x80000000;
          for(int x = 0; x < w; ++x, mask >>= 1)
            if(ptr & mask)
              buffer[x] = (uInt32) myFB.myDefPalette[color];
        }
        buffer += myPitch;
      }
      break;
    }
    default:
      break;
  }
  SDL_UnlockSurface(mySurface);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSoft::drawBitmap(uInt32* bitmap, Int32 tx, Int32 ty,
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
        rect.x = x + tx + myXOffset;
        rect.y = y + ty + myYOffset;
        rect.w = rect.h = 1;
        SDL_FillRect(mySurface, &rect, myFB.myDefPalette[color]);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSoft::addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h)
{
  // Base surfaces use dirty-rectangle updates, since they can be quite
  // large, and updating the entire surface each frame would be too slow
  // Non-base surfaces are usually smaller, and can be updated entirely
  if(myIsBaseSurface)
  {
    // Add a dirty rect to the UI rectangle list
    // TODO - intelligent merging of rectangles, to avoid overlap
    SDL_Rect temp;
    temp.x = x + myXOrig;  temp.y = y + myYOrig;  temp.w = w;  temp.h = h;
    myFB.myRectList->add(&temp);
  }
  else
  {
    SDL_Rect temp;
    temp.x = myXOrig;  temp.y = myYOrig;  temp.w = myWidth;  temp.h = myHeight;
    myFB.myRectList->add(&temp);

    // Indicate that at least one dirty rect has been added
    // This is an optimization for the update() method
    mySurfaceIsDirty = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSoft::getPos(uInt32& x, uInt32& y) const
{
  // Return the origin of the 'usable' area of a surface
  if(myIsBaseSurface)
  {
    x = myXOffset;
    y = myYOffset;
  }
  else
  {
    x = myXOrig;
    y = myYOrig;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSoft::setPos(uInt32 x, uInt32 y)
{
  // Make sure pitch is valid
  recalc();

  myXOrig = x;
  myYOrig = y;

  if(myIsBaseSurface)
  {
    const GUI::Rect& image = myFB.imageRect();
    myXOffset = image.x();
    myYOffset = image.y();
    myBaseOffset = myYOffset * myPitch + myXOffset;
  }
  else
  {
    myXOffset = myYOffset = myBaseOffset = 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSoft::setWidth(uInt32 w)
{
  myWidth = w;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSoft::setHeight(uInt32 h)
{
  myHeight = h;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSoft::translateCoords(Int32& x, Int32& y) const
{
  x -= myXOrig;
  y -= myYOrig;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSoft::update()
{
  // Since this method is called each frame, we only blit the surfaces when
  // absolutely necessary
  if(mySurfaceIsDirty /* && !myIsBaseSurface */)
  {
    SDL_Rect srcrect;
    srcrect.x = 0;
    srcrect.y = 0;
    srcrect.w = myWidth;
    srcrect.h = myHeight;

    SDL_Rect dstrect;
    dstrect.x = myXOrig;
    dstrect.y = myYOrig;
    dstrect.w = myWidth;
    dstrect.h = myHeight;

    SDL_BlitSurface(mySurface, &srcrect, myFB.myScreen, &dstrect);
    mySurfaceIsDirty = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSoft::recalc()
{
  switch(mySurface->format->BytesPerPixel)
  {
    case 2:  // 16-bit
      myPitch = mySurface->pitch/2;
      break;
    case 3:  // 24-bit
      myPitch = mySurface->pitch;
      break;
    case 4:  // 32-bit
      myPitch = mySurface->pitch/4;
      break;
  }
}


#if 0
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::drawSurface(const GUI::Surface* surface, Int32 x, Int32 y)
{
  SDL_Rect dstrect;
  dstrect.x = x * myZoomLevel + myImageDim.x;
  dstrect.y = y * myZoomLevel + myImageDim.y;
  SDL_Rect srcrect;
  srcrect.x = 0;
  srcrect.y = 0;
  srcrect.w = surface->myClipWidth * myZoomLevel;
  srcrect.h = surface->myClipHeight * myZoomLevel;

  SDL_BlitSurface(surface->myData, &srcrect, myScreen, &dstrect);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::bytesToSurface(GUI::Surface* surface, int row,
                                     uInt8* data, int rowbytes) const
{
  // Calculate a scanline of zoomed surface data
  SDL_Surface* s = surface->myData;
  SDL_Rect rect;
  rect.x = 0;
  rect.y = row * myZoomLevel;
  for(int c = 0; c < rowbytes; c += 3)
  {
    uInt32 pixel = SDL_MapRGB(s->format, data[c], data[c+1], data[c+2]);
    rect.x += myZoomLevel;
    rect.w = rect.h = myZoomLevel;
    SDL_FillRect(surface->myData, &rect, pixel);
  }
}
#endif
