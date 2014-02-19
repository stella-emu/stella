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

#include <cmath>

#include "Font.hxx"
#include "FrameBufferSDL2.hxx"
#include "TIA.hxx"
#include "NTSCFilter.hxx"

#include "FBSurfaceTIA.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceTIA::FBSurfaceTIA(FrameBufferSDL2& buffer)
  : myFB(buffer),
    mySurface(NULL),
    myTexture(NULL),
    myScanlinesEnabled(false)
//    myScanlineIntensityI(50),
//    myScanlineIntensityF(0.5)
{
  // Texture width is set to contain all possible sizes for a TIA image,
  // including Blargg filtering
  int width = ATARI_NTSC_OUT_WIDTH(160);
  int height = 320;

  // Create a surface in the same format as the parent GL class
  const SDL_PixelFormat& pf = myFB.myPixelFormat;

  mySurface = SDL_CreateRGBSurface(0, width, height,
      pf.BitsPerPixel, pf.Rmask, pf.Gmask, pf.Bmask, pf.Amask);

  mySrc.x = mySrc.y = myDst.x = myDst.y = 0;
  mySrc.w = myDst.w = width;
  mySrc.h = myDst.h = height;

  myPitch = mySurface->pitch / pf.BytesPerPixel;

  // To generate texture
  reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceTIA::~FBSurfaceTIA()
{
  if(mySurface)
    SDL_FreeSurface(mySurface);

  free();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::getPos(uInt32& x, uInt32& y) const
{
  x = mySrc.x;
  y = mySrc.y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::translateCoords(Int32& x, Int32& y) const
{
  x = mySrc.x;
  y = mySrc.y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::update()
{
  // Copy the mediasource framebuffer to the RGB texture
  // In hardware rendering mode, it's faster to just assume that the screen
  // is dirty and always do an update

  uInt8* currentFrame  = myTIA->currentFrameBuffer();
  uInt8* previousFrame = myTIA->previousFrameBuffer();
  uInt32 width         = myTIA->width();
  uInt32 height        = myTIA->height();
  uInt32* buffer       = (uInt32*) mySurface->pixels;

  // TODO - Eventually 'phosphor' won't be a separate mode, and will become
  //        a post-processing filter by blending several frames.
  switch(myFB.myFilterType)
  {
    case FrameBufferSDL2::kNormal:
    {
      uInt32 bufofsY    = 0;
      uInt32 screenofsY = 0;
      for(uInt32 y = 0; y < height; ++y)
      {
        uInt32 pos = screenofsY;
        for(uInt32 x = 0; x < width; ++x)
          buffer[pos++] = (uInt32) myFB.myDefPalette[currentFrame[bufofsY + x]];

        bufofsY    += width;
        screenofsY += myPitch;
      }
      break;
    }
    case FrameBufferSDL2::kPhosphor:
    {
      uInt32 bufofsY    = 0;
      uInt32 screenofsY = 0;
      for(uInt32 y = 0; y < height; ++y)
      {
        uInt32 pos = screenofsY;
        for(uInt32 x = 0; x < width; ++x)
        {
          const uInt32 bufofs = bufofsY + x;
          buffer[pos++] = (uInt32)
            myFB.myAvgPalette[currentFrame[bufofs]][previousFrame[bufofs]];
        }
        bufofsY    += width;
        screenofsY += myPitch;
      }
      break;
    }
    case FrameBufferSDL2::kBlarggNormal:
    {
      myFB.myNTSCFilter.blit_single(currentFrame, width, height,
                                    buffer, mySurface->pitch);
      break;
    }
    case FrameBufferSDL2::kBlarggPhosphor:
    {
      myFB.myNTSCFilter.blit_double(currentFrame, previousFrame, width, height,
                                    buffer, mySurface->pitch);
      break;
    }
  }

SDL_Rect tmp;
tmp.x = tmp.y = 0;
tmp.w = 160;  tmp.h = 210;
  SDL_UpdateTexture(myTexture, NULL, mySurface->pixels, mySurface->pitch);
  SDL_RenderCopy(myFB.myRenderer, myTexture, &mySrc, &myDst);


#if 0
  myGL.EnableClientState(GL_VERTEX_ARRAY);
  myGL.EnableClientState(GL_TEXTURE_COORD_ARRAY);

  // Update TIA image (texture 0), then blend scanlines (texture 1)
  myGL.ActiveTexture(GL_TEXTURE0);
  myGL.BindTexture(GL_TEXTURE_2D, myTexID[0]);
  myGL.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
  myGL.PixelStorei(GL_UNPACK_ROW_LENGTH, myPitch);
  myGL.TexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, myBaseW, myBaseH,
                    GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                    mySurface->pixels);

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
#endif

  // Let postFrameUpdate() know that a change has been made
  myFB.myDirtyFlag = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::invalidate()
{
  SDL_FillRect(mySurface, NULL, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::free()
{
  if(myTexture)
  {
    SDL_DestroyTexture(myTexture);
    myTexture = NULL;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::reload()
{
  // Re-create texture; the underlying SDL_Surface is fine as-is
  myTexture = SDL_CreateTexture(myFB.myRenderer,
      SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
      mySurface->w, mySurface->h);
#if 0
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
  myGL.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
  myGL.PixelStorei(GL_UNPACK_ROW_LENGTH, myPitch);
  myGL.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, myTexWidth, myTexHeight, 0,
                 GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                 mySurface->pixels);

  // Scanline texture (@ index 1)
  myGL.BindTexture(GL_TEXTURE_2D, myTexID[1]);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, myTexFilter[1]);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, myTexFilter[1]);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  static uInt32 const scanline[2] = { 0x00000000, 0xff000000 };
  myGL.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
  myGL.PixelStorei(GL_UNPACK_ROW_LENGTH, 1);
  myGL.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 2, 0,
                 GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                 scanline);

  // Cache vertex and texture coordinates using vertex buffer object
  if(myFB.myVBOAvailable)
  {
    myGL.GenBuffers(1, &myVBOID);
    myGL.BindBuffer(GL_ARRAY_BUFFER, myVBOID);
    myGL.BufferData(GL_ARRAY_BUFFER, 32*sizeof(GLfloat), myCoord, GL_STATIC_DRAW);
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::setScanIntensity(uInt32 intensity)
{
#if 0
  myScanlineIntensityI = (GLuint)intensity;
  myScanlineIntensityF = (GLfloat)intensity / 100;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::setTexInterpolation(bool enable)
{
#if 0
  myTexFilter[0] = enable ? GL_LINEAR : GL_NEAREST;
  myGL.BindTexture(GL_TEXTURE_2D, myTexID[0]);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, myTexFilter[0]);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, myTexFilter[0]);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::setScanInterpolation(bool enable)
{
#if 0
  myTexFilter[1] = enable ? GL_LINEAR : GL_NEAREST;
  myGL.BindTexture(GL_TEXTURE_2D, myTexID[1]);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, myTexFilter[1]);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, myTexFilter[1]);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::updateCoords(uInt32 baseH,
     uInt32 imgX, uInt32 imgY, uInt32 imgW, uInt32 imgH)
{
//cerr << "baseH=" << baseH << ", x=" << imgX << ", y=" << imgY << ", w=" << imgW << ", h=" << imgH << endl;

  mySrc.w = myFB.ntscEnabled() ? ATARI_NTSC_OUT_WIDTH(160) : 160;
  mySrc.h = baseH;

  myDst.w = imgW;
  myDst.h = imgH;

//cerr << "src: x=" << mySrc.x << ", y=" << mySrc.y << ", w=" << mySrc.w << ", h=" << mySrc.h << endl;
//cerr << "dst: x=" << myDst.x << ", y=" << myDst.y << ", w=" << myDst.w << ", h=" << myDst.h << endl;

#if 0
  myBaseH = baseH;
  myImageX = imgX;  myImageY = imgY;
  myImageW = imgW;  myImageH = imgH;

  updateCoords();
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::updateCoords()
{
#if 0
  // Normal TIA rendering and TV effects use different widths
  // We use the same buffer, and only pick the width we need
  myBaseW = myFB.ntscEnabled() ? ATARI_NTSC_OUT_WIDTH(160) : 160;

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
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceTIA::setTIAPalette(const uInt32* palette)
{
  myFB.myNTSCFilter.setTIAPalette(myFB, palette);
}
