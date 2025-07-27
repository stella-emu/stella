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
// Copyright (c) 1995-2025 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef SDL_LIB_HXX
#define SDL_LIB_HXX

#include "bspf.hxx"

/*
 * We can't control the quality of code from outside projects, so for now
 * just disable warnings for it.
 */
#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wdocumentation"
  #pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
  #pragma clang diagnostic ignored "-Wimplicit-fallthrough"
  #pragma clang diagnostic ignored "-Wreserved-id-macro"
  #pragma clang diagnostic ignored "-Wold-style-cast"
  #pragma clang diagnostic ignored "-Wreserved-identifier"
  #pragma clang diagnostic ignored "-Wswitch-default"
  #include <SDL3/SDL.h>
  #pragma clang diagnostic pop
#elif defined(BSPF_WINDOWS)
  #pragma warning(push, 0)
  #include <SDL3/SDL.h>
  #pragma warning(pop)
#else
  #include <SDL3/SDL.h>
#endif

static inline string SDLVersion()
{
  ostringstream buf;
  const int ver = SDL_GetVersion();
  buf << "SDL "
      << SDL_VERSIONNUM_MAJOR(ver) << "."
      << SDL_VERSIONNUM_MINOR(ver) << "."
      << SDL_VERSIONNUM_MICRO(ver);
  return buf.str();
}

static inline bool SDLOpenURL(const string& url)
{
  return SDL_OpenURL(url.c_str());
}

#endif  // SDL_LIB_HXX
