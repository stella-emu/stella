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
// $Id: FrameBufferSDL.cxx,v 1.3 2003-11-09 23:53:20 stephena Exp $
//============================================================================

#include <SDL.h>
#include <SDL_syswm.h>
#include <sstream>

#include "Console.hxx"
#include "FrameBuffer.hxx"
#include "FrameBufferSDL.hxx"
#include "MediaSrc.hxx"
#include "Settings.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferSDL::FrameBufferSDL()
   :  x11Available(false),
      theZoomLevel(1),
      theMaxZoomLevel(1),
      theGrabMouseIndicator(false),
      theHideCursorIndicator(false),
      isFullscreen(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferSDL::~FrameBufferSDL()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL::pause(bool status)
{
  myPauseStatus = status;

  // Shade the palette to 75% normal value in pause mode
  if(myPauseStatus)
    setupPalette(0.75);
  else
    setupPalette(1.0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL::toggleFullscreen()
{
  isFullscreen = !isFullscreen;

  // Update the settings
  ostringstream tmp;
  tmp << isFullscreen;
  myConsole->settings().set("fullscreen", tmp.str());

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
    grabMouse(theGrabMouseIndicator);
    showCursor(!theHideCursorIndicator);
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
    if(isFullscreen)
      return;

    if(theZoomLevel == theMaxZoomLevel)
      theZoomLevel = 1;
    else
      theZoomLevel++;
  }
  else if(mode == -1)   // decrease size
  {
    if(isFullscreen)
      return;

    if(theZoomLevel == 1)
      theZoomLevel = theMaxZoomLevel;
    else
      theZoomLevel--;
  }

  if(!createScreen())
    return;

  // Update the settings
  ostringstream tmp;
  tmp << theZoomLevel;
  myConsole->settings().set("zoom", tmp.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL::showCursor(bool show)
{
  if(isFullscreen)
    return;

  if(show)
    SDL_ShowCursor(SDL_ENABLE);
  else
    SDL_ShowCursor(SDL_DISABLE);

  // Update the settings
  ostringstream tmp;
  tmp << !show;
  myConsole->settings().set("hidecursor", tmp.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL::grabMouse(bool grab)
{
  if(isFullscreen)
    return;

  if(grab)
    SDL_WM_GrabInput(SDL_GRAB_ON);
  else
    SDL_WM_GrabInput(SDL_GRAB_OFF);

  // Update the settings
  ostringstream tmp;
  tmp << grab;
  myConsole->settings().set("grabmouse", tmp.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBufferSDL::maxWindowSizeForScreen()
{
  if(!x11Available)
    return 1;

#ifdef UNIX
  // Otherwise, lock the screen and get the width and height
  myWMInfo.info.x11.lock_func();
  Display* theX11Display = myWMInfo.info.x11.display;
  myWMInfo.info.x11.unlock_func();

  int screenWidth  = DisplayWidth(theX11Display, DefaultScreen(theX11Display));
  int screenHeight = DisplayHeight(theX11Display, DefaultScreen(theX11Display));

  uInt32 multiplier = screenWidth / myWidth;
  bool found = false;

  while(!found && (multiplier > 0))
  {
    // Figure out the desired size of the window
    int width  = myWidth  * multiplier;
    int height = myHeight * multiplier;

    if((width < screenWidth) && (height < screenHeight))
      found = true;
    else
      multiplier--;
  }

  if(found)
    return multiplier;
  else
    return 1;
#else
  return 1;
#endif
}
