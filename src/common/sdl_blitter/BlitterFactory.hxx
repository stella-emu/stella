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

#ifndef BLITTER_FACTORY_HXX
#define BLITTER_FACTORY_HXX

#include <string>

#include "Blitter.hxx"
#include "FBBackendSDL.hxx"
#include "bspf.hxx"

class BlitterFactory {
  public:

    enum class ScalingAlgorithm: uInt8 {
      nearestNeighbour,
      bilinear,
      quasiInteger
    };

  public:

    static unique_ptr<Blitter> createBlitter(FBBackendSDL& fb, ScalingAlgorithm scaling);
};

#endif // BLITTER_FACTORY_HXX
