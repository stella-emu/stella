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

#ifndef TIA_CONSTANTS_HXX
#define TIA_CONSTANTS_HXX

#include "bspf.hxx"

namespace TIAConstants {

  constexpr uInt32 frameBufferHeight = 320;
  constexpr uInt32 minYStart = 1, maxYStart = 64;
  constexpr uInt32 minViewableHeight = 210, maxViewableHeight = 256;
  constexpr uInt32 initialGarbageFrames = 10;

}

#endif // TIA_CONSTANTS_HXX
