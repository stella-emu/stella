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
// Copyright (c) 1995-2010 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#if defined(HAVE_X11)
  #include <SDL_syswm.h>
  #include <X11/Xutil.h>
#endif

#include "bspf.hxx"
#include "OSystem.hxx"
#include "OSystemUNIX.hxx"

/**
  Each derived class is responsible for calling the following methods
  in its constructor:

  setBaseDir()
  setConfigFile()

  See OSystem.hxx for a further explanation
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemUNIX::OSystemUNIX()
  : OSystem()
{
  setBaseDir("~/.stella");
  setConfigFile("~/.stella/stellarc");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemUNIX::~OSystemUNIX()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemUNIX::setAppWindowPos(int x, int y, int, int)
{
#if defined(HAVE_X11)
  SDL_SysWMinfo sdl_info;
  memset(&sdl_info, 0, sizeof(sdl_info));

  SDL_VERSION (&sdl_info.version);
  if(SDL_GetWMInfo(&sdl_info) > 0 && sdl_info.subsystem == SDL_SYSWM_X11)
  {
    XSizeHints* hints = XAllocSizeHints();
    if(hints)
    {
      Display* display = sdl_info.info.x11.display;
      Window window = sdl_info.info.x11.wmwindow;
      hints->flags |= USPosition;
      hints->x = x;
      hints->y = y;
      XMoveWindow(display, window, hints->x, hints->y);

      /* Flush the resize event so we don't catch it later */
      XSync(display, True);
      XSetWMNormalHints(display, window, hints);
      XFree(hints);
    }
  }
#endif
}
