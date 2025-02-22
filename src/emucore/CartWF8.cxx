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
// Copyright (c) 1995-2025 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "CartWF8.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeWF8::CartridgeWF8(const ByteBuffer& image, size_t size,
                           string_view md5, const Settings& settings,
                           size_t bsSize)
  : CartridgeEnhanced(image, size, md5, settings, bsSize)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeWF8::checkSwitchBank(uInt16 address, uInt8 value)
{
  // Switch banks if necessary
  // Note: addresses could be calculated from hotspot and bank count
  if (address == 0x1FF8)
  {
    bank((value & 0b100) >> 2);
    return true;
  }
  return false;
}
