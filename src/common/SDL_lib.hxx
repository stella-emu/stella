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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef SDL_LIB_HXX
#define SDL_LIB_HXX

#include <SDL.h>

/*
  Seems to be needed for ppc64le, doesn't hurt other archs
  Note that this is a problem in SDL2, which includes <altivec.h>
    https://bugzilla.redhat.com/show_bug.cgi?id=1419452
*/
#undef vector
#undef pixel
#undef bool

#endif
