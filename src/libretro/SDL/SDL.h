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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef SDL_H
#define SDL_H

#include "bspf.hxx"

typedef struct SDL_version
{
  uInt8 major;        /**< major version */
  uInt8 minor;        /**< minor version */
  uInt8 patch;        /**< update version */
} SDL_version;

typedef enum
{
  SDL_SCANCODE_UNKNOWN = 0
} SDL_Scancode;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static inline void SDL_GetVersion(SDL_version* ver)
{
  ver->major = 2;
  ver->minor = 0;
  ver->patch = 9;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static inline void SDL_Quit(void) {}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static inline const char* SDL_GetScancodeName(SDL_Scancode scancode) { return ""; }

#endif
