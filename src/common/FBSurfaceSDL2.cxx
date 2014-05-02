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

#include "Font.hxx"
#include "FBSurfaceSDL2.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceSDL2::FBSurfaceSDL2(FrameBufferSDL2& buffer, uInt32 width, uInt32 height)
  : FBSurface(buffer.myDefPalette),
    myFB(buffer),
    mySurface(NULL),
    myTexture(NULL),
    mySurfaceIsDirty(true)
{
  // Create a surface in the same format as the parent GL class
  const SDL_PixelFormat* pf = myFB.myPixelFormat;

  mySurface = SDL_CreateRGBSurface(0, width, height,
      pf->BitsPerPixel, pf->Rmask, pf->Gmask, pf->Bmask, pf->Amask);

  mySrc.x = mySrc.y = myDst.x = myDst.y = 0;
  mySrc.w = myDst.w = width;
  mySrc.h = myDst.h = height;

  myPixels = (uInt32*) mySurface->pixels;
  myPitch = mySurface->pitch / pf->BytesPerPixel;

  // To generate texture
  reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceSDL2::~FBSurfaceSDL2()
{
  if(mySurface)
    SDL_FreeSurface(mySurface);

  free();
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
  SDL_FillRect(mySurface, &tmp, myFB.myDefPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::drawSurface(const FBSurface* surface, uInt32 tx, uInt32 ty)
{
  const FBSurfaceSDL2* s = (const FBSurfaceSDL2*) surface;

  SDL_Rect dst;
  dst.x = tx;
  dst.y = ty;
  dst.w = s->mySrc.w;
  dst.h = s->mySrc.h;

  SDL_BlitSurface(s->mySurface, &(s->mySrc), mySurface, &dst);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h)
{
  // It's faster to just update the entire (hardware) surface
  mySurfaceIsDirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::getPos(uInt32& x, uInt32& y) const
{
  x = myDst.x;
  y = myDst.y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::setPos(uInt32 x, uInt32 y)
{
  myDst.x = x;
  myDst.y = y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::setWidth(uInt32 w)
{
  // This method can't be used with 'scaled' surface (aka TIA surfaces)
  // That shouldn't really matter, though, as all the UI stuff isn't scaled,
  // and it's the only thing that uses it
  mySrc.w = w;
  myDst.w = w;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::setHeight(uInt32 h)
{
  // This method can't be used with 'scaled' surface (aka TIA surfaces)
  // That shouldn't really matter, though, as all the UI stuff isn't scaled,
  // and it's the only thing that uses it
  mySrc.h = h;
  myDst.h = h;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::translateCoords(Int32& x, Int32& y) const
{
  x -= myDst.x;
  y -= myDst.y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::render()
{
  if(mySurfaceIsDirty)
  {
//cerr << "src: x=" << mySrc.x << ", y=" << mySrc.y << ", w=" << mySrc.w << ", h=" << mySrc.h << endl;
//cerr << "dst: x=" << myDst.x << ", y=" << myDst.y << ", w=" << myDst.w << ", h=" << myDst.h << endl;

    SDL_UpdateTexture(myTexture, &mySrc, mySurface->pixels, mySurface->pitch);
    SDL_RenderCopy(myFB.myRenderer, myTexture, &mySrc, &myDst);

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
  myTexture = SDL_CreateTexture(myFB.myRenderer,
      SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
      mySurface->w, mySurface->h);
}
