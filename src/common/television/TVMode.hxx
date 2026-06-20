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

#ifndef TV_MODE_HXX
#define TV_MODE_HXX

#include "bspf.hxx"

// The type of TV video connection being emulated, ordered by how many
// separate channels the signal carries and therefore how few artifacts it
// shows: None is the raw TIA palette (no signal model at all, for external
// capture); RGB (3 channels) is the cleanest engine output; S-Video (2)
// band-limits the colour; Composite (1) combines everything and shows all
// artifacts.  Custom is a user-tweaked composite.
enum class TVMode: uInt8
{
  None      = 0,
  RGB       = 1,
  SVideo    = 2,
  Composite = 3,
  Custom    = 4
};

#endif  // TV_MODE_HXX
