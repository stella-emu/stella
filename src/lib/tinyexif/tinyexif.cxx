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
// Copyright (c) 1995-2022 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

/*
 * We can't control the quality of code from outside projects, so for now
 * just disable warnings for it.
 */
#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Weverything"
  #include "source/TinyEXIF.cpp"  // NOLINT
  #pragma clang diagnostic pop
#elif defined(__GNUC__) || defined(__GNUG__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wall"
  #pragma GCC diagnostic ignored "-Wcast-function-type"
  #pragma GCC diagnostic ignored "-Wshift-negative-value"
  #pragma GCC diagnostic ignored "-Wunused-function"
  #include "source/TinyEXIF.cpp"  // NOLINT
  #pragma GCC diagnostic pop
#elif defined(BSPF_WINDOWS)
  #pragma warning(push, 0)
  #pragma warning(disable : 4505)
  #include "source/TinyEXIF.cpp"  // NOLINT
  #pragma warning(pop)
#else
  #include "source/TinyEXIF.cpp"  // NOLINT
#endif
