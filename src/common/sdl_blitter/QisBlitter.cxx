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

#include "FBBackendSDL.hxx"
#include "ThreadDebugging.hxx"
#include "QisBlitter.hxx"

static void P(const char* str, const SDL_Rect r, const SDL_FRect fr)
{
  cerr << str << ": "
       << r.x << "," << r.y << " " << r.w << "x" << r.h << "  =>  "
       << fr.x << "," << fr.y << " " << fr.w << "x" << fr.h << '\n';
}
static void PF(const char* str, const SDL_FRect r, const SDL_FRect fr)
{
  cerr << str << ": "
       << r.x << "," << r.y << " " << r.w << "x" << r.h << "  =>  "
       << fr.x << "," << fr.y << " " << fr.w << "x" << fr.h << '\n';
}

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
  if (!fb.isInitialized()) throw runtime_error("framebuffer not initialized");

  return fb.hasRenderTargetSupport();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QisBlitter::reinitialize(
  SDL_Rect srcRect,
  SDL_Rect destRect,
  FBSurface::Attributes attributes,
  SDL_Surface* staticData
)
{
  myRecreateTextures = myRecreateTextures || !(
    mySrcRect.w == srcRect.w &&
    mySrcRect.h == srcRect.h &&
    myDstRect.w == myFB.scaleX(destRect.w) &&
    myDstRect.h == myFB.scaleY(destRect.h) &&
    attributes == myAttributes &&
    myStaticData == staticData
   );

  myAttributes = attributes;
  myStaticData = staticData;

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

  const std::array<SDL_Texture*, 3> textures = {
    mySrcTexture, myIntermediateTexture, mySecondaryIntermediateTexture
  };
  for (SDL_Texture* texture: textures) {
    if (!texture) continue;

    SDL_DestroyTexture(texture);
  }

  myTexturesAreAllocated = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QisBlitter::blit(SDL_Surface& surface)
{
  ASSERT_MAIN_THREAD;

  recreateTexturesIfNecessary();

  SDL_Texture* intermediateTexture = myIntermediateTexture;

  if(myStaticData == nullptr) {
    SDL_UpdateTexture(mySrcTexture, &mySrcRect, surface.pixels, surface.pitch);

    blitToIntermediate();

    myIntermediateTexture = mySecondaryIntermediateTexture;
    mySecondaryIntermediateTexture = intermediateTexture;

    SDL_Texture* temporary = mySrcTexture;
    mySrcTexture = mySecondarySrcTexture;
    mySecondarySrcTexture = temporary;
  }

  SDL_RenderTexture(myFB.renderer(), intermediateTexture,
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

  SDL_SetRenderDrawColor(myFB.renderer(), 0, 0, 0, 255);
  SDL_RenderClear(myFB.renderer());

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

  const SDL_TextureAccess texAccess = myStaticData == nullptr
                                        ? SDL_TEXTUREACCESS_STREAMING
                                        : SDL_TEXTUREACCESS_STATIC;

  myIntermediateRect.w = (myDstRect.w / mySrcRect.w) * mySrcRect.w;
  myIntermediateRect.h = (myDstRect.h / mySrcRect.h) * mySrcRect.h;
  myIntermediateRect.x = 0;
  myIntermediateRect.y = 0;
  SDL_RectToFRect(&myIntermediateRect, &myIntermediateFRect);

  mySrcTexture = SDL_CreateTexture(myFB.renderer(), myFB.pixelFormat().format,
    texAccess, mySrcRect.w, mySrcRect.h);
  SDL_SetTextureScaleMode(mySrcTexture, SDL_SCALEMODE_NEAREST);

  if (myStaticData == nullptr) {
    mySecondarySrcTexture = SDL_CreateTexture(myFB.renderer(),
        myFB.pixelFormat().format, texAccess, mySrcRect.w, mySrcRect.h);
    SDL_SetTextureScaleMode(mySecondarySrcTexture, SDL_SCALEMODE_NEAREST);
  } else {
    mySecondarySrcTexture = nullptr;
  }

  myIntermediateTexture = SDL_CreateTexture(myFB.renderer(), myFB.pixelFormat().format,
      SDL_TEXTUREACCESS_TARGET, myIntermediateRect.w, myIntermediateRect.h);
  SDL_SetTextureScaleMode(myIntermediateTexture, SDL_SCALEMODE_LINEAR);

  if (myStaticData == nullptr) {
    mySecondaryIntermediateTexture = SDL_CreateTexture(myFB.renderer(),
        myFB.pixelFormat().format, SDL_TEXTUREACCESS_TARGET,
        myIntermediateRect.w, myIntermediateRect.h);
    SDL_SetTextureScaleMode(mySecondaryIntermediateTexture,
                            SDL_SCALEMODE_LINEAR);
  } else {
    mySecondaryIntermediateTexture = nullptr;
    SDL_UpdateTexture(mySrcTexture, nullptr, myStaticData->pixels, myStaticData->pitch);

    blitToIntermediate();
  }

  if (myAttributes.blending) {
    const auto blendAlpha = static_cast<uInt8>(myAttributes.blendalpha * 2.55);

    const std::array<SDL_Texture*, 2> textures = {
      myIntermediateTexture, mySecondaryIntermediateTexture
    };
    for (SDL_Texture* texture: textures) {
      if (!texture) continue;

      SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
      SDL_SetTextureAlphaMod(texture, blendAlpha);
    }
  }

  myRecreateTextures = false;
  myTexturesAreAllocated = true;
}
