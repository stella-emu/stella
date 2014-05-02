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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <cmath>

#include "NTSCFilter.hxx"
#include "TIA.hxx"

#include "FBSurfaceTIA.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceTIA::FBSurfaceTIA(FrameBufferSDL2& buffer)
  : FBSurface(buffer.myDefPalette),
    myFB(buffer),
    mySurface(NULL),
    myTexture(NULL),
    myScanlines(NULL),
    myScanlinesEnabled(false),
    myScanlineIntensity(50)
{
  // Texture width is set to contain all possible sizes for a TIA image,
  // including Blargg filtering
  int width = ATARI_NTSC_OUT_WIDTH(160);
  int height = 320;

  // Create a surface in the same format as the parent GL class
  const SDL_PixelFormat* pf = myFB.myPixelFormat;

  mySurface = SDL_CreateRGBSurface(0, width, height*2,
      pf->BitsPerPixel, pf->Rmask, pf->Gmask, pf->Bmask, pf->Amask);

  mySrcR.x = mySrcR.y = myDstR.x = myDstR.y = myScanR.x = myScanR.y = 0;
  mySrcR.w = myDstR.w = width;
  mySrcR.h = myDstR.h = height;
  myScanR.w = 1;  myScanR.h = 0;

  myPitch = mySurface->pitch / pf->BytesPerPixel;

  // Generate scanline data
  myScanData = new uInt32[mySurface->h];
  for(int i = 0; i < mySurface->h; i+=2)
  {
    myScanData[i]   = 0x00000000;
    myScanData[i+1] = 0xff000000;
  }

  // To generate textures
  reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceTIA::~FBSurfaceTIA()
{
  if(mySurface)
    SDL_FreeSurface(mySurface);

  free();
  delete[] myScanData;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::getPos(uInt32& x, uInt32& y) const
{
  x = mySrcR.x;
  y = mySrcR.y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::translateCoords(Int32& x, Int32& y) const
{
  x = mySrcR.x;
  y = mySrcR.y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::render()
{
  // Copy the mediasource framebuffer to the RGB texture
  // In hardware rendering mode, it's faster to just assume that the screen
  // is dirty and always do an update

  uInt8* currentFrame  = myTIA->currentFrameBuffer();
  uInt8* previousFrame = myTIA->previousFrameBuffer();
  uInt32 width         = myTIA->width();
  uInt32 height        = myTIA->height();
  uInt32* buffer       = (uInt32*) mySurface->pixels;

  // TODO - Eventually 'phosphor' won't be a separate mode, and will become
  //        a post-processing filter by blending several frames.
  switch(myFB.myFilterType)
  {
    case FrameBufferSDL2::kNormal:
    {
      uInt32 bufofsY    = 0;
      uInt32 screenofsY = 0;
      for(uInt32 y = 0; y < height; ++y)
      {
        uInt32 pos = screenofsY;
        for(uInt32 x = 0; x < width; ++x)
          buffer[pos++] = (uInt32) myFB.myDefPalette[currentFrame[bufofsY + x]];

        bufofsY    += width;
        screenofsY += myPitch;
      }
      break;
    }
    case FrameBufferSDL2::kPhosphor:
    {
      uInt32 bufofsY    = 0;
      uInt32 screenofsY = 0;
      for(uInt32 y = 0; y < height; ++y)
      {
        uInt32 pos = screenofsY;
        for(uInt32 x = 0; x < width; ++x)
        {
          const uInt32 bufofs = bufofsY + x;
          buffer[pos++] = (uInt32)
            myFB.myAvgPalette[currentFrame[bufofs]][previousFrame[bufofs]];
        }
        bufofsY    += width;
        screenofsY += myPitch;
      }
      break;
    }
    case FrameBufferSDL2::kBlarggNormal:
    {
      myFB.myNTSCFilter.blit_single(currentFrame, width, height,
                                    buffer, mySurface->pitch);
      break;
    }
    case FrameBufferSDL2::kBlarggPhosphor:
    {
      myFB.myNTSCFilter.blit_double(currentFrame, previousFrame, width, height,
                                    buffer, mySurface->pitch);
      break;
    }
  }

  // Draw TIA image
  SDL_UpdateTexture(myTexture, &mySrcR, mySurface->pixels, mySurface->pitch);
  SDL_RenderCopy(myFB.myRenderer, myTexture, &mySrcR, &myDstR);

  // Draw overlaying scanlines
  if(myScanlinesEnabled)
    SDL_RenderCopy(myFB.myRenderer, myScanlines, &myScanR, &myDstR);

  // Let postFrameUpdate() know that a change has been made
  myFB.myDirtyFlag = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::invalidate()
{
  SDL_FillRect(mySurface, NULL, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::free()
{
  if(myTexture)
  {
    SDL_DestroyTexture(myTexture);
    myTexture = NULL;
  }
  if(myScanlines)
  {
    SDL_DestroyTexture(myScanlines);
    myScanlines = NULL;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::reload()
{
  // Re-create texture; the underlying SDL_Surface is fine as-is
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, myTexFilter[0] ? "1" : "0");
  myTexture = SDL_CreateTexture(myFB.myRenderer,
      myFB.myPixelFormat->format, SDL_TEXTUREACCESS_STREAMING,
      mySurface->w, mySurface->h);

  // Re-create scanline texture (contents don't change)
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, myTexFilter[1] ? "1" : "0");
  myScanlines = SDL_CreateTexture(myFB.myRenderer,
      myFB.myPixelFormat->format, SDL_TEXTUREACCESS_STATIC,
      1, mySurface->h);
  SDL_SetTextureBlendMode(myScanlines, SDL_BLENDMODE_BLEND);
  SDL_SetTextureAlphaMod(myScanlines, Uint8(myScanlineIntensity*2.55));
  SDL_UpdateTexture(myScanlines, &myScanR, myScanData, 4);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::updateCoords(uInt32 baseH,
     uInt32 imgX, uInt32 imgY, uInt32 imgW, uInt32 imgH)
{
  mySrcR.h = baseH;
  myDstR.w = imgW;
  myDstR.h = imgH;

  // Scanline repeating is sensitive to non-integral vertical resolution,
  // so rounding is performed to eliminate it
  // This won't be 100% accurate, but non-integral scaling isn't 100%
  // accurate anyway
  myScanR.w = 1;
  myScanR.h = int(2 * float(imgH) / floor(((float)imgH / baseH) + 0.5));

  updateCoords();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::updateCoords()
{
  // Normal TIA rendering and TV effects use different widths
  // We use the same buffer, and only pick the width we need
  mySrcR.w = myFB.ntscEnabled() ? ATARI_NTSC_OUT_WIDTH(160) : 160;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::setTIAPalette(const uInt32* palette)
{
  myFB.myNTSCFilter.setTIAPalette(myFB, palette);
}
