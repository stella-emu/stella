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

#ifndef TV_ADJUSTABLE_HXX
#define TV_ADJUSTABLE_HXX

#include "bspf.hxx"

// Shared tweakable-parameter descriptor used by NTSCSignal, PALSignal, and
// TVSignal's adjustable-cycling UI.  'value' points into the filter's static
// custom-setup struct; the float must be in [-1..1] so that the scale
// helpers below work without modification.
struct AdjustableTag {
  string_view type;
  float* value{nullptr};
};

// Scale helpers shared by NTSCSignal, PALSignal, and TVSignal.
// [-1..1] float  ←→  [0..100] integer used in GUI sliders.
template<typename T>
  requires std::is_arithmetic_v<T>
constexpr float scaleFrom100(T x) {
  return (static_cast<float>(x) / 50.F) - 1.F;
}

template<typename T>
  requires std::is_arithmetic_v<T>
constexpr uInt32 scaleTo100(T x) {
  return static_cast<uInt32>(50.0001F * (static_cast<float>(x) + 1.F));
}

#endif  // TV_ADJUSTABLE_HXX
