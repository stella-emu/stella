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
// Copyright (c) 1995-1999 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FrameBufferSDL.cxx,v 1.3 2005-01-04 20:06:20 stephena Exp $
//============================================================================

#include <SDL.h>
#include <SDL_syswm.h>
#include <sstream>

#include "Console.hxx"
#include "FrameBuffer.hxx"
#include "FrameBufferSDL.hxx"
#include "MediaSrc.hxx"
#include "Settings.hxx"

#include "stella.xpm"   // The Stella icon

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferSDL::FrameBufferSDL()
   :  myWMAvailable(false),
      theZoomLevel(1),
      theMaxZoomLevel(1),
      theAspectRatio(1.0),
      myPauseStatus(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferSDL::~FrameBufferSDL()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL::pauseEvent(bool status)
{
  myPauseStatus = status;
  setupPalette();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL::setupPalette()
{
  // Shade the palette to 75% normal value in pause mode
  float shade = 1.0;
  if(myPauseStatus)
    shade = 0.75;

  const uInt32* gamePalette = myMediaSource->palette();
  for(uInt32 i = 0; i < 256; ++i)
  {
    Uint8 r, g, b;

    r = (Uint8) (((gamePalette[i] & 0x00ff0000) >> 16) * shade);
    g = (Uint8) (((gamePalette[i] & 0x0000ff00) >> 8) * shade);
    b = (Uint8) ((gamePalette[i] & 0x000000ff) * shade);

    myPalette[i] = mapRGB(r, g, b);
  }

  theRedrawEntireFrameIndicator = true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL::toggleFullscreen()
{
  bool isFullscreen = !myConsole->settings().getBool("fullscreen");

  // Update the settings
  myConsole->settings().setBool("fullscreen", isFullscreen);

  if(isFullscreen)
    mySDLFlags |= SDL_FULLSCREEN;
  else
    mySDLFlags &= ~SDL_FULLSCREEN;

  if(!createScreen())
    return;

  if(isFullscreen)  // now in fullscreen mode
  {
    grabMouse(true);
    showCursor(false);
  }
  else    // now in windowed mode
  {
    grabMouse(myConsole->settings().getBool("grabmouse"));
    showCursor(!myConsole->settings().getBool("hidecursor"));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL::resize(int mode)
{
  // reset size to that given in properties
  // this is a special case of allowing a resize while in fullscreen mode
  if(mode == 0)
  {
    myWidth  = myMediaSource->width() << 1;
    myHeight = myMediaSource->height();
  }
  else if(mode == 1)   // increase size
  {
    if(myConsole->settings().getBool("fullscreen"))
      return;

    if(theZoomLevel == theMaxZoomLevel)
      theZoomLevel = 1;
    else
      theZoomLevel++;
  }
  else if(mode == -1)   // decrease size
  {
    if(myConsole->settings().getBool("fullscreen"))
      return;

    if(theZoomLevel == 1)
      theZoomLevel = theMaxZoomLevel;
    else
      theZoomLevel--;
  }

  if(!createScreen())
    return;

  // Update the settings
  myConsole->settings().setInt("zoom", theZoomLevel);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL::showCursor(bool show)
{
  if(show)
    SDL_ShowCursor(SDL_ENABLE);
  else
    SDL_ShowCursor(SDL_DISABLE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL::grabMouse(bool grab)
{
  if(grab)
    SDL_WM_GrabInput(SDL_GRAB_ON);
  else
    SDL_WM_GrabInput(SDL_GRAB_OFF);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferSDL::fullScreen()
{
  return myConsole->settings().getBool("fullscreen");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBufferSDL::maxWindowSizeForScreen()
{
  uInt32 sWidth     = screenWidth();
  uInt32 sHeight    = screenHeight();
  uInt32 multiplier = sWidth / myWidth;

  // If screenwidth or height could not be found, use default zoom value
  if(sWidth == 0 || sHeight == 0)
    return 4;

  bool found = false;
  while(!found && (multiplier > 0))
  {
    // Figure out the desired size of the window
    uInt32 width  = (uInt32) (myWidth * multiplier * theAspectRatio);
    uInt32 height = myHeight * multiplier;

    if((width < sWidth) && (height < sHeight))
      found = true;
    else
      multiplier--;
  }

  if(found)
    return multiplier;
  else
    return 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBufferSDL::screenWidth()
{
  uInt32 width = 0;

  if(myWMAvailable)
  {
#if defined(UNIX)
    if(myWMInfo.subsystem == SDL_SYSWM_X11)
    {
      myWMInfo.info.x11.lock_func();
      width = DisplayWidth(myWMInfo.info.x11.display,
                           DefaultScreen(myWMInfo.info.x11.display));
      myWMInfo.info.x11.unlock_func();
    }
#elif defined(WIN32)
    width = (uInt32) GetSystemMetrics(SM_CXSCREEN);
#elif defined(MAC_OSX)
  // FIXME - add OSX Desktop code here (I don't think SDL supports it yet)
#endif
  }

  return width;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBufferSDL::screenHeight()
{
  uInt32 height = 0;

  if(myWMAvailable)
  {
#if defined(UNIX)
    if(myWMInfo.subsystem == SDL_SYSWM_X11)
    {
      myWMInfo.info.x11.lock_func();
      height = DisplayHeight(myWMInfo.info.x11.display,
                             DefaultScreen(myWMInfo.info.x11.display));
      myWMInfo.info.x11.unlock_func();
    }
#elif defined(WIN32)
    height = (uInt32) GetSystemMetrics(SM_CYSCREEN);
#elif defined(MAC_OSX)
  // FIXME - add OSX Desktop code here (I don't think SDL supports it yet)
#endif
  }

  return height;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL::setWindowAttributes()
{
  // Set the window title
  ostringstream name;
  name << "Stella: \"" << myConsole->properties().get("Cartridge.Name") << "\"";
  SDL_WM_SetCaption(name.str().c_str(), "stella");

#ifndef MAC_OSX
  // Set the window icon
  uInt32 w, h, ncols, nbytes;
  uInt32 rgba[256], icon[32 * 32];
  uInt8  mask[32][4];

  sscanf(stella_icon[0], "%d %d %d %d", &w, &h, &ncols, &nbytes);
  if((w != 32) || (h != 32) || (ncols > 255) || (nbytes > 1))
  {
    cerr << "ERROR: Couldn't load the icon.\n";
    return;
  }

  for(uInt32 i = 0; i < ncols; i++)
  {
    unsigned char  code;
	char color[32];
    uInt32 col;

    sscanf(stella_icon[1 + i], "%c c %s", &code, color);
    if(!strcmp(color, "None"))
      col = 0x00000000;
    else if(!strcmp(color, "black"))
      col = 0xFF000000;
    else if (color[0] == '#')
    {
      sscanf(color + 1, "%06x", &col);
      col |= 0xFF000000;
    }
    else
    {
      cerr << "ERROR: Couldn't load the icon.\n";
      return;
    }
    rgba[code] = col;
  }

  memset(mask, 0, sizeof(mask));
  for(h = 0; h < 32; h++)
  {
    const char* line = stella_icon[1 + ncols + h];
    for(w = 0; w < 32; w++)
    {
      icon[w + 32 * h] = rgba[(int)line[w]];
      if(rgba[(int)line[w]] & 0xFF000000)
        mask[h][w >> 3] |= 1 << (7 - (w & 0x07));
    }
  }

  SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(icon, 32, 32, 32,
                         32 * 4, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF000000);
  SDL_WM_SetIcon(surface, (unsigned char *) mask);
  SDL_FreeSurface(surface);
#endif
}
