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

#ifndef QIS_BLITTER_HXX
#define QIS_BLITTER_HXX

#include "Blitter.hxx"
#include "FrameBufferSDL2.hxx"
#include "SDL_lib.hxx"

class QisBlitter : public Blitter {

  public:

    explicit QisBlitter(FrameBufferSDL2& fb);

    static bool isSupported(FrameBufferSDL2 &fb);

    virtual ~QisBlitter();

    virtual void reinitialize(
      SDL_Rect srcRect,
      SDL_Rect destRect,
      FBSurface::Attributes attributes,
      SDL_Surface* staticData = nullptr
    ) override;

    virtual void blit(SDL_Surface& surface) override;

  private:

    SDL_Texture* mySrcTexture;
    SDL_Texture* myIntermediateTexture;
    SDL_Texture* mySecondaryIntermedateTexture;

    SDL_Rect mySrcRect, myIntermediateRect, myDstRect;
    FBSurface::Attributes myAttributes;

    bool myTexturesAreAllocated;
    bool myRecreateTextures;

    SDL_Surface* myStaticData;
    unique_ptr<uInt32[]> myBlankBuffer;

    FrameBufferSDL2& myFB;

  private:

    void free();

    void recreateTexturesIfNecessary();

    void blitToIntermediate();

  private:

    QisBlitter(const QisBlitter&) = delete;

    QisBlitter(QisBlitter&&) = delete;

    QisBlitter& operator=(const QisBlitter&) = delete;

    QisBlitter& operator=(QisBlitter&&) = delete;
};

#endif // QIS_BLITTER_HXX
