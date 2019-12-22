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

#ifndef BILINEAR_BLITTER_HXX
#define BILINEAR_BLITTER_HXX

#include "Blitter.hxx"
#include "FrameBufferSDL2.hxx"
#include "SDL_lib.hxx"

class BilinearBlitter : public Blitter {

  public:

    BilinearBlitter(FrameBufferSDL2& fb, bool interpolate);

    virtual ~BilinearBlitter();

    virtual void reinitialize(
      SDL_Rect srcRect,
      SDL_Rect destRect,
      FBSurface::Attributes attributes,
      SDL_Surface* staticData = nullptr
    ) override;

    virtual void blit(SDL_Surface& surface) override;

  private:

    SDL_Texture* myTexture;
    SDL_Texture* mySecondaryTexture;
    SDL_Rect mySrcRect, myDstRect;
    FBSurface::Attributes myAttributes;

    bool myInterpolate;
    bool myTexturesAreAllocated;
    bool myRecreateTextures;

    SDL_Surface* myStaticData;

    FrameBufferSDL2& myFB;

  private:

    void free();

    void recreateTexturesIfNecessary();

  private:

    BilinearBlitter(const BilinearBlitter&) = delete;

    BilinearBlitter(BilinearBlitter&&) = delete;

    BilinearBlitter& operator=(const BilinearBlitter&) = delete;

    BilinearBlitter& operator=(BilinearBlitter&&) = delete;
};

#endif // BILINEAR_BLITTER_HXX
