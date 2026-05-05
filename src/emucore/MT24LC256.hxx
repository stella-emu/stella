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

#ifndef MT24LC256_HXX
#define MT24LC256_HXX

#include "MicroChip24LC.hxx"

/**
  Emulates a Microchip Technology Inc. 24LC256, a 32KB Serial Electrically
  Erasable PROM accessed using the I2C protocol.

  @author Stephen Anthony & J. Payson
*/
class MT24LC256 : public MicroChip24LC<32_KB, 64>
{
  using MicroChip24LC::MicroChip24LC;
};

#endif  // MT24LC256_HXX
