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
// Copyright (c) 1995-2006 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FrameBufferGP2X.cxx,v 1.13 2006-12-19 12:40:30 stephena Exp $
//============================================================================

#include <SDL.h>

#include "Console.hxx"
#include "MediaSrc.hxx"
#include "OSystem.hxx"
#include "Font.hxx"
#include "GuiUtils.hxx"
#include "FrameBufferGP2X.hxx"


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferGP2X::FrameBufferGP2X(OSystem* osystem)
  : FrameBuffer(osystem),
    myDirtyFlag(true)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferGP2X::~FrameBufferGP2X()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGP2X::initSubsystem()
{
  // Create the screen
  return createScreen();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FrameBufferGP2X::about()
{
  // TODO - add SDL info to this string
  return "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::setAspectRatio()
{
  // Aspect ratio correction not yet available in software mode
  theAspectRatio = 1.0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::setScaler(Scaler scaler)
{
  // Not supported, we always use 1x zoom
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGP2X::createScreen()
{
  // Make sure to clear the screen, since we're using different resolutions,
  // and there tends to be lingering artifacts in hardware mode
  if(myScreen)
  {
    SDL_FillRect(myScreen, NULL, 0);
    SDL_UpdateRect(myScreen, 0, 0, 0, 0);
  }

  myScreenDim.x = myScreenDim.y = 0;
  myScreenDim.w = myBaseDim.w;
  myScreenDim.h = myBaseDim.h;

  // In software mode, the image and screen dimensions are always the same
  myImageDim = myScreenDim;

  // The GP2X always uses a 16-bit hardware buffer
  myScreen = SDL_SetVideoMode(myScreenDim.w, myScreenDim.h, 16, mySDLFlags);
  if(myScreen == NULL)
  {
    cerr << "ERROR: Unable to open SDL window: " << SDL_GetError() << endl;
    return false;
  }
  myPitch = myScreen->pitch/2;
  myDirtyFlag = true;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::drawMediaSource()
{
  MediaSource& mediasrc = myOSystem->console().mediaSource();

  uInt8* currentFrame  = mediasrc.currentFrameBuffer();
  uInt8* previousFrame = mediasrc.previousFrameBuffer();
  uInt32 width         = mediasrc.width();
  uInt32 height        = mediasrc.height();
  uInt16* buffer       = (uInt16*) myScreen->pixels;

  uInt32 bufofsY    = 0;
  uInt32 screenofsY = 0;

  if(!myUsePhosphor)
  {
    for(uInt32 y = 0; y < height; ++y)
    {
      uInt32 pos = screenofsY;
      for(uInt32 x = 0; x < width; ++x)
      {
        const uInt32 bufofs = bufofsY + x;
        uInt8 v = currentFrame[bufofs];
        uInt8 w = previousFrame[bufofs];

        if(v != w || theRedrawTIAIndicator)
        {
          // If we ever get to this point, we know the current and previous
          // buffers differ.  In that case, make sure the changes are
          // are drawn in postFrameUpdate()
          myDirtyFlag = true;
          buffer[pos] = buffer[pos+1] = (uInt16) myDefPalette[v];
        }
        pos += 2;
      }
      bufofsY    += width;
      screenofsY += myPitch;
    }
  }
  else
  {
    // Phosphor mode always implies a dirty update,
    // so we don't care about theRedrawTIAIndicator
    myDirtyFlag = true;

    for(uInt32 y = 0; y < height; ++y)
    {
      uInt32 pos = screenofsY;
      for(uInt32 x = 0; x < width; ++x)
      {
        const uInt32 bufofs = bufofsY + x;
        uInt8 v = currentFrame[bufofs];
        uInt8 w = previousFrame[bufofs];

        buffer[pos++] = (uInt16) myAvgPalette[v][w];
        buffer[pos++] = (uInt16) myAvgPalette[v][w];
      }
      bufofsY    += width;
      screenofsY += myPitch;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::preFrameUpdate()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::postFrameUpdate()
{
  if(myDirtyFlag)
  {
    SDL_Flip(myScreen);
    myDirtyFlag = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::scanline(uInt32 row, uInt8* data)
{
  // Make sure no pixels are being modified
  SDL_LockSurface(myScreen);

  uInt32 bpp     = myScreen->format->BytesPerPixel;
  uInt8* start   = (uInt8*) myScreen->pixels;
  uInt32 yoffset = row * myScreen->pitch;
  uInt32 pixel = 0;
  uInt8 *p, r, g, b;

  for(Int32 x = 0; x < myScreen->w; x++)
  {
    p = (Uint8*) (start    +  // Start at top of RAM
                 (yoffset) +  // Go down 'row' lines
                 (x * bpp));  // Go in 'x' pixels

    pixel = *(Uint16*) p;
    SDL_GetRGB(pixel, myScreen->format, &r, &g, &b);

    data[x * 3 + 0] = r;
    data[x * 3 + 1] = g;
    data[x * 3 + 2] = b;
  }

  SDL_UnlockSurface(myScreen);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::toggleFilter()
{
  // Not supported
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::hLine(uInt32 x, uInt32 y, uInt32 x2, int color)
{
  SDL_Rect tmp;

  // Horizontal line
  tmp.x = x;
  tmp.y = y;
  tmp.w = (x2 - x + 1);
  tmp.h = 1;
  SDL_FillRect(myScreen, &tmp, myDefPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::vLine(uInt32 x, uInt32 y, uInt32 y2, int color)
{
  SDL_Rect tmp;

  // Vertical line
  tmp.x = x;
  tmp.y = y;
  tmp.w = 1;
  tmp.h = (y2 - y + 1);
  SDL_FillRect(myScreen, &tmp, myDefPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                               int color)
{
  SDL_Rect tmp;

  // Fill the rectangle
  tmp.x = x;
  tmp.y = y;
  tmp.w = w;
  tmp.h = h;
  SDL_FillRect(myScreen, &tmp, myDefPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::drawChar(const GUI::Font* font, uInt8 chr,
                               uInt32 xorig, uInt32 yorig, int color)
{
  const FontDesc& desc = font->desc();

  // If this character is not included in the font, use the default char.
  if(chr < desc.firstchar || chr >= desc.firstchar + desc.size)
  {
    if (chr == ' ')
      return;
    chr = desc.defaultchar;
  }

  const Int32 w = font->getCharWidth(chr);
  const Int32 h = font->getFontHeight();
  chr -= desc.firstchar;
  const uInt16* tmp = desc.bits + (desc.offset ? desc.offset[chr] : (chr * h));

  uInt16* buffer = (uInt16*) myScreen->pixels + yorig * myScreen->w + xorig;
  for(int y = 0; y < h; ++y)
  {
    const uInt16 ptr = *tmp++;
    uInt16 mask = 0x8000;
    for(int x = 0; x < w; ++x, mask >>= 1)
      if(ptr & mask)
        buffer[x] = (uInt16) myDefPalette[color];

    buffer += myScreen->w;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::drawBitmap(uInt32* bitmap, Int32 xorig, Int32 yorig,
                                 int color, Int32 h)
{
  uInt16* buffer = (uInt16*) myScreen->pixels + yorig * myScreen->w + xorig;
  for(int y = 0; y < h; ++y)
  {
    uInt32 mask = 0xF0000000;
    for(int x = 0; x < 8; ++x, mask >>= 4)
      if(bitmap[y] & mask)
        buffer[x] = (uInt16) myDefPalette[color];

    buffer += myScreen->w;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::translateCoords(Int32* x, Int32* y)
{
  // Coordinates don't change
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h)
{
  // We're using a hardware buffer; just indicate that the buffer is dirty
  myDirtyFlag = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::enablePhosphor(bool enable, int blend)
{
  myUsePhosphor   = enable;
  myPhosphorBlend = blend;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::showCursor(bool show)
{
  // Never show the cursor
  SDL_ShowCursor(SDL_DISABLE);
}
