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

#include "Font.hxx"
#include "FBSurfaceUI.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceUI::FBSurfaceUI(FrameBufferSDL2& buffer, uInt32 width, uInt32 height)
  : myFB(buffer),
    mySurface(NULL),
    myTexture(NULL),
    mySurfaceIsDirty(true)
{
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
FBSurfaceUI::~FBSurfaceUI()
{
  if(mySurface)
    SDL_FreeSurface(mySurface);

  free();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceUI::hLine(uInt32 x, uInt32 y, uInt32 x2, uInt32 color)
{
  uInt32* buffer = (uInt32*) mySurface->pixels + y * myPitch + x;
  while(x++ <= x2)
    *buffer++ = (uInt32) myFB.myDefPalette[color];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceUI::vLine(uInt32 x, uInt32 y, uInt32 y2, uInt32 color)
{
  uInt32* buffer = (uInt32*) mySurface->pixels + y * myPitch + x;
  while(y++ <= y2)
  {
    *buffer = (uInt32) myFB.myDefPalette[color];
    buffer += myPitch;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceUI::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, uInt32 color)
{
  // Fill the rectangle
  SDL_Rect tmp;
  tmp.x = x;
  tmp.y = y;
  tmp.w = w;
  tmp.h = h;
  SDL_FillRect(mySurface, &tmp, myFB.myDefPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceUI::drawChar(const GUI::Font& font, uInt8 chr,
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
  uInt32* buffer = (uInt32*) mySurface->pixels +
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
void FBSurfaceUI::drawBitmap(uInt32* bitmap, uInt32 tx, uInt32 ty,
                             uInt32 color, uInt32 h)
{
  uInt32* buffer = (uInt32*) mySurface->pixels + ty * myPitch + tx;

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
void FBSurfaceUI::drawPixels(uInt32* data, uInt32 tx, uInt32 ty, uInt32 numpixels)
{
  uInt32* buffer = (uInt32*) mySurface->pixels + ty * myPitch + tx;

  for(uInt32 i = 0; i < numpixels; ++i)
    *buffer++ = (uInt32) data[i];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceUI::drawSurface(const FBSurface* surface, uInt32 tx, uInt32 ty)
{
  const FBSurfaceUI* s = (const FBSurfaceUI*) surface;

  SDL_Rect dst;
  dst.x = tx;
  dst.y = ty;
  dst.w = s->mySrc.w;
  dst.h = s->mySrc.h;

  SDL_BlitSurface(s->mySurface, &(s->mySrc), mySurface, &dst);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceUI::addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h)
{
  // It's faster to just update the entire (hardware) surface
  mySurfaceIsDirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceUI::getPos(uInt32& x, uInt32& y) const
{
  x = myDst.x;
  y = myDst.y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceUI::setPos(uInt32 x, uInt32 y)
{
  myDst.x = x;
  myDst.y = y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceUI::setWidth(uInt32 w)
{
  // This method can't be used with 'scaled' surface (aka TIA surfaces)
  // That shouldn't really matter, though, as all the UI stuff isn't scaled,
  // and it's the only thing that uses it
  mySrc.w = w;
  myDst.w = w;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceUI::setHeight(uInt32 h)
{
  // This method can't be used with 'scaled' surface (aka TIA surfaces)
  // That shouldn't really matter, though, as all the UI stuff isn't scaled,
  // and it's the only thing that uses it
  mySrc.h = h;
  myDst.h = h;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceUI::translateCoords(Int32& x, Int32& y) const
{
  x -= myDst.x;
  y -= myDst.y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceUI::update()
{
  if(mySurfaceIsDirty)
  {
//cerr << "src: x=" << mySrc.x << ", y=" << mySrc.y << ", w=" << mySrc.w << ", h=" << mySrc.h << endl;
//cerr << "dst: x=" << myDst.x << ", y=" << myDst.y << ", w=" << myDst.w << ", h=" << myDst.h << endl;

    SDL_UpdateTexture(myTexture, &mySrc, mySurface->pixels, mySurface->pitch);
    SDL_RenderCopy(myFB.myRenderer, myTexture, &mySrc, &myDst);

    mySurfaceIsDirty = false;

    // Let postFrameUpdate() know that a change has been made
    myFB.myDirtyFlag = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceUI::invalidate()
{
  SDL_FillRect(mySurface, NULL, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceUI::free()
{
  if(myTexture)
  {
    SDL_DestroyTexture(myTexture);
    myTexture = NULL;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceUI::reload()
{
  // Re-create texture; the underlying SDL_Surface is fine as-is
  myTexture = SDL_CreateTexture(myFB.myRenderer,
      SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
      mySurface->w, mySurface->h);
}
