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

#include "HqBlitter.hxx"

#include "ThreadDebugging.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HqBlitter::HqBlitter(FrameBufferSDL2& fb) :
  mySrcTexture(nullptr),
  myIntermediateTexture(nullptr),
  mySecondaryIntermedateTexture(nullptr),
  myTexturesAreAllocated(false),
  myRecreateTextures(false),
  myStaticData(nullptr),
  myFB(fb)
{}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HqBlitter::~HqBlitter()
{
  free();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HqBlitter::isSupported(FrameBufferSDL2 &fb)
{
  ASSERT_MAIN_THREAD;

  if (!fb.isInitialized()) throw runtime_error("frambuffer not initialized");

  SDL_RendererInfo info;

  SDL_GetRendererInfo(fb.renderer(), &info);

  if (!(info.flags & SDL_RENDERER_TARGETTEXTURE)) return false;

  SDL_Texture* tex = SDL_CreateTexture(fb.renderer(), fb.pixelFormat().format, SDL_TEXTUREACCESS_TARGET, 16, 16);

  if (!tex) return false;

  int sdlError = SDL_SetRenderTarget(fb.renderer(), tex);
  SDL_SetRenderTarget(fb.renderer(), nullptr);

  SDL_DestroyTexture(tex);

  return sdlError == 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HqBlitter::reinitialize(
  SDL_Rect srcRect,
  SDL_Rect destRect,
  FBSurface::Attributes attributes,
  SDL_Surface* staticData
)
{
  myRecreateTextures = !(
    mySrcRect.w == srcRect.w &&
    mySrcRect.h == srcRect.h &&
    myDstRect.w == destRect.w &&
    myDstRect.h == destRect.h &&
    attributes == myAttributes &&
    myStaticData == staticData
   );

   myStaticData = staticData;
   mySrcRect = srcRect;
   myDstRect = destRect;
   myAttributes = attributes;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HqBlitter::free()
{
  if (!myTexturesAreAllocated) {
    return;
  }

  ASSERT_MAIN_THREAD;

  SDL_Texture* textures[] = {mySrcTexture, myIntermediateTexture, mySecondaryIntermedateTexture};
  for (SDL_Texture* texture: textures) {
    if (!texture) continue;

    SDL_DestroyTexture(texture);
  }

  myTexturesAreAllocated = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HqBlitter::blit(SDL_Surface& surface)
{
  ASSERT_MAIN_THREAD;

  recreateTexturesIfNecessary();

  SDL_Texture* texture = myIntermediateTexture;

    if(myStaticData == nullptr) {
      SDL_UpdateTexture(mySrcTexture, &mySrcRect, surface.pixels, surface.pitch);

      blitToIntermediate();

      myIntermediateTexture = mySecondaryIntermedateTexture;
      mySecondaryIntermedateTexture = texture;
    }

    SDL_RenderCopy(myFB.renderer(), texture, &myIntermediateRect, &myDstRect);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HqBlitter::blitToIntermediate()
{
  ASSERT_MAIN_THREAD;

  SDL_Rect r = mySrcRect;
  r.x = r.y = 0;

  SDL_UpdateTexture(myIntermediateTexture, nullptr, myBlankBuffer.get(), 4 * myIntermediateRect.w);

  SDL_SetRenderTarget(myFB.renderer(), myIntermediateTexture);
  SDL_RenderCopy(myFB.renderer(), mySrcTexture, &r, &myIntermediateRect);

  SDL_SetRenderTarget(myFB.renderer(), nullptr);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HqBlitter::recreateTexturesIfNecessary()
{
  if (myTexturesAreAllocated && !myRecreateTextures) {
    return;
  }

  ASSERT_MAIN_THREAD;

  if (myTexturesAreAllocated) {
    free();
  }

  SDL_TextureAccess texAccess = myStaticData == nullptr ? SDL_TEXTUREACCESS_STREAMING : SDL_TEXTUREACCESS_STATIC;

  myIntermediateRect.w = (myDstRect.w / mySrcRect.w) * mySrcRect.w;
  myIntermediateRect.h = (myDstRect.h / mySrcRect.h) * mySrcRect.h;
  myIntermediateRect.x = 0;
  myIntermediateRect.y = 0;

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

  mySrcTexture = SDL_CreateTexture(myFB.renderer(), myFB.pixelFormat().format,
    texAccess, mySrcRect.w, mySrcRect.h);

  myBlankBuffer = make_unique<uInt32[]>(4 * myIntermediateRect.w * myIntermediateRect.h);
  memset(myBlankBuffer.get(), 0, 4 * myIntermediateRect.w * myIntermediateRect.h);

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, myAttributes.smoothing ? "1" : "0");

  myIntermediateTexture = SDL_CreateTexture(myFB.renderer(), myFB.pixelFormat().format,
      SDL_TEXTUREACCESS_TARGET, myIntermediateRect.w, myIntermediateRect.h);

  if (myStaticData == nullptr) {
    mySecondaryIntermedateTexture = SDL_CreateTexture(myFB.renderer(), myFB.pixelFormat().format,
        SDL_TEXTUREACCESS_TARGET, myIntermediateRect.w, myIntermediateRect.h);
  } else {
    mySecondaryIntermedateTexture = nullptr;
    SDL_UpdateTexture(mySrcTexture, nullptr, myStaticData->pixels, myStaticData->pitch);

    blitToIntermediate();
  }

  if (myAttributes.blending) {
    uInt8 blendAlpha = uInt8(myAttributes.blendalpha * 2.55);

    SDL_Texture* textures[] = {mySrcTexture, myIntermediateTexture, mySecondaryIntermedateTexture};
    for (SDL_Texture* texture: textures) {
      if (!texture) continue;

      SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
      SDL_SetTextureAlphaMod(texture, blendAlpha);
    }
  }

  myRecreateTextures = false;
  myTexturesAreAllocated = true;
}
