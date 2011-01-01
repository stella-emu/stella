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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <SDL.h>

#include "Console.hxx"
#include "MediaSrc.hxx"
#include "OSystem.hxx"
#include "Font.hxx"
#include "Surface.hxx"
#include "Settings.hxx"
#include "FrameBufferGP2X.hxx"


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferGP2X::FrameBufferGP2X(OSystem* osystem)
  : FrameBuffer(osystem),
    myBasePtr(0),
    myDirtyFlag(true)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferGP2X::~FrameBufferGP2X()
{
  myTvHeight = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGP2X::initSubsystem(VideoMode mode)
{
  // Create the screen
  return setVidMode(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FrameBufferGP2X::about() const
{
  // TODO - add SDL info to this string
  return "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGP2X::setVidMode(VideoMode mode)
{
  const SDL_VideoInfo* info = NULL;

  // Make sure to clear the screen, since we're using different resolutions,
  // and there tends to be lingering artifacts in hardware mode
  if(myScreen)
  {
    SDL_FillRect(myScreen, NULL, 0);
    SDL_UpdateRect(myScreen, 0, 0, 0, 0);
  }

  myScreenDim.x = myScreenDim.y = 0;
  myScreenDim.w = mode.screen_w;
  myScreenDim.h = mode.screen_h;

  myImageDim.x = mode.image_x;
  myImageDim.y = mode.image_y;
  myImageDim.w = mode.image_w;
  myImageDim.h = mode.image_h;

  // If we got a screenmode that won't be scaled, center it vertically
  // Otherwise, SDL hardware scaling kicks in, and we won't mess with it
  if(myBaseDim.h <= 240)
  {
    // If we can center vertically, do so
    // It means the screen we open must be larger than the TIA buffer,
    // since we're going to start drawing at an y offset
    myScreenDim.y = (240 - myBaseDim.h) / 2;
    myScreenDim.h = myScreenDim.y + myBaseDim.h;
  }

  // check to see if we're displaying on a TV
  if (!myTvHeight) {
    info = SDL_GetVideoInfo();

    if (info && info->current_w == 720
             && (info->current_h == 480 || info->current_h == 576)) {
      myTvHeight = info->current_h;

    } else {
      myTvHeight = -1;
    }
  }

  // if I am displaying on a TV then I want to handle overscan
  // I do this as per the following:
  // http://www.gp32x.com/board/index.php?showtopic=23819&st=375&p=464689&#entry464689
  // Basically, I set the screen bigger than the base image.  Thus, the image
  // should be (mostly) on screen.  The SDL HW scaler will make sure the
  // screen fills the TV.
  if (myTvHeight > 0) {
    myScreenDim.w =  (int)(((float)myScreenDim.w)
                  * myOSystem->settings().getFloat("tv_scale_width"));
    myScreenDim.h =  (int)(((float)myScreenDim.h)
                  * myOSystem->settings().getFloat("tv_scale_height"));

    if ((myScreenDim.w % 2))
      myScreenDim.w++;

    if ((myScreenDim.h % 2))
      myScreenDim.h++;

    myScreenDim.x = (myScreenDim.w - mode.screen_w)/2;
    myScreenDim.y = (myScreenDim.h - mode.screen_h)/2;
  }

  // The GP2X always uses a 16-bit hardware buffer
  myScreen = SDL_SetVideoMode(myScreenDim.w, myScreenDim.h, 16, mySDLFlags);
  if(myScreen == NULL)
  {
    cerr << "ERROR: Unable to open SDL window: " << SDL_GetError() << endl;
    return false;
  }
  myPitch = myScreen->pitch/2;
  myBasePtr = (uInt16*) myScreen->pixels + myScreenDim.y * myPitch;
  myDirtyFlag = true;
  myFormat = myScreen->format;

  // Make sure drawMediaSource() knows which renderer to use
  stateChanged(myOSystem->eventHandler().state());

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
  uInt16* buffer       = myBasePtr;

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
void FrameBufferGP2X::scanline(uInt32 row, uInt8* data) const
{
  // Make sure no pixels are being modified
  SDL_LockSurface(myScreen);

  uInt32 bpp     = myScreen->format->BytesPerPixel;
  uInt8* start   = (uInt8*) myBasePtr;
  uInt32 yoffset = row * myScreen->pitch;
  uInt32 pixel = 0;
  uInt8 *p, r, g, b;

  for(Int32 x = 0; x < myBaseDim.w; ++x)
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
  tmp.y = y + myScreenDim.y;
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
  tmp.y = y + myScreenDim.y;
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
  tmp.y = y + myScreenDim.y;
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

  uInt16* buffer = (uInt16*) myBasePtr + yorig * myPitch + xorig;
  for(int y = 0; y < h; ++y)
  {
    const uInt16 ptr = *tmp++;
    uInt16 mask = 0x8000;
    for(int x = 0; x < w; ++x, mask >>= 1)
      if(ptr & mask)
        buffer[x] = (uInt16) myDefPalette[color];

    buffer += myPitch;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::drawBitmap(uInt32* bitmap, Int32 xorig, Int32 yorig,
                                 int color, Int32 h)
{
  uInt16* buffer = (uInt16*) myBasePtr + yorig * myPitch + xorig;
  for(int y = 0; y < h; ++y)
  {
    uInt32 mask = 0xF0000000;
    for(int x = 0; x < 8; ++x, mask >>= 4)
      if(bitmap[y] & mask)
        buffer[x] = (uInt16) myDefPalette[color];

    buffer += myPitch;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::drawSurface(const GUI::Surface* surface, Int32 x, Int32 y)
{
/* TODO - not supported yet
  SDL_Rect clip;
  clip.x = x;
  clip.y = y;

  SDL_BlitSurface(surface->myData, 0, myScreen, &clip);
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::bytesToSurface(GUI::Surface* surface, int row,
                                     uInt8* data, int rowbytes) const
{
/* TODO - not supported yet
  SDL_Surface* s = surface->myData;

  uInt16* pixels = (uInt16*) s->pixels;
  int surfbytes = s->pitch/2;
  pixels += (row * surfbytes);

  // Calculate a scanline of zoomed surface data
  for(int c = 0; c < rowbytes; c += 3)
  {
    uInt32 pixel = SDL_MapRGB(s->format, data[c], data[c+1], data[c+2]);
    *pixels++ = pixel;
  }
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GUI::Surface* FrameBufferGP2X::createSurface(int width, int height) const
{
  SDL_Surface* data =
    SDL_CreateRGBSurface(SDL_SWSURFACE, width, height,
                         16, myFormat->Rmask, myFormat->Gmask,
                         myFormat->Bmask, myFormat->Amask);

  return data ? new GUI::Surface(width, height, data) : NULL;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::translateCoords(Int32& x, Int32& y) const
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

  theRedrawTIAIndicator = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGP2X::showCursor(bool show)
{
  // Never show the cursor
  SDL_ShowCursor(SDL_DISABLE);
}
