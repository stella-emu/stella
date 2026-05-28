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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef DR_WAV_LIB_HXX
#define DR_WAV_LIB_HXX

// We can't control the quality of code from outside projects, so for now
// just disable warnings for it.
#ifdef __clang__
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Weverything"
  #include "dr_wav.h"
  #pragma clang diagnostic pop
#elif defined(__GNUC__) || defined(__GNUG__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wall"
  #pragma GCC diagnostic ignored "-Wunused-function"
  #pragma GCC diagnostic ignored "-Wsign-conversion"
  #include "dr_wav.h"
  #pragma GCC diagnostic pop
#elif defined(BSPF_WINDOWS)
  #pragma warning(push, 0)
  #include "dr_wav.h"
  #pragma warning(pop)
#else
  #include "dr_wav.h"
#endif

#endif  // DR_WAV_LIB_HXX
