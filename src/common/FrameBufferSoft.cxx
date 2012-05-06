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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <sstream>
#include <SDL.h>

#include "bspf.hxx"

#include "Console.hxx"
#include "Font.hxx"
#include "OSystem.hxx"
#include "RectList.hxx"
#include "Settings.hxx"
#include "TIA.hxx"

#include "FrameBufferSoft.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferSoft::FrameBufferSoft(OSystem* osystem)
  : FrameBuffer(osystem),
    myZoomLevel(2),
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
    myOSystem->logMessage("ERROR: Unable to get memory for SDL rects\n", 0);
    return false;
  }

  // Create the screen
  return setVidMode(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FrameBufferSoft::about() const
{
  ostringstream buf;

  buf << "Video rendering: Software mode" << endl << setfill('0') 
      << "  Color: " << (int)myFormat->BitsPerPixel << " bit" << endl
      << "  Rmask = " << hex << setw(8) << (int)myFormat->Rmask
      << ", Rshift = "<< dec << setw(2) << (int)myFormat->Rshift
      << ", Rloss = " << dec << setw(2) << (int)myFormat->Rloss << endl
      << "  Gmask = " << hex << setw(8) << (int)myFormat->Gmask
      << ", Gshift = "<< dec << setw(2) << (int)myFormat->Gshift
      << ", Gloss = " << dec << setw(2) << (int)myFormat->Gloss << endl
      << "  Bmask = " << hex << setw(8) << (int)myFormat->Bmask
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
    ostringstream buf;
    buf << "ERROR: Unable to open SDL window: " << SDL_GetError() << endl;
    myOSystem->logMessage(buf.str(), 0);
    return false;
  }
  myFormat = myScreen->format;
  myBytesPerPixel = myFormat->BytesPerPixel;

  // Make sure the flags represent the current screen state
  mySDLFlags = myScreen->flags;

  // Make sure drawTIA() knows which renderer to use
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
  myBaseOffset = mode.image_y * myPitch + mode.image_x;

  // If software mode can open the given screen, it will always be in the
  // requested format, or not at all; we only update mode when the screen
  // is successfully created
  mode.screen_w = myScreen->w;
  mode.screen_h = myScreen->h;
  myZoomLevel = mode.gfxmode.zoom;
// FIXME - look at gfxmode directly

  // Erase old rects, since they've probably been scaled for
  // a different sized screen
  myRectList->start();

  // Any previously allocated surfaces have probably changed as well,
  // so we should refresh them
  resetSurfaces();

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::invalidate()
{
  if(myScreen)
    SDL_FillRect(myScreen, NULL, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::drawTIA(bool fullRedraw)
{
  const TIA& tia = myOSystem->console().tia();

  uInt8* currentFrame   = tia.currentFrameBuffer();
  uInt8* previousFrame  = tia.previousFrameBuffer();

  uInt32 width  = tia.width();
  uInt32 height = tia.height();

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

            if(v != w || fullRedraw)
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

            if(v != w || fullRedraw)
            {
              uInt8 a = myDefPalette24[v][0],
                    b = myDefPalette24[v][1],
                    c = myDefPalette24[v][2];

              while(xstride--)
              {
                buffer[pos++] = a;  buffer[pos++] = b;  buffer[pos++] = c;
                buffer[pos++] = a;  buffer[pos++] = b;  buffer[pos++] = c;
              }
              myTiaDirty = true;
            }
            else  // try to eliminate multiply whereever possible
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

            if(v != w || fullRedraw)
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
            uInt8 a, b, c;
            uInt32 pixel = myAvgPalette[v][w];
            if(SDL_BYTEORDER == SDL_LIL_ENDIAN)
            {
              a = (pixel & myFormat->Bmask) >> myFormat->Bshift;
              b = (pixel & myFormat->Gmask) >> myFormat->Gshift;
              c = (pixel & myFormat->Rmask) >> myFormat->Rshift;
            }
            else
            {
              a = (pixel & myFormat->Rmask) >> myFormat->Rshift;
              b = (pixel & myFormat->Gmask) >> myFormat->Gshift;
              c = (pixel & myFormat->Bmask) >> myFormat->Bshift;
            }

            while(xstride--)
            {
              buffer[pos++] = a;  buffer[pos++] = b;  buffer[pos++] = c;
              buffer[pos++] = a;  buffer[pos++] = b;  buffer[pos++] = c;
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
    SDL_UpdateRect(myScreen, 0, 0, 0, 0);
    myTiaDirty = false;
  }
  else if(myRectList->numRects() > 0)
  {
//myRectList->print(myScreen->w, myScreen->h);
    SDL_UpdateRects(myScreen, myRectList->numRects(), myRectList->rects());
  }
  myRectList->start();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSoft::enablePhosphor(bool enable, int blend)
{
  myUsePhosphor   = enable;
  myPhosphorBlend = blend;

  // Make sure drawMediaSource() knows which renderer to use
  switch(myBytesPerPixel)
  {
    case 2:  // 16-bit
      myRenderType = myUsePhosphor ? kPhosphor_16 : kSoftZoom_16;
      break;
    case 3:  // 24-bit
      myRenderType = myUsePhosphor ? kPhosphor_24 : kSoftZoom_24;
      break;
    case 4:  // 32-bit
      myRenderType = myUsePhosphor ? kPhosphor_32 : kSoftZoom_32;
      break;
  }
  myRedrawEntireFrame = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurface* FrameBufferSoft::createSurface(int w, int h, bool isBase) const
{
  // For some unknown reason, OSX in software fullscreen mode doesn't like
  // to use the underlying surface directly
  // I suspect it's an SDL compatibility thing, since I get errors
  // referencing Quartz vs. QuickDraw, and then a program crash
  // For now, we'll just always use entire surfaces for OSX
  // I don't think this will have much effect, since OpenGL mode is the
  // preferred method in OSX (basically, all OSX installations have OpenGL
  // support)
#ifdef MAC_OSX
  isBase = false;
#endif

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
    myPitch(0),
    myXOrig(0),
    myYOrig(0),
    myXOffset(0),
    myYOffset(0)
{
  reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceSoft::~FBSurfaceSoft()
{
  if(!myIsBaseSurface)
    SDL_FreeSurface(mySurface);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSoft::hLine(uInt32 x, uInt32 y, uInt32 x2, uInt32 color)
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
void FBSurfaceSoft::vLine(uInt32 x, uInt32 y, uInt32 y2, uInt32 color)
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
void FBSurfaceSoft::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, uInt32 color)
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
void FBSurfaceSoft::drawChar(const GUI::Font& font, uInt8 chr,
                             uInt32 tx, uInt32 ty, uInt32 color)
{
  const FontDesc& desc = font.desc();

  // If this character is not included in the font, use the default char.
  if(chr < desc.firstchar || chr >= desc.firstchar + desc.size)
  {
    if (chr == ' ') return;
    chr = desc.defaultchar;
  }
  chr -= desc.firstchar;
 
  // Get the bounding box of the character
  int bbw, bbh, bbx, bby;
  if(!desc.bbx)
  {
    bbw = desc.fbbw;
    bbh = desc.fbbh;
    bbx = desc.fbbx;
    bby = desc.fbby;
  }
  else
  {
    bbw = desc.bbx[chr].w;
    bbh = desc.bbx[chr].h;
    bbx = desc.bbx[chr].x;
    bby = desc.bbx[chr].y;
  }

  const uInt16* tmp = desc.bits + (desc.offset ? desc.offset[chr] : (chr * desc.fbbh));
  switch(myFB.myBytesPerPixel)
  {
    case 2:
    {
      // Get buffer position where upper-left pixel of the character will be drawn
      uInt16* buffer = (uInt16*)getBasePtr(tx + bbx, ty + desc.ascent - bby - bbh);

      for(int y = 0; y < bbh; y++)
      {
        const uInt16 ptr = *tmp++;
        uInt16 mask = 0x8000;
 
        for(int x = 0; x < bbw; x++, mask >>= 1)
          if(ptr & mask)
            buffer[x] = (uInt16) myFB.myDefPalette[color];

        buffer += myPitch;
      }
      break;
    }
    case 3:
    {
      // Get buffer position where upper-left pixel of the character will be drawn
      uInt8* buffer = (uInt8*)getBasePtr(tx + bbx, ty + desc.ascent - bby - bbh);

      uInt8 a = myFB.myDefPalette24[color][0],
            b = myFB.myDefPalette24[color][1],
            c = myFB.myDefPalette24[color][2];

      for(int y = 0; y < bbh; y++, buffer += myPitch)
      {
        const uInt16 ptr = *tmp++;
        uInt16 mask = 0x8000;

        uInt8* buf_ptr = buffer;
        for(int x = 0; x < bbw; x++, mask >>= 1)
        {
          if(ptr & mask)
          {
            *buf_ptr++ = a;  *buf_ptr++ = b;  *buf_ptr++ = c;
          }
          else
            buf_ptr += 3;
        }
      }
      break;
    }
    case 4:
    {
      // Get buffer position where upper-left pixel of the character will be drawn
      uInt32* buffer = (uInt32*)getBasePtr(tx + bbx, ty + desc.ascent - bby - bbh);
      for(int y = 0; y < bbh; y++, buffer += myPitch)
      {
        const uInt16 ptr = *tmp++;
        uInt16 mask = 0x8000;
 
        for(int x = 0; x < bbw; x++, mask >>= 1)
          if(ptr & mask)
            buffer[x] = (uInt32) myFB.myDefPalette[color];
      }
      break;
    }
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSoft::drawBitmap(uInt32* bitmap, uInt32 tx, uInt32 ty,
                               uInt32 color, uInt32 h)
{
  SDL_Rect rect;
  rect.y = ty + myYOffset;
  rect.w = rect.h = 1;
  for(uInt32 y = 0; y < h; y++)
  {
    rect.x = tx + myXOffset;
    uInt32 mask = 0xF0000000;
    for(uInt32 x = 0; x < 8; x++, mask >>= 4)
    {
      if(bitmap[y] & mask)
        SDL_FillRect(mySurface, &rect, myFB.myDefPalette[color]);

      rect.x++;
    }
    rect.y++;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSoft::drawPixels(uInt32* data, uInt32 tx, uInt32 ty,
                               uInt32 numpixels)
{
  SDL_Rect rect;
  rect.x = tx + myXOffset;
  rect.y = ty + myYOffset;
  rect.w = rect.h = 1;
  for(uInt32 x = 0; x < numpixels; ++x)
  {
    SDL_FillRect(mySurface, &rect, data[x]);
    rect.x++;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSoft::drawSurface(const FBSurface* surface, uInt32 tx, uInt32 ty)
{
  const FBSurfaceSoft* s = (const FBSurfaceSoft*) surface;

  SDL_Rect dstrect;
  dstrect.x = tx + myXOffset;
  dstrect.y = ty + myYOffset;
  SDL_Rect srcrect;
  srcrect.x = 0;
  srcrect.y = 0;
  srcrect.w = s->myWidth;
  srcrect.h = s->myHeight;

  SDL_BlitSurface(s->mySurface, &srcrect, mySurface, &dstrect);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSoft::addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h)
{
//cerr << " -> addDirtyRect: x = " << x << ", y = " << y << ", w = " << w << ", h = " << h << endl;

  // Base surfaces use dirty-rectangle updates, since they can be quite
  // large, and updating the entire surface each frame would be too slow
  // Non-base surfaces are usually smaller, and can be updated entirely
  if(myIsBaseSurface)
  {
    // Add a dirty rect to the UI rectangle list
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
  myXOrig = x;
  myYOrig = y;

  if(myIsBaseSurface)
  {
    myXOffset = myFB.imageRect().x();
    myYOffset = myFB.imageRect().y();
  }
  else
  {
    myXOffset = myYOffset = 0;
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
void FBSurfaceSoft::reload()
{
  switch(mySurface->format->BytesPerPixel)
  {
    case 2:  // 16-bit
      myPitch = mySurface->pitch >> 1;
      break;
    case 3:  // 24-bit
      myPitch = mySurface->pitch;
      break;
    case 4:  // 32-bit
      myPitch = mySurface->pitch >> 2;
      break;
  }
}
