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
  A surface suitable for SDL Render2D API, making use of hardware
  acceleration behind the scenes.  This class extends
  FrameBuffer::FBSurface.

  @author  Stephen Anthony
*/
class FBSurfaceSDL2 : public FBSurface
{
  friend class FrameBufferSDL2;

  public:
    FBSurfaceSDL2(FrameBufferSDL2& buffer, uInt32 width, uInt32 height);
    virtual ~FBSurfaceSDL2();

    // Most of the surface drawing primitives are defined in FBSurface;
    // the ones defined here use SDL-specific code
    //
    void fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, uInt32 color);
    void drawSurface(const FBSurface* surface, uInt32 x, uInt32 y);
    void addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h);
    void getPos(uInt32& x, uInt32& y) const;
    void setPos(uInt32 x, uInt32 y);
    uInt32 getWidth()  const { return mySrc.w; }
    uInt32 getHeight() const { return mySrc.h; }
    void setWidth(uInt32 w);
    void setHeight(uInt32 h);
    void translateCoords(Int32& x, Int32& y) const;
    void render();
    void invalidate();
    void free();
    void reload();

  private:
    FrameBufferSDL2& myFB;

    SDL_Surface* mySurface;
    SDL_Texture* myTexture;
    SDL_Rect mySrc, myDst;

    bool mySurfaceIsDirty;
};

#endif
