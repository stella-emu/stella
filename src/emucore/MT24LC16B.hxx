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

#ifndef MT24LC16B_HXX
#define MT24LC16B_HXX

#include "MicroChip24LC.hxx"

/**
  Emulates a Microchip Technology Inc. 24LC16B, a 2KiB Serial Electrically
  Erasable PROM accessed using the I2C protocol. Thanks to J. Payson
  (aka Supercat) for the bulk of the 24LC256 code; altered for
  24LC16B / EFF (Grizzards) cart type by Bruce-Robert Pocock.

  @author Stephen Anthony, J. Payson, and Bruce-Robert Pocock
*/
class MT24LC16B : public MicroChip24LC<2_KB, 16>
{
  using MicroChip24LC::MicroChip24LC;
};

#endif
