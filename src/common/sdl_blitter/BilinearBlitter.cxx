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
#include "BilinearBlitter.hxx"

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
  uInt8 blendLevel, SDL_Surface* staticData
)
{
  myRecreateTextures = myRecreateTextures || !(
    mySrcRect.w == srcRect.w &&
    mySrcRect.h == srcRect.h &&
    myDstRect.w == myFB.scaleX(destRect.w) &&
    myDstRect.h == myFB.scaleY(destRect.h) &&
    blendLevel  == myBlendLevel &&
    enableBlend == myEnableBlend &&
    myStaticData == staticData
   );

  myEnableBlend = enableBlend;
  myBlendLevel = blendLevel;
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
void BilinearBlitter::free()
{
  if (!myTexturesAreAllocated) {
    return;
  }

  ASSERT_MAIN_THREAD;

  const std::array<SDL_Texture*, 2> textures = {
    myTexture, mySecondaryTexture
  };
  for (SDL_Texture* texture: textures) {
    if (!texture) continue;

    SDL_DestroyTexture(texture);
  }

  myTexturesAreAllocated = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BilinearBlitter::blit(SDL_Surface& surface)
{
  ASSERT_MAIN_THREAD;

  recreateTexturesIfNecessary();

  SDL_Texture* texture = myTexture;

  if(myStaticData == nullptr) {
    SDL_UpdateTexture(myTexture, &mySrcRect, surface.pixels, surface.pitch);
    myTexture = mySecondaryTexture;
    mySecondaryTexture = texture;
  }

  SDL_RenderTexture(myFB.renderer(), texture, &mySrcFRect, &myDstFRect);
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

  const SDL_TextureAccess texAccess = myStaticData == nullptr
    ? SDL_TEXTUREACCESS_STREAMING
    : SDL_TEXTUREACCESS_STATIC;

  myTexture = SDL_CreateTexture(myFB.renderer(), myFB.pixelFormat().format,
      texAccess, mySrcRect.w, mySrcRect.h);
  SDL_SetTextureScaleMode(myTexture, myInterpolate
      ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);

  if (myStaticData == nullptr) {
    mySecondaryTexture = SDL_CreateTexture(myFB.renderer(),
        myFB.pixelFormat().format,
        texAccess, mySrcRect.w, mySrcRect.h);
    SDL_SetTextureScaleMode(mySecondaryTexture, myInterpolate
        ? SDL_SCALEMODE_LINEAR
        : SDL_SCALEMODE_NEAREST);
  } else {
    mySecondaryTexture = nullptr;
    SDL_UpdateTexture(myTexture, nullptr, myStaticData->pixels, myStaticData->pitch);
  }

  const std::array<SDL_Texture*, 2> textures = { myTexture, mySecondaryTexture };

  for (SDL_Texture* texture: textures) {
    if (!texture) continue;

    if (myEnableBlend) {
      SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
      SDL_SetTextureAlphaMod(texture, myBlendLevel * 2.55);
    } else {
      SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
    }
  }

  myRecreateTextures = false;
  myTexturesAreAllocated = true;
}
