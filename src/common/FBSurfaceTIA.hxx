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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef FB_SURFACE_TIA_HXX
#define FB_SURFACE_TIA_HXX

#ifdef DISPLAY_OPENGL

#include "bspf.hxx"
#include "FrameBuffer.hxx"
#include "FrameBufferGL.hxx"
#include "NTSCFilter.hxx"

/**
  A surface suitable for OpenGL rendering mode, but specifically for
  rendering from a TIA source.  It doesn't implement most of the
  drawing primitives, since it's concerned with TIA images only.
  This class extends FrameBuffer::FBSurface.

  @author  Stephen Anthony
*/
class FBSurfaceTIA : public FBSurface
{
  friend class FrameBufferGL;

  public:
    FBSurfaceTIA(FrameBufferGL& buffer, uInt32 baseWidth, uInt32 baseHeight,
                 uInt32 imgX, uInt32 imgY, uInt32 imgW, uInt32 imgH);
    virtual ~FBSurfaceTIA();

    // TIA surfaces don't implement most of the drawing primitives,
    // only the methods absolutely necessary for dealing with drawing
    // a TIA image
    void getPos(uInt32& x, uInt32& y) const;
    uInt32 getWidth()  const { return myImageW; }
    uInt32 getHeight() const { return myImageH; }
    void translateCoords(Int32& x, Int32& y) const;
    void update();
    void free();
    void reload();

  private:
    void setTIA(const TIA& tia) { myTIA = &tia; }
    void setTIAPalette(const uInt32* palette);
    void setFilter(const string& name);
    void updateCoords();

  private:
    FrameBufferGL& myFB;
    const FrameBufferGL::GLpointers& myGL;
    const TIA* myTIA;
    SDL_Surface* myTexture;
    NTSCFilter myNTSCFilter;
    uInt32 myPitch;

    GLuint  myTexID[2], myVBOID;
    GLsizei myTexWidth;
    GLsizei myTexHeight;
    GLuint  myImageX, myImageY, myImageW, myImageH;
    GLfloat myTexCoordW, myTexCoordH;
    GLfloat myCoord[32];
};

#endif  // DISPLAY_OPENGL

#endif
