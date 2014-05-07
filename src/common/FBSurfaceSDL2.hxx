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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef FBSURFACE_SDL2_HXX
#define FBSURFACE_SDL2_HXX

#include "bspf.hxx"
#include "FrameBufferSDL2.hxx"

/**
  An FBSurface suitable for the SDL2 Render2D API, making use of hardware
  acceleration behind the scenes.

  @author  Stephen Anthony
*/
class FBSurfaceSDL2 : public FBSurface
{
  public:
    FBSurfaceSDL2(FrameBufferSDL2& buffer, uInt32 width, uInt32 height);

    virtual ~FBSurfaceSDL2();

    // Most of the surface drawing primitives are implemented in FBSurface;
    // the ones implemented here use SDL-specific code for extra performance
    //
    void fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, uInt32 color);
    void drawSurface(const FBSurface* surface, uInt32 x, uInt32 y);
    void addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h);

    void setStaticContents(const uInt32* pixels, uInt32 pitch);
    void setInterpolationAndBlending(bool smoothScale, bool useBlend,
                                     uInt32 blendAlpha);

    const GUI::Rect& srcRect();
    const GUI::Rect& dstRect();
    void setSrcPos(uInt32 x, uInt32 y);
    void setSrcSize(uInt32 w, uInt32 h);
    void setDstPos(uInt32 x, uInt32 y);
    void setDstSize(uInt32 w, uInt32 h);

    void translateCoords(Int32& x, Int32& y) const;
    void render();
    void invalidate();
    void free();
    void reload();

  private:
    FrameBufferSDL2& myFB;

    SDL_Surface* mySurface;
    SDL_Texture* myTexture;
    SDL_Rect mySrcR, myDstR;

    bool mySurfaceIsDirty;

    bool myDataIsStatic;  // Is pixel data constant or can it change?
    bool myInterpolate;   // Scaling is smoothed or blocky
    bool myBlendEnabled;  // Blending is enabled
    uInt8 myBlendAlpha;   // Alpha to use in blending mode

    uInt32* myStaticData; // The data to use when the buffer contents are static
    uInt32 myStaticPitch; // The number of bytes in a row of static data

    GUI::Rect mySrcGUIR, myDstGUIR;
};

#endif
