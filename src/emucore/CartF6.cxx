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

#include "CartF6.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeF6::CartridgeF6(const ByteBuffer& image, size_t size,
                         const string& md5, const Settings& settings)
  : CartridgeEnhanced(image, size, 16_KB, md5, settings)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF6::checkSwitchBank(uInt16 address, uInt8)
{
  // Switch banks if necessary
  // Note: addresses could be calculated from hotspot and bank count
  if((address >= 0x1FF6) && (address <= 0x1FF9))
  {
    bank(address - 0x1FF6);
    return true;
  }
  return false;
}

