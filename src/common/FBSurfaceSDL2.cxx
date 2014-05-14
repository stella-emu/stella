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

#include "FBSurfaceSDL2.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceSDL2::FBSurfaceSDL2(FrameBufferSDL2& buffer,
                             uInt32 width, uInt32 height, const uInt32* data)
  : myFB(buffer),
    mySurface(NULL),
    myTexture(NULL),
    mySurfaceIsDirty(true),
    myTexAccess(SDL_TEXTUREACCESS_STREAMING),
    myInterpolate(false),
    myBlendEnabled(false),
    myBlendAlpha(255),
    myStaticData(NULL)
{
  // Create a surface in the same format as the parent GL class
  const SDL_PixelFormat* pf = myFB.myPixelFormat;

  mySurface = SDL_CreateRGBSurface(0, width, height,
      pf->BitsPerPixel, pf->Rmask, pf->Gmask, pf->Bmask, pf->Amask);

  // We start out with the src and dst rectangles containing the same
  // dimensions, indicating no scaling or re-positioning
  mySrcR.x = mySrcR.y = myDstR.x = myDstR.y = 0;
  mySrcR.w = myDstR.w = width;
  mySrcR.h = myDstR.h = height;

  ////////////////////////////////////////////////////
  // These *must* be set for the parent class
  myPixels = (uInt32*) mySurface->pixels;
  myPitch = mySurface->pitch / pf->BytesPerPixel;
  ////////////////////////////////////////////////////

  if(data)
  {
    myTexAccess = SDL_TEXTUREACCESS_STATIC;
    myStaticPitch = mySurface->w * 4;  // we need pitch in 'bytes'
    myStaticData = new uInt32[mySurface->w * mySurface->h];
    SDL_memcpy(myStaticData, data, mySurface->w * mySurface->h * 4);
  }

  // To generate texture
  reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceSDL2::~FBSurfaceSDL2()
{
  if(mySurface)
    SDL_FreeSurface(mySurface);

  free();

  if(myStaticData)
  {
    delete[] myStaticData;
    myStaticData = NULL;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, uInt32 color)
{
  // Fill the rectangle
  SDL_Rect tmp;
  tmp.x = x;
  tmp.y = y;
  tmp.w = w;
  tmp.h = h;
  SDL_FillRect(mySurface, &tmp, myPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::drawSurface(const FBSurface* surface)
{
  const FBSurfaceSDL2* s = (const FBSurfaceSDL2*) surface;
  SDL_Rect dst = s->myDstR;
  SDL_BlitSurface(s->mySurface, &(s->mySrcR), mySurface, &dst);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FBSurfaceSDL2::width() const
{
  return mySurface->w;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FBSurfaceSDL2::height() const
{
  return mySurface->h;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const GUI::Rect& FBSurfaceSDL2::srcRect()
{
  mySrcGUIR.setBounds(mySrcR.x, mySrcR.y, mySrcR.x+mySrcR.w, mySrcR.y+mySrcR.h);
  return mySrcGUIR;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const GUI::Rect& FBSurfaceSDL2::dstRect()
{
  myDstGUIR.setBounds(myDstR.x, myDstR.y, myDstR.x+myDstR.w, myDstR.y+myDstR.h);
  return myDstGUIR;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::setSrcPos(uInt32 x, uInt32 y)
{
  mySrcR.x = x;  mySrcR.y = y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::setSrcSize(uInt32 w, uInt32 h)
{
  mySrcR.w = w;  mySrcR.h = h;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::setDstPos(uInt32 x, uInt32 y)
{
  myDstR.x = x;  myDstR.y = y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::setDstSize(uInt32 w, uInt32 h)
{
  myDstR.w = w;  myDstR.h = h;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::translateCoords(Int32& x, Int32& y) const
{
  x -= myDstR.x;
  y -= myDstR.y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::render()
{
  if(mySurfaceIsDirty)
  {
//cerr << "src: x=" << mySrcR.x << ", y=" << mySrcR.y << ", w=" << mySrcR.w << ", h=" << mySrcR.h << endl;
//cerr << "dst: x=" << myDstR.x << ", y=" << myDstR.y << ", w=" << myDstR.w << ", h=" << myDstR.h << endl;

    if(myTexAccess == SDL_TEXTUREACCESS_STREAMING)
      SDL_UpdateTexture(myTexture, &mySrcR, mySurface->pixels, mySurface->pitch);
    SDL_RenderCopy(myFB.myRenderer, myTexture, &mySrcR, &myDstR);

    mySurfaceIsDirty = false;

    // Let postFrameUpdate() know that a change has been made
    myFB.myDirtyFlag = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::invalidate()
{
  SDL_FillRect(mySurface, NULL, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::free()
{
  if(myTexture)
  {
    SDL_DestroyTexture(myTexture);
    myTexture = NULL;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::reload()
{
  // Re-create texture; the underlying SDL_Surface is fine as-is
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, myInterpolate ? "1" : "0");
  myTexture = SDL_CreateTexture(myFB.myRenderer, myFB.myPixelFormat->format,
      myTexAccess, mySurface->w, mySurface->h);

  // If the data is static, we only upload it once
  if(myTexAccess == SDL_TEXTUREACCESS_STATIC)
    SDL_UpdateTexture(myTexture, NULL, myStaticData, myStaticPitch);

  // Blending enabled?
  if(myBlendEnabled)
  {
    SDL_SetTextureBlendMode(myTexture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(myTexture, myBlendAlpha);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::applyAttributes(bool immediate)
{
  myInterpolate  = myAttributes.smoothing;
  myBlendEnabled = myAttributes.blending;
  myBlendAlpha   = uInt8(myAttributes.blendalpha * 2.55);

  if(immediate)
  {
    // Re-create the texture with the new settings
    free();
    reload();
  }
}
