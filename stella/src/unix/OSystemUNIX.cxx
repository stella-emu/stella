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
// $Id: OSystemUNIX.cxx,v 1.24 2006-12-28 18:31:26 stephena Exp $
//============================================================================

#include <SDL.h>
#include <SDL_syswm.h>

#include <cstdlib>
#include <sstream>
#include <fstream>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "bspf.hxx"
#include "OSystem.hxx"
#include "OSystemUNIX.hxx"

#ifdef HAVE_GETTIMEOFDAY
  #include <time.h>
  #include <sys/time.h>
#endif

/**
  Each derived class is responsible for calling the following methods
  in its constructor:

  setBaseDir()
  setStateDir()
  setPropertiesDir()
  setConfigFile()
  setCacheFile()

  See OSystem.hxx for a further explanation
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemUNIX::OSystemUNIX(const string& path) : OSystem()
{
  const string& basedir = (path.length() > 0) ? path :
                           string(getenv("HOME")) + "/.stella";
  setBaseDir(basedir);

  setStateDir(basedir + "/state");

  setPropertiesDir(basedir);
  setConfigFile(basedir + "/stellarc");

  setCacheFile(basedir + "/stella.cache");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemUNIX::~OSystemUNIX()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 OSystemUNIX::getTicks()
{
#ifdef HAVE_GETTIMEOFDAY
  timeval now;
  gettimeofday(&now, 0);

  return (uInt32) (now.tv_sec * 1000000 + now.tv_usec);
#else
  return (uInt32) SDL_GetTicks() * 1000;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemUNIX::getScreenDimensions(int& width, int& height)
{
  // We might need to temporarily enable VIDEO support to check
  // screen dimensions
  bool isAlreadyInitialized = (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) > 0;
  if(!isAlreadyInitialized)
    SDL_Init(SDL_INIT_VIDEO);

  SDL_SysWMinfo myWMInfo;
  SDL_VERSION(&myWMInfo.version);
  if(SDL_GetWMInfo(&myWMInfo) > 0 && myWMInfo.subsystem == SDL_SYSWM_X11)
  {
    myWMInfo.info.x11.lock_func();
    width  = DisplayWidth(myWMInfo.info.x11.display,
               DefaultScreen(myWMInfo.info.x11.display));
    height = DisplayHeight(myWMInfo.info.x11.display,
               DefaultScreen(myWMInfo.info.x11.display));
    myWMInfo.info.x11.unlock_func();
  }

  if(!isAlreadyInitialized)
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}
