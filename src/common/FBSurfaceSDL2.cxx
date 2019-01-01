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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "FBSurfaceSDL2.hxx"

#include "ThreadDebugging.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceSDL2::FBSurfaceSDL2(FrameBufferSDL2& buffer,
                             uInt32 width, uInt32 height, const uInt32* data)
  : myFB(buffer),
    mySurface(nullptr),
    myTexture(nullptr),
    mySurfaceIsDirty(true),
    myIsVisible(true),
    myTexAccess(SDL_TEXTUREACCESS_STREAMING),
    myInterpolate(false),
    myBlendEnabled(false),
    myBlendAlpha(255)
{
  createSurface(width, height, data);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceSDL2::~FBSurfaceSDL2()
{
  ASSERT_MAIN_THREAD;

  if(mySurface)
  {
    SDL_FreeSurface(mySurface);
    mySurface = nullptr;
  }

  free();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, ColorId color)
{
  ASSERT_MAIN_THREAD;

  // Fill the rectangle
  SDL_Rect tmp;
  tmp.x = x;
  tmp.y = y;
  tmp.w = w;
  tmp.h = h;
  SDL_FillRect(mySurface, &tmp, myPalette[color]);
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
const GUI::Rect& FBSurfaceSDL2::srcRect() const
{
  return mySrcGUIR;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const GUI::Rect& FBSurfaceSDL2::dstRect() const
{
  return myDstGUIR;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::setSrcPos(uInt32 x, uInt32 y)
{
  mySrcR.x = x;  mySrcR.y = y;
  mySrcGUIR.moveTo(x, y);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::setSrcSize(uInt32 w, uInt32 h)
{
  mySrcR.w = w;  mySrcR.h = h;
  mySrcGUIR.setWidth(w);  mySrcGUIR.setHeight(h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::setDstPos(uInt32 x, uInt32 y)
{
  myDstR.x = x;  myDstR.y = y;
  myDstGUIR.moveTo(x, y);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::setDstSize(uInt32 w, uInt32 h)
{
  myDstR.w = w;  myDstR.h = h;
  myDstGUIR.setWidth(w);  myDstGUIR.setHeight(h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::setVisible(bool visible)
{
  myIsVisible = visible;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::translateCoords(Int32& x, Int32& y) const
{
  x -= myDstR.x;  x /= myDstR.w / mySrcR.w;
  y -= myDstR.y;  y /= myDstR.h / mySrcR.h;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FBSurfaceSDL2::render()
{
  ASSERT_MAIN_THREAD;

  if(myIsVisible)
  {
    if(myTexAccess == SDL_TEXTUREACCESS_STREAMING)
      SDL_UpdateTexture(myTexture, &mySrcR, mySurface->pixels, mySurface->pitch);
    SDL_RenderCopy(myFB.myRenderer, myTexture, &mySrcR, &myDstR);

    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::invalidate()
{
  ASSERT_MAIN_THREAD;

  SDL_FillRect(mySurface, nullptr, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::free()
{
  ASSERT_MAIN_THREAD;

  if(myTexture)
  {
    SDL_DestroyTexture(myTexture);
    myTexture = nullptr;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::reload()
{
  ASSERT_MAIN_THREAD;

  // Re-create texture; the underlying SDL_Surface is fine as-is
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, myInterpolate ? "1" : "0");
  myTexture = SDL_CreateTexture(myFB.myRenderer, myFB.myPixelFormat->format,
      myTexAccess, mySurface->w, mySurface->h);

  // If the data is static, we only upload it once
  if(myTexAccess == SDL_TEXTUREACCESS_STATIC)
    SDL_UpdateTexture(myTexture, nullptr, myStaticData.get(), myStaticPitch);

  // Blending enabled?
  if(myBlendEnabled)
  {
    SDL_SetTextureBlendMode(myTexture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(myTexture, myBlendAlpha);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::resize(uInt32 width, uInt32 height)
{
  ASSERT_MAIN_THREAD;

  // We will only resize when necessary, and not using static textures
  if((myTexAccess == SDL_TEXTUREACCESS_STATIC) || (mySurface &&
      int(width) <= mySurface->w && int(height) <= mySurface->h))
    return;  // don't need to resize at all

  if(mySurface)
    SDL_FreeSurface(mySurface);
  free();

  createSurface(width, height, nullptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL2::createSurface(uInt32 width, uInt32 height,
                                  const uInt32* data)
{
  ASSERT_MAIN_THREAD;

  // Create a surface in the same format as the parent GL class
  const SDL_PixelFormat* pf = myFB.myPixelFormat;

  mySurface = SDL_CreateRGBSurface(0, width, height,
      pf->BitsPerPixel, pf->Rmask, pf->Gmask, pf->Bmask, pf->Amask);

  // We start out with the src and dst rectangles containing the same
  // dimensions, indicating no scaling or re-positioning
  setSrcPos(0, 0);
  setDstPos(0, 0);
  setSrcSize(width, height);
  setDstSize(width, height);

  ////////////////////////////////////////////////////
  // These *must* be set for the parent class
  myPixels = reinterpret_cast<uInt32*>(mySurface->pixels);
  myPitch = mySurface->pitch / pf->BytesPerPixel;
  ////////////////////////////////////////////////////

  if(data)
  {
    myTexAccess = SDL_TEXTUREACCESS_STATIC;
    myStaticPitch = mySurface->w * 4;  // we need pitch in 'bytes'
    myStaticData = make_unique<uInt32[]>(mySurface->w * mySurface->h);
    SDL_memcpy(myStaticData.get(), data, mySurface->w * mySurface->h * 4);
  }

  applyAttributes(false);

  // To generate texture
  reload();
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
