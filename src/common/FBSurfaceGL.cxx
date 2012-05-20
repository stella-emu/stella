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

#include "Font.hxx"
#include "FrameBufferGL.hxx"

#include "FBSurfaceGL.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceGL::FBSurfaceGL(FrameBufferGL& buffer, uInt32 width, uInt32 height)
  : myFB(buffer),
    myGL(myFB.p_gl),
    myTexture(NULL),
    myTexID(0),
    myVBOID(0),
    myImageX(0),
    myImageY(0),
    myImageW(width),
    myImageH(height)
{
  // Fill buffer struct with valid data
  myTexWidth  = FrameBufferGL::power_of_two(myImageW);
  myTexHeight = FrameBufferGL::power_of_two(myImageH);
  myTexCoordW = (GLfloat) myImageW / myTexWidth;
  myTexCoordH = (GLfloat) myImageH / myTexHeight;

  // Create a surface in the same format as the parent GL class
  const SDL_PixelFormat& pf = myFB.myPixelFormat;
  myTexture = SDL_CreateRGBSurface(SDL_SWSURFACE, myTexWidth, myTexHeight,
                  pf.BitsPerPixel, pf.Rmask, pf.Gmask, pf.Bmask, pf.Amask);

  myPitch = myTexture->pitch / pf.BytesPerPixel;

  // Associate the SDL surface with a GL texture object
  updateCoords();
  reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceGL::~FBSurfaceGL()
{
  if(myTexture)
    SDL_FreeSurface(myTexture);

  free();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::hLine(uInt32 x, uInt32 y, uInt32 x2, uInt32 color)
{
  uInt32* buffer = (uInt32*) myTexture->pixels + y * myPitch + x;
  while(x++ <= x2)
    *buffer++ = (uInt32) myFB.myDefPalette[color];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::vLine(uInt32 x, uInt32 y, uInt32 y2, uInt32 color)
{
  uInt32* buffer = (uInt32*) myTexture->pixels + y * myPitch + x;
  while(y++ <= y2)
  {
    *buffer = (uInt32) myFB.myDefPalette[color];
    buffer += myPitch;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, uInt32 color)
{
  // Fill the rectangle
  SDL_Rect tmp;
  tmp.x = x;
  tmp.y = y;
  tmp.w = w;
  tmp.h = h;
  SDL_FillRect(myTexture, &tmp, myFB.myDefPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::drawChar(const GUI::Font& font, uInt8 chr,
                           uInt32 tx, uInt32 ty, uInt32 color)
{
  const FontDesc& desc = font.desc();

  // If this character is not included in the font, use the default char.
  if(chr < desc.firstchar || chr >= desc.firstchar + desc.size)
  {
    if (chr == ' ') return;
    chr = desc.defaultchar;
  }
  chr -= desc.firstchar;

  // Get the bounding box of the character
  int bbw, bbh, bbx, bby;
  if(!desc.bbx)
  {
    bbw = desc.fbbw;
    bbh = desc.fbbh;
    bbx = desc.fbbx;
    bby = desc.fbby;
  }
  else
  {
    bbw = desc.bbx[chr].w;
    bbh = desc.bbx[chr].h;
    bbx = desc.bbx[chr].x;
    bby = desc.bbx[chr].y;
  }

  const uInt16* tmp = desc.bits + (desc.offset ? desc.offset[chr] : (chr * desc.fbbh));
  uInt32* buffer = (uInt32*) myTexture->pixels +
                   (ty + desc.ascent - bby - bbh) * myPitch +
                   tx + bbx;

  for(int y = 0; y < bbh; y++)
  {
    const uInt16 ptr = *tmp++;
    uInt16 mask = 0x8000;

    for(int x = 0; x < bbw; x++, mask >>= 1)
      if(ptr & mask)
        buffer[x] = (uInt32) myFB.myDefPalette[color];

    buffer += myPitch;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::drawBitmap(uInt32* bitmap, uInt32 tx, uInt32 ty,
                             uInt32 color, uInt32 h)
{
  uInt32* buffer = (uInt32*) myTexture->pixels + ty * myPitch + tx;

  for(uInt32 y = 0; y < h; ++y)
  {
    uInt32 mask = 0xF0000000;
    for(uInt32 x = 0; x < 8; ++x, mask >>= 4)
      if(bitmap[y] & mask)
        buffer[x] = (uInt32) myFB.myDefPalette[color];

    buffer += myPitch;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::drawPixels(uInt32* data, uInt32 tx, uInt32 ty, uInt32 numpixels)
{
  uInt32* buffer = (uInt32*) myTexture->pixels + ty * myPitch + tx;

  for(uInt32 i = 0; i < numpixels; ++i)
    *buffer++ = (uInt32) data[i];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::drawSurface(const FBSurface* surface, uInt32 tx, uInt32 ty)
{
  const FBSurfaceGL* s = (const FBSurfaceGL*) surface;

  SDL_Rect dstrect;
  dstrect.x = tx;
  dstrect.y = ty;
  SDL_Rect srcrect;
  srcrect.x = 0;
  srcrect.y = 0;
  srcrect.w = s->myImageW;
  srcrect.h = s->myImageH;

  SDL_BlitSurface(s->myTexture, &srcrect, myTexture, &dstrect);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h)
{
  // OpenGL mode doesn't make use of dirty rectangles
  // It's faster to just update the entire surface
  mySurfaceIsDirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::getPos(uInt32& x, uInt32& y) const
{
  x = myImageX;
  y = myImageY;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::setPos(uInt32 x, uInt32 y)
{
  if(myImageX != x || myImageY != y)
  {
    myImageX = x;
    myImageY = y;
    updateCoords();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::setWidth(uInt32 w)
{
  // This method can't be used with 'scaled' surface (aka TIA surfaces)
  // That shouldn't really matter, though, as all the UI stuff isn't scaled,
  // and it's the only thing that uses it
  if(myImageW != w)
  {
    myImageW = w;
    myTexCoordW = (GLfloat) myImageW / myTexWidth;
    updateCoords();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::setHeight(uInt32 h)
{
  // This method can't be used with 'scaled' surface (aka TIA surfaces)
  // That shouldn't really matter, though, as all the UI stuff isn't scaled,
  // and it's the only thing that uses it
  if(myImageH != h)
  {
    myImageH = h;
    myTexCoordH = (GLfloat) myImageH / myTexHeight;
    updateCoords();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::translateCoords(Int32& x, Int32& y) const
{
  x -= myImageX;
  y -= myImageY;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::update()
{
  if(mySurfaceIsDirty)
  {
    // Texturemap complete texture to surface so we have free scaling
    // and antialiasing
    myGL.ActiveTexture(GL_TEXTURE0);
    myGL.BindTexture(GL_TEXTURE_2D, myTexID);
    myGL.TexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, myTexWidth, myTexHeight,
                       GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                       myTexture->pixels);

    myGL.EnableClientState(GL_VERTEX_ARRAY);
    myGL.EnableClientState(GL_TEXTURE_COORD_ARRAY);
    if(myFB.myVBOAvailable)
    {
      myGL.BindBuffer(GL_ARRAY_BUFFER, myVBOID);
      myGL.VertexPointer(2, GL_FLOAT, 0, (const GLvoid*)0);
      myGL.TexCoordPointer(2, GL_FLOAT, 0, (const GLvoid*)(8*sizeof(GLfloat)));
    }
    else
    {
      myGL.VertexPointer(2, GL_FLOAT, 0, myCoord);
      myGL.TexCoordPointer(2, GL_FLOAT, 0, myCoord+8);
    }
    myGL.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    myGL.DisableClientState(GL_VERTEX_ARRAY);
    myGL.DisableClientState(GL_TEXTURE_COORD_ARRAY);

    mySurfaceIsDirty = false;

    // Let postFrameUpdate() know that a change has been made
    myFB.myDirtyFlag = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::free()
{
  myGL.DeleteTextures(1, &myTexID);
  if(myFB.myVBOAvailable)
    myGL.DeleteBuffers(1, &myVBOID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::reload()
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
  myGL.GenTextures(1, &myTexID);
  myGL.BindTexture(GL_TEXTURE_2D, myTexID);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  myGL.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Create the texture in the most optimal format
  myGL.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, myTexWidth, myTexHeight, 0,
                  GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                  myTexture->pixels);

  // Cache vertex and texture coordinates using vertex buffer object
  if(myFB.myVBOAvailable)
  {
    myGL.GenBuffers(1, &myVBOID);
    myGL.BindBuffer(GL_ARRAY_BUFFER, myVBOID);
    myGL.BufferData(GL_ARRAY_BUFFER, 16*sizeof(GLfloat), myCoord, GL_STATIC_DRAW);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::updateCoords()
{
  // Vertex coordinates for texture
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

  // Texture coordinates for texture
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

  // Cache vertex and texture coordinates using vertex buffer object
  if(myFB.myVBOAvailable)
  {
    myGL.BindBuffer(GL_ARRAY_BUFFER, myVBOID);
    myGL.BufferData(GL_ARRAY_BUFFER, 16*sizeof(GLfloat), myCoord, GL_STATIC_DRAW);
  }
}

#endif
