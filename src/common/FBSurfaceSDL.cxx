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
// Copyright (c) 1995-2025 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "FBSurfaceSDL.hxx"

#include "Logger.hxx"
#include "ThreadDebugging.hxx"
#include "sdl_blitter/BlitterFactory.hxx"

namespace {
  BlitterFactory::ScalingAlgorithm scalingAlgorithm(ScalingInterpolation inter)
  {
    switch (inter) {
      case ScalingInterpolation::none:
        return BlitterFactory::ScalingAlgorithm::nearestNeighbour;

      case ScalingInterpolation::blur:
        return BlitterFactory::ScalingAlgorithm::bilinear;

      case ScalingInterpolation::sharp:
        return BlitterFactory::ScalingAlgorithm::quasiInteger;

      default:
        throw runtime_error("unreachable");
    }
  }
} // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceSDL::FBSurfaceSDL(FBBackendSDL& backend,
                           uInt32 width, uInt32 height,
                           ScalingInterpolation inter,
                           const uInt32* staticData)
  : myBackend{backend},
    myInterpolationMode{inter}
{
  //cerr << width << " x " << height << '\n';
  createSurface(width, height, staticData);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceSDL::~FBSurfaceSDL()
{
  ASSERT_MAIN_THREAD;

  if(mySurface)
  {
    SDL_DestroySurface(mySurface);
    mySurface = nullptr;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, ColorId color)
{
  ASSERT_MAIN_THREAD;

  const SDL_Rect tmp = ToSDLRect(x, y, w, h);
  SDL_FillSurfaceRect(mySurface, &tmp, myPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FBSurfaceSDL::width() const
{
  return mySurface->w;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FBSurfaceSDL::height() const
{
  return mySurface->h;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Common::Rect& FBSurfaceSDL::srcRect() const
{
  return mySrcGUIR;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Common::Rect& FBSurfaceSDL::dstRect() const
{
  return myDstGUIR;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL::setSrcPos(uInt32 x, uInt32 y)
{
  if(setSrcPosInternal(x, y))
    reinitializeBlitter();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL::setSrcSize(uInt32 w, uInt32 h)
{
  if(setSrcSizeInternal(w, h))
    reinitializeBlitter();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL::setSrcRect(const Common::Rect& r)
{
  const bool posChanged = setSrcPosInternal(r.x(), r.y()),
             sizeChanged = setSrcSizeInternal(r.w(), r.h());

  if(posChanged || sizeChanged)
    reinitializeBlitter();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL::setDstPos(uInt32 x, uInt32 y)
{
  if(setDstPosInternal(x, y))
    reinitializeBlitter();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL::setDstSize(uInt32 w, uInt32 h)
{
  if(setDstSizeInternal(w, h))
    reinitializeBlitter();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL::setDstRect(const Common::Rect& r)
{
  const bool posChanged = setDstPosInternal(r.x(), r.y()),
             sizeChanged = setDstSizeInternal(r.w(), r.h());

  if(posChanged || sizeChanged)
    reinitializeBlitter();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL::setVisible(bool visible)
{
  myIsVisible = visible;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL::translateCoords(Int32& x, Int32& y) const
{
  x -= myDstR.x;  x /= myDstR.w / mySrcR.w;
  y -= myDstR.y;  y /= myDstR.h / mySrcR.h;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FBSurfaceSDL::render()
{
  if(!myBlitter)
    reinitializeBlitter();

  if(myIsVisible && myBlitter)
  {
    myBlitter->blit(*mySurface);

    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL::invalidate()
{
  ASSERT_MAIN_THREAD;

  SDL_FillSurfaceRect(mySurface, nullptr, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL::invalidateRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h)
{
  ASSERT_MAIN_THREAD;

  // Note: Transparency has to be 0 to clear the rectangle foreground
  //       without affecting the background display.
  const SDL_Rect tmp = ToSDLRect(x, y, w, h);
  SDL_FillSurfaceRect(mySurface, &tmp, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL::reload()
{
  reinitializeBlitter(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL::resize(uInt32 width, uInt32 height)
{
  ASSERT_MAIN_THREAD;

  if(mySurface)
    SDL_DestroySurface(mySurface);

  // NOTE: Currently, a resize changes a 'static' surface to 'streaming'
  //       No code currently does this, but we should at least check for it
  if(myIsStatic)
    Logger::error("Resizing static texture!");

  createSurface(width, height, nullptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL::createSurface(uInt32 width, uInt32 height, const uInt32* data)
{
  ASSERT_MAIN_THREAD;

  assert(width > 0 && height > 0);

  // Create a surface in the same format as the parent GL class
  const SDL_PixelFormatDetails& pf = myBackend.pixelFormat();
  mySurface = SDL_CreateSurface(width, height, pf.format);

  // We start out with the src and dst rectangles containing the same
  // dimensions, indicating no scaling or re-positioning
  setSrcPosInternal(0, 0);
  setDstPosInternal(0, 0);
  setSrcSizeInternal(width, height);
  setDstSizeInternal(width, height);

  ////////////////////////////////////////////////////
  // These *must* be set for the parent class
  myPixels = static_cast<uInt32*>(mySurface->pixels);
  myPitch = mySurface->pitch / pf.bytes_per_pixel;
  ////////////////////////////////////////////////////

  myIsStatic = data != nullptr;
  if(myIsStatic)
    SDL_memcpy(mySurface->pixels, data,
               static_cast<size_t>(mySurface->w) * mySurface->h * 4);

  reload();  // NOLINT
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL::reinitializeBlitter(bool force)
{
  if(force)
    myBlitter.reset();

  if(!myBlitter && myBackend.isInitialized())
    myBlitter = BlitterFactory::createBlitter(
        myBackend, scalingAlgorithm(myInterpolationMode));

  if(myBlitter)
    myBlitter->reinitialize(mySrcR, myDstR, myEnableBlend, myBlendLevel,
                            myIsStatic ? mySurface : nullptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL::setBlendLevel(uInt32 percent)
{
  myBlendLevel = BSPF::clamp(percent, 0U, 100U);

  reinitializeBlitter();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL::enableBlend(bool enableBlend)
{
  myEnableBlend = enableBlend;

  reinitializeBlitter();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceSDL::setScalingInterpolation(ScalingInterpolation interpolation)
{
  if(interpolation == ScalingInterpolation::sharp &&
      (
        static_cast<int>(mySrcGUIR.h()) >= myBackend.scaleY(myDstGUIR.h()) ||
        static_cast<int>(mySrcGUIR.w()) >= myBackend.scaleX(myDstGUIR.w())
      )
  )
    interpolation = ScalingInterpolation::blur;

  if(interpolation == myInterpolationMode)
    return;

  myInterpolationMode = interpolation;
  reload();
}
