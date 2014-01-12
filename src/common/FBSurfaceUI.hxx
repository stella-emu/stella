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

#ifndef FB_SURFACE_UI_HXX
#define FB_SURFACE_UI_HXX

#include "bspf.hxx"
#include "FrameBufferSDL2.hxx"

/**
  A surface suitable for OpenGL rendering mode, used for various UI dialogs.
  This class extends FrameBuffer::FBSurface.

  @author  Stephen Anthony
*/
class FBSurfaceUI : public FBSurface
{
  friend class FrameBufferSDL2;

  public:
    FBSurfaceUI(FrameBufferSDL2& buffer, uInt32 width, uInt32 height);
    virtual ~FBSurfaceUI();

    // Normal surfaces need all drawing primitives
    void hLine(uInt32 x, uInt32 y, uInt32 x2, uInt32 color);
    void vLine(uInt32 x, uInt32 y, uInt32 y2, uInt32 color);
    void fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, uInt32 color);
    void drawChar(const GUI::Font& font, uInt8 c, uInt32 x, uInt32 y, uInt32 color);
    void drawBitmap(uInt32* bitmap, uInt32 x, uInt32 y, uInt32 color, uInt32 h = 8);
    void drawPixels(uInt32* data, uInt32 x, uInt32 y, uInt32 numpixels);
    void drawSurface(const FBSurface* surface, uInt32 x, uInt32 y);
    void addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h);
    void getPos(uInt32& x, uInt32& y) const;
    void setPos(uInt32 x, uInt32 y);
    uInt32 getWidth()  const { return myImageW; }
    uInt32 getHeight() const { return myImageH; }
    void setWidth(uInt32 w);
    void setHeight(uInt32 h);
    void translateCoords(Int32& x, Int32& y) const;
    void update();
    void free();
    void reload();

  private:
    void updateCoords();

  private:
    FrameBufferSDL2& myFB;
    const FrameBufferSDL2::GLpointers& myGL;
    SDL_Surface* myTexture;

    GLuint  myTexID, myVBOID;
    GLsizei myTexWidth;
    GLsizei myTexHeight;
    GLuint  myImageX, myImageY, myImageW, myImageH;
    GLfloat myTexCoordW, myTexCoordH;
    GLfloat myCoord[16];

    bool mySurfaceIsDirty;
    uInt32 myPitch;
};

#endif
