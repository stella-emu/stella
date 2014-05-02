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

#ifndef FB_SURFACE_TIA_HXX
#define FB_SURFACE_TIA_HXX

#include "bspf.hxx"
#include "FrameBuffer.hxx"
#include "FrameBufferSDL2.hxx"

/**
  A surface suitable for SDL Render2D API and rendering from a TIA source.
  It doesn't implement most of the drawing primitives, since it's concerned
  with TIA images only.  This class extends FrameBuffer::FBSurface.

  @author  Stephen Anthony
*/
class FBSurfaceTIA : public FBSurface
{
  friend class FrameBufferSDL2;

  public:
    FBSurfaceTIA(FrameBufferSDL2& buffer);
    virtual ~FBSurfaceTIA();

    // TIA surfaces don't implement most of the drawing primitives,
    // only the methods absolutely necessary for dealing with drawing
    // a TIA image
    void getPos(uInt32& x, uInt32& y) const;
    uInt32 getWidth()  const { return mySrcR.w; }
    uInt32 getHeight() const { return mySrcR.h; }
    void translateCoords(Int32& x, Int32& y) const;
    void render();
    void invalidate();
    void free();
    void reload();

  private:
    void setTIA(const TIA& tia) { myTIA = &tia; }
    void setTIAPalette(const uInt32* palette);
    void enableScanlines(bool enable) { myScanlinesEnabled = enable; }
    void setScanIntensity(uInt32 intensity) { myScanlineIntensity = intensity; }
    void setTexInterpolation(bool enable) { myTexFilter[0] = enable; }
    void setScanInterpolation(bool enable) { myTexFilter[1] = enable; }
    void updateCoords(uInt32 baseH, uInt32 imgX, uInt32 imgY, uInt32 imgW, uInt32 imgH);
    void updateCoords();

  private:
    FrameBufferSDL2& myFB;
    const TIA* myTIA;

    SDL_Surface* mySurface;
    SDL_Texture* myTexture;
    SDL_Texture* myScanlines;
    SDL_Rect mySrcR, myDstR, myScanR;
    uInt32 myPitch;

    bool myScanlinesEnabled;
    uInt32* myScanData;
    uInt32 myScanlineIntensity;

    bool myTexFilter[2];
};

#endif
