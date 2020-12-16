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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef SQLITE_LIB_CXX
#define SQLITE_LIB_CXX

// Linux and libretro doesn't build unless this is enabled
#define SQLITE_OMIT_LOAD_EXTENSION 1

/*
 * We can't control the quality of code from outside projects, so for now
 * just disable warnings for it.
 */
#ifdef __clang__
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Weverything"
  #include "source/sqlite3.c"
  #pragma clang diagnostic pop
#else
  #include "source/sqlite3.c"
#endif

#endif  // SQLITE_LIB_CXX
