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

#ifdef DISPLAY_OPENGL

#include <cmath>

#include "Font.hxx"
#include "FrameBufferGL.hxx"
#include "TIA.hxx"
#include "NTSCFilter.hxx"

#include "FBSurfaceTIA.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceTIA::FBSurfaceTIA(FrameBufferGL& buffer)
  : myFB(buffer),
    myGL(myFB.p_gl),
    myTexture(NULL),
    myVBOID(0),
    myScanlinesEnabled(false),
    myScanlineIntensityI(50),
    myScanlineIntensityF(0.5)
{
  myTexID[0] = myTexID[1] = 0;

  // Texture width is set to contain all possible sizes for a TIA image,
  // including Blargg filtering
  myTexWidth  = FrameBufferGL::power_of_two(ATARI_NTSC_OUT_WIDTH(160));
  myTexHeight = FrameBufferGL::power_of_two(320);

  // Based on experimentation, the following are the fastest 16-bit
  // formats for OpenGL (on all platforms)
  myTexture = SDL_CreateRGBSurface(SDL_SWSURFACE, myTexWidth, myTexHeight, 16,
                  myFB.myPixelFormat.Rmask, myFB.myPixelFormat.Gmask,
                  myFB.myPixelFormat.Bmask, 0x00000000);

  myPitch = myTexture->pitch >> 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceTIA::~FBSurfaceTIA()
{
  if(myTexture)
    SDL_FreeSurface(myTexture);

  free();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::getPos(uInt32& x, uInt32& y) const
{
  x = myImageX;
  y = myImageY;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::translateCoords(Int32& x, Int32& y) const
{
  x -= myImageX;
  y -= myImageY;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::update()
{
  // Copy the mediasource framebuffer to the RGB texture
  // In OpenGL mode, it's faster to just assume that the screen is dirty
  // and always do an update

  uInt8* currentFrame  = myTIA->currentFrameBuffer();
  uInt8* previousFrame = myTIA->previousFrameBuffer();
  uInt32 width         = myTIA->width();
  uInt32 height        = myTIA->height();
  uInt16* buffer       = (uInt16*) myTexture->pixels;

  // TODO - Eventually 'phosphor' won't be a separate mode, and will become
  //        a post-processing filter by blending several frames.
  switch(myFB.myFilterType)
  {
    case FrameBufferGL::kNone:
    {
      uInt32 bufofsY    = 0;
      uInt32 screenofsY = 0;
      for(uInt32 y = 0; y < height; ++y)
      {
        uInt32 pos = screenofsY;
        for(uInt32 x = 0; x < width; ++x)
          buffer[pos++] = (uInt16) myFB.myDefPalette[currentFrame[bufofsY + x]];

        bufofsY    += width;
        screenofsY += myPitch;
      }
      break;
    }

    case FrameBufferGL::kPhosphor:
    {
      uInt32 bufofsY    = 0;
      uInt32 screenofsY = 0;
      for(uInt32 y = 0; y < height; ++y)
      {
        uInt32 pos = screenofsY;
        for(uInt32 x = 0; x < width; ++x)
        {
          const uInt32 bufofs = bufofsY + x;
          buffer[pos++] = (uInt16)
            myFB.myAvgPalette[currentFrame[bufofs]][previousFrame[bufofs]];
        }
        bufofsY    += width;
        screenofsY += myPitch;
      }
      break;
    }

    case FrameBufferGL::kBlarggNTSC:
    {
      myFB.myNTSCFilter.blit_1555
        (currentFrame, width, height, buffer, myTexture->pitch);
      break;
    }
  }

  myGL.EnableClientState(GL_VERTEX_ARRAY);
  myGL.EnableClientState(GL_TEXTURE_COORD_ARRAY);

  // Update TIA image (texture 0), then blend scanlines (texture 1)
  myGL.ActiveTexture(GL_TEXTURE0);
  myGL.BindTexture(GL_TEXTURE_2D, myTexID[0]);
  myGL.TexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, myTexWidth, myTexHeight,
                    GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV,
                    myTexture->pixels);

  if(myFB.myVBOAvailable)
  {
    myGL.BindBuffer(GL_ARRAY_BUFFER, myVBOID);
    myGL.VertexPointer(2, GL_FLOAT, 0, (const GLvoid*)0);
    myGL.TexCoordPointer(2, GL_FLOAT, 0, (const GLvoid*)(8*sizeof(GLfloat)));
    myGL.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if(myScanlinesEnabled)
    {
      myGL.Enable(GL_BLEND);
      myGL.Color4f(1.0f, 1.0f, 1.0f, myScanlineIntensityF);
      myGL.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      myGL.BindTexture(GL_TEXTURE_2D, myTexID[1]);
      myGL.VertexPointer(2, GL_FLOAT, 0, (const GLvoid*)(16*sizeof(GLfloat)));
      myGL.TexCoordPointer(2, GL_FLOAT, 0, (const GLvoid*)(24*sizeof(GLfloat)));
      myGL.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      myGL.Disable(GL_BLEND);
    }
  }
  else
  {
    myGL.VertexPointer(2, GL_FLOAT, 0, myCoord);
    myGL.TexCoordPointer(2, GL_FLOAT, 0, myCoord+8);
    myGL.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if(myScanlinesEnabled)
    {
      myGL.Enable(GL_BLEND);
      myGL.Color4f(1.0f, 1.0f, 1.0f, myScanlineIntensityF);
      myGL.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      myGL.BindTexture(GL_TEXTURE_2D, myTexID[1]);
      myGL.VertexPointer(2, GL_FLOAT, 0, myCoord+16);
      myGL.TexCoordPointer(2, GL_FLOAT, 0, myCoord+24);
      myGL.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      myGL.Disable(GL_BLEND);
    }
  }

  myGL.DisableClientState(GL_VERTEX_ARRAY);
  myGL.DisableClientState(GL_TEXTURE_COORD_ARRAY);

  // Let postFrameUpdate() know that a change has been made
  myFB.myDirtyFlag = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::free()
{
  myGL.DeleteTextures(2, myTexID);
  if(myFB.myVBOAvailable)
    myGL.DeleteBuffers(1, &myVBOID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::reload()
{
  // This does a 'soft' reset of the surface
  // It seems that on some system (notably, OSX), creating a new SDL window
  // destroys the GL context, requiring a reload of all textures
  // However, destroying the entire FBSurfaceGL object is wasteful, since
  // it will also regenerate SDL software surfaces (which are not required
  // to be regenerated)
  // Basically, all that needs to be done is to re-call glTexImage2D with a
  // new texture ID, so that's what we do here

  myGL.ActiveTexture(GL_TEXTURE0);
  myGL.Enable(GL_TEXTURE_2D);

  // TIA surfaces also use a scanline texture
  myGL.GenTextures(2, myTexID);

  // Base texture (@ index 0)
  myGL.BindTexture(GL_TEXTURE_2D, myTexID[0]);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, myTexFilter[0]);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, myTexFilter[0]);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Create the texture in the most optimal format
  myGL.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, myTexWidth, myTexHeight, 0,
                 GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV,
                 myTexture->pixels);

  // Scanline texture (@ index 1)
  myGL.BindTexture(GL_TEXTURE_2D, myTexID[1]);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, myTexFilter[1]);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, myTexFilter[1]);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  static uInt16 const scanline[4] = { 0x0000, 0x0000, 0x8000, 0x0000  };
  myGL.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 2, 0,
                 GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV,
                 scanline);

  // Cache vertex and texture coordinates using vertex buffer object
  if(myFB.myVBOAvailable)
  {
    myGL.GenBuffers(1, &myVBOID);
    myGL.BindBuffer(GL_ARRAY_BUFFER, myVBOID);
    myGL.BufferData(GL_ARRAY_BUFFER, 32*sizeof(GLfloat), myCoord, GL_STATIC_DRAW);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::setScanIntensity(uInt32 intensity)
{
  myScanlineIntensityI = (GLuint)intensity;
  myScanlineIntensityF = (GLfloat)intensity / 100;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::setTexInterpolation(bool enable)
{
  myTexFilter[0] = enable ? GL_LINEAR : GL_NEAREST;
  myGL.BindTexture(GL_TEXTURE_2D, myTexID[0]);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, myTexFilter[0]);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, myTexFilter[0]);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::setScanInterpolation(bool enable)
{
  myTexFilter[1] = enable ? GL_LINEAR : GL_NEAREST;
  myGL.BindTexture(GL_TEXTURE_2D, myTexID[1]);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, myTexFilter[1]);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, myTexFilter[1]);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::updateCoords(uInt32 baseH,
     uInt32 imgX, uInt32 imgY, uInt32 imgW, uInt32 imgH)
{
  myBaseH = baseH;
  myImageX = imgX;  myImageY = imgY;
  myImageW = imgW;  myImageH = imgH;

  updateCoords();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::updateCoords()
{
  // Normal TIA rendering and TV effects use different widths
  // We use the same buffer, and only pick the width we need
  myBaseW = myFB.myFilterType == FrameBufferGL::kBlarggNTSC ?
      ATARI_NTSC_OUT_WIDTH(160) : 160;

  myTexCoordW = (GLfloat) myBaseW / myTexWidth;
  myTexCoordH = (GLfloat) myBaseH / myTexHeight;

  // Vertex coordinates for texture 0 (main texture)
  // Upper left (x,y)
  myCoord[0] = (GLfloat)myImageX;
  myCoord[1] = (GLfloat)myImageY;
  // Upper right (x+w,y)
  myCoord[2] = (GLfloat)(myImageX + myImageW);
  myCoord[3] = (GLfloat)myImageY;
  // Lower left (x,y+h)
  myCoord[4] = (GLfloat)myImageX;
  myCoord[5] = (GLfloat)(myImageY + myImageH);
  // Lower right (x+w,y+h)
  myCoord[6] = (GLfloat)(myImageX + myImageW);
  myCoord[7] = (GLfloat)(myImageY + myImageH);

  // Texture coordinates for texture 0 (main texture)
  // Upper left (x,y)
  myCoord[8] = 0.0f;
  myCoord[9] = 0.0f;
  // Upper right (x+w,y)
  myCoord[10] = myTexCoordW;
  myCoord[11] = 0.0f;
  // Lower left (x,y+h)
  myCoord[12] = 0.0f;
  myCoord[13] = myTexCoordH;
  // Lower right (x+w,y+h)
  myCoord[14] = myTexCoordW;
  myCoord[15] = myTexCoordH;

  // Vertex coordinates for texture 1 (scanline texture)
  // Upper left (x,y)
  myCoord[16] = (GLfloat)myImageX;
  myCoord[17] = (GLfloat)myImageY;
  // Upper right (x+w,y)
  myCoord[18] = (GLfloat)(myImageX + myImageW);
  myCoord[19] = (GLfloat)myImageY;
  // Lower left (x,y+h)
  myCoord[20] = (GLfloat)myImageX;
  myCoord[21] = (GLfloat)(myImageY + myImageH);
  // Lower right (x+w,y+h)
  myCoord[22] = (GLfloat)(myImageX + myImageW);
  myCoord[23] = (GLfloat)(myImageY + myImageH);

  // Texture coordinates for texture 1 (scanline texture)
  // Upper left (x,y)
  myCoord[24] = 0.0f;
  myCoord[25] = 0.0f;
  // Upper right (x+w,y)
  myCoord[26] = 1.0f;
  myCoord[27] = 0.0f;
  // Scanline repeating is sensitive to non-integral vertical resolution,
  // so rounding is performed to eliminate it
  // This won't be 100% accurate, but non-integral scaling isn't 100%
  // accurate anyway
  // Lower left (x,y+h)
  myCoord[28] = 0.0f;
  myCoord[29] = GLfloat(myImageH) / floor(((float)myImageH / myBaseH) + 0.5);
  // Lower right (x+w,y+h)
  myCoord[30] = 1.0f;
  myCoord[31] = myCoord[29];

  // Cache vertex and texture coordinates using vertex buffer object
  if(myFB.myVBOAvailable)
  {
    myGL.BindBuffer(GL_ARRAY_BUFFER, myVBOID);
    myGL.BufferData(GL_ARRAY_BUFFER, 32*sizeof(GLfloat), myCoord, GL_STATIC_DRAW);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::setTIAPalette(const uInt32* palette)
{
  myFB.myNTSCFilter.setTIAPalette(palette);
}

#endif
