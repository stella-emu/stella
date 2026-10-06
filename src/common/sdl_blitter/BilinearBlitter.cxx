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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "FBBackendSDL.hxx"
#include "ThreadDebugging.hxx"
#include "BilinearBlitter.hxx"

namespace {
  constexpr float ALPHA_SCALE = 255.F / 100.F;

  // Texture allocation granularity: rounding a growing source up to this
  // gives the allocation headroom, so a live-resize drag recreates the
  // textures roughly once per block instead of on every ~15px growth step.
  constexpr int TEXTURE_ALLOC_BLOCK = 128;

  constexpr int roundUpToBlock(int size) {
    return (size + TEXTURE_ALLOC_BLOCK - 1) / TEXTURE_ALLOC_BLOCK * TEXTURE_ALLOC_BLOCK;
  }
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BilinearBlitter::BilinearBlitter(FBBackendSDL& fb, bool interpolate)
  : myFB{fb},
    myInterpolate{interpolate}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BilinearBlitter::~BilinearBlitter()
{
  free();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BilinearBlitter::reinitialize(
  SDL_Rect srcRect, SDL_Rect destRect, bool enableBlend,
  uInt8 blendLevel, bool isStatic
)
{
  // The textures are sized from the SOURCE only; the destination rect is
  // applied at render time (SDL_RenderTexture).  So a destination change (e.g.
  // rescaling as the window is resized) needs no texture recreation.  A source
  // that fits the current texture is reused too — only the rendered sub-rect
  // shrinks — so a live window resize does not thrash textures.  Recreate only
  // when the source grows past the allocation, or blending/static data changes.
  myRecreateTextures = myRecreateTextures ||
    srcRect.w > myTexW || srcRect.h > myTexH ||
    blendLevel  != myBlendLevel ||
    enableBlend != myEnableBlend ||
    isStatic != myIsStatic;

  // Only the source rect was ever uploaded, so a different one isn't in the
  // textures yet
  myUploadPending = myUploadPending || !SDL_RectsEqual(&srcRect, &mySrcRect);

  myEnableBlend = enableBlend;
  myBlendLevel = blendLevel;
  myIsStatic = isStatic;

  mySrcRect = srcRect;
  SDL_RectToFRect(&mySrcRect, &mySrcFRect);

  myDstRect.x = myFB.scaleX(destRect.x);
  myDstRect.y = myFB.scaleY(destRect.y);
  myDstRect.w = myFB.scaleX(destRect.w);
  myDstRect.h = myFB.scaleY(destRect.h);
  SDL_RectToFRect(&myDstRect, &myDstFRect);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BilinearBlitter::free()
{
  if (!myTexturesAreAllocated) {
    return;
  }

  ASSERT_MAIN_THREAD;

  if (myTexture) {
    SDL_DestroyTexture(myTexture);  myTexture = nullptr;
  }
  if (mySecondaryTexture) {
    SDL_DestroyTexture(mySecondaryTexture);  mySecondaryTexture = nullptr;
  }

  myTexturesAreAllocated = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BilinearBlitter::blit(SDL_Surface& surface, bool upload)
{
  ASSERT_MAIN_THREAD;

  recreateTexturesIfNecessary();

  if(upload || myUploadPending)
  {
    if(myIsStatic)
    {
      // Only the data's own area: the texture may be larger (see
      // recreateTexturesIfNecessary())
      const SDL_Rect staticRect{0, 0, surface.w, surface.h};
      SDL_UpdateTexture(myTexture, &staticRect, surface.pixels, surface.pitch);
    }
    else
    {
      // Upload into the texture that wasn't drawn last, since that one may
      // still be in use; myTexture is then always the one to draw
      std::swap(myTexture, mySecondaryTexture);
      SDL_UpdateTexture(myTexture, &mySrcRect, surface.pixels, surface.pitch);
    }
    myUploadPending = false;
  }

  SDL_RenderTexture(myFB.renderer(), myTexture, &mySrcFRect, &myDstFRect);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BilinearBlitter::recreateTexturesIfNecessary()
{
  if (myTexturesAreAllocated && !myRecreateTextures) {
    return;
  }

  ASSERT_MAIN_THREAD;

  if (myTexturesAreAllocated) {
    free();
  }

  const SDL_TextureAccess texAccess = myIsStatic
    ? SDL_TEXTUREACCESS_STATIC
    : SDL_TEXTUREACCESS_STREAMING;

  // Size the textures to the larger of the current source (rounded up for
  // headroom) and the previous allocation, so they never shrink; only the
  // rendered sub-rect (mySrcFRect) shrinks.  (Static surfaces have a fixed
  // source, so the rounding just wastes a little VRAM for them.)
  myTexW = std::max(roundUpToBlock(mySrcRect.w), myTexW);
  myTexH = std::max(roundUpToBlock(mySrcRect.h), myTexH);

  myTexture = SDL_CreateTexture(myFB.renderer(), myFB.pixelFormat().format,
      texAccess, myTexW, myTexH);
  SDL_SetTextureScaleMode(myTexture, myInterpolate
      ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);

  if (!myIsStatic) {
    mySecondaryTexture = SDL_CreateTexture(myFB.renderer(),
        myFB.pixelFormat().format,
        texAccess, myTexW, myTexH);
    SDL_SetTextureScaleMode(mySecondaryTexture, myInterpolate
        ? SDL_SCALEMODE_LINEAR
        : SDL_SCALEMODE_NEAREST);
  } else {
    mySecondaryTexture = nullptr;
  }

  const std::array<SDL_Texture*, 2> textures = { myTexture, mySecondaryTexture };

  for (SDL_Texture* texture: textures) {
    if (!texture) continue;

    if (myEnableBlend) {
      SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
      SDL_SetTextureAlphaMod(texture, myBlendLevel * ALPHA_SCALE);
    } else {
      SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
    }
  }

  myRecreateTextures = false;
  myTexturesAreAllocated = true;
  // New textures are empty
  myUploadPending = true;
}
