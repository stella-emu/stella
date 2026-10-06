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
#include "QisBlitter.hxx"

namespace {
  constexpr float ALPHA_SCALE = 255.F / 100.F;
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QisBlitter::QisBlitter(FBBackendSDL& fb)
  : myFB{fb}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QisBlitter::~QisBlitter()
{
  free();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QisBlitter::isSupported(const FBBackendSDL& fb)
{
  if (!fb.isInitialized()) throw std::runtime_error("framebuffer not initialized");

  return fb.hasRenderTargetSupport();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QisBlitter::reinitialize(
  SDL_Rect srcRect, SDL_Rect destRect, bool enableBlend,
  uInt8 blendLevel, bool isStatic
)
{
  myRecreateTextures = myRecreateTextures || !(
    mySrcRect.w == srcRect.w &&
    mySrcRect.h == srcRect.h &&
    myDstRect.w == myFB.scaleX(destRect.w) &&
    myDstRect.h == myFB.scaleY(destRect.h) &&
    blendLevel  == myBlendLevel &&
    enableBlend == myEnableBlend &&
    isStatic == myIsStatic
   );

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
void QisBlitter::free()
{
  if (!myTexturesAreAllocated) {
    return;
  }

  ASSERT_MAIN_THREAD;

  if (mySrcTexture) {
    SDL_DestroyTexture(mySrcTexture);
    mySrcTexture = nullptr;
  }
  if (mySecondarySrcTexture) {
    SDL_DestroyTexture(mySecondarySrcTexture);
    mySecondarySrcTexture = nullptr;
  }
  if (myIntermediateTexture) {
    SDL_DestroyTexture(myIntermediateTexture);
    myIntermediateTexture = nullptr;
  }
  if (mySecondaryIntermediateTexture) {
    SDL_DestroyTexture(mySecondaryIntermediateTexture);
    mySecondaryIntermediateTexture = nullptr;
  }

  myTexturesAreAllocated = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QisBlitter::blit(SDL_Surface& surface, bool upload)
{
  ASSERT_MAIN_THREAD;

  recreateTexturesIfNecessary();

  if (myIsStatic) {
    if (upload || myUploadPending) {
      SDL_UpdateTexture(mySrcTexture, nullptr, surface.pixels, surface.pitch);
      blitToIntermediate();
      myUploadPending = false;
    }
  } else {
    // Render into the intermediate texture that wasn't drawn last, since that
    // one may still be in use; myIntermediateTexture is then always the one
    // to draw
    std::swap(myIntermediateTexture, mySecondaryIntermediateTexture);

    if (upload || myUploadPending) {
      // Likewise for the source texture
      std::swap(mySrcTexture, mySecondarySrcTexture);
      SDL_UpdateTexture(mySrcTexture, &mySrcRect, surface.pixels, surface.pitch);
      myUploadPending = false;
    }

    // Redrawn every time, even from unchanged pixels: a render target can
    // lose its contents (e.g. Direct3D 9 recreates them empty whenever the
    // window size or vsync changes)
    blitToIntermediate();
  }

  SDL_RenderTexture(myFB.renderer(), myIntermediateTexture,
                    &myIntermediateFRect, &myDstFRect);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QisBlitter::blitToIntermediate()
{
  ASSERT_MAIN_THREAD;

  SDL_FRect r{};
  SDL_RectToFRect(&mySrcRect, &r);
  r.x = r.y = 0.F;

  SDL_SetRenderTarget(myFB.renderer(), myIntermediateTexture);

  SDL_RenderTexture(myFB.renderer(), mySrcTexture, &r, &myIntermediateFRect);

  SDL_SetRenderTarget(myFB.renderer(), nullptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QisBlitter::recreateTexturesIfNecessary()
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

  myIntermediateRect.w = (myDstRect.w / mySrcRect.w) * mySrcRect.w;
  myIntermediateRect.h = (myDstRect.h / mySrcRect.h) * mySrcRect.h;
  myIntermediateRect.x = 0;
  myIntermediateRect.y = 0;
  SDL_RectToFRect(&myIntermediateRect, &myIntermediateFRect);

  mySrcTexture = SDL_CreateTexture(myFB.renderer(), myFB.pixelFormat().format,
    texAccess, mySrcRect.w, mySrcRect.h);
  SDL_SetTextureScaleMode(mySrcTexture, SDL_SCALEMODE_NEAREST);
  SDL_SetTextureBlendMode(mySrcTexture, SDL_BLENDMODE_NONE);

  if (!myIsStatic) {
    mySecondarySrcTexture = SDL_CreateTexture(myFB.renderer(),
        myFB.pixelFormat().format, texAccess, mySrcRect.w, mySrcRect.h);
    SDL_SetTextureScaleMode(mySecondarySrcTexture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(mySecondarySrcTexture, SDL_BLENDMODE_NONE);
  } else {
    mySecondarySrcTexture = nullptr;
  }

  myIntermediateTexture = SDL_CreateTexture(myFB.renderer(), myFB.pixelFormat().format,
      SDL_TEXTUREACCESS_TARGET, myIntermediateRect.w, myIntermediateRect.h);
  SDL_SetTextureScaleMode(myIntermediateTexture, SDL_SCALEMODE_LINEAR);

  if (!myIsStatic) {
    mySecondaryIntermediateTexture = SDL_CreateTexture(myFB.renderer(),
        myFB.pixelFormat().format, SDL_TEXTUREACCESS_TARGET,
        myIntermediateRect.w, myIntermediateRect.h);
    SDL_SetTextureScaleMode(mySecondaryIntermediateTexture,
                            SDL_SCALEMODE_LINEAR);
  } else {
    mySecondaryIntermediateTexture = nullptr;
  }

  const std::array<SDL_Texture*, 2> textures = {
    myIntermediateTexture, mySecondaryIntermediateTexture
  };

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
