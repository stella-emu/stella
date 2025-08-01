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

#ifndef QIS_BLITTER_HXX
#define QIS_BLITTER_HXX

class FBBackendSDL;

#include "Blitter.hxx"
#include "SDL_lib.hxx"

class QisBlitter : public Blitter {

  public:

    explicit QisBlitter(FBBackendSDL& fb);

    static bool isSupported(const FBBackendSDL& fb);

    ~QisBlitter() override;

    void reinitialize(
      SDL_Rect srcRect, SDL_Rect destRect, bool enableBlend,
      uInt8 blendLevel, SDL_Surface* staticData
    ) override;

    void blit(SDL_Surface& surface) override;

  private:

    FBBackendSDL& myFB;

    SDL_Texture* mySrcTexture{nullptr};
    SDL_Texture* mySecondarySrcTexture{nullptr};
    SDL_Texture* myIntermediateTexture{nullptr};
    SDL_Texture* mySecondaryIntermediateTexture{nullptr};

    SDL_Rect mySrcRect{0, 0, 0, 0},
             myIntermediateRect{0, 0, 0, 0},
             myDstRect{0, 0, 0, 0};
    SDL_FRect mySrcFRect{0.F, 0.F, 0.F, 0.F},
              myIntermediateFRect{0.F, 0.F, 0.F, 0.F},
              myDstFRect{0.F, 0.F, 0.F, 0.F};

    uInt32 myBlendLevel{100};
    bool myEnableBlend{false};
    bool myTexturesAreAllocated{false};
    bool myRecreateTextures{false};

    SDL_Surface* myStaticData{nullptr};

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
