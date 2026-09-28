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

#include "System.hxx"
#include "CartUA.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeUA::CartridgeUA(ByteSpan image, string_view md5,
                         const Settings& settings, bool swapHotspots)
  : CartridgeEnhanced(image, md5, settings, 8_KB),
    mySwappedHotspots{swapHotspots}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeUA::install(System& system)
{
  CartridgeEnhanced::install(system);

  // Get the page accessing methods for the hot spots since they overlap
  // areas within the TIA we'll need to forward requests to the TIA
  myHotSpotPageAccess[0] = mySystem->getPageAccess(0x0220);
  myHotSpotPageAccess[1] = mySystem->getPageAccess(0x0220U | 0x80U);

  // Set the page accessing methods for the hot spots
  const System::PageAccess access(this, System::PageAccessType::READ);
  // Map all potential addresses
  // - A11, A10 and A8 are not connected to RIOT
  // - A9 is the fixed part of the hotspot address
  // - A7 is used by Brazilian carts
  // - A6 and A5 determine bank
  for(uInt16 a11 = 0; a11 <= 1; ++a11)
    for(uInt16 a10 = 0; a10 <= 1; ++a10)
      for(uInt16 a8 = 0; a8 <= 1; ++a8)
        for(uInt16 a7 = 0; a7 <= 1; ++a7)
        {
          const uInt16 addr = (a11 << 11U) + (a10 << 10U) + (a8 << 8U) + (a7 << 7U);

          mySystem->setPageAccess(0x0220U | addr, access);
          mySystem->setPageAccess(0x0240U | addr, access);
        }
  // Install pages for the startup bank
  bank(startBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeUA::checkSwitchBank(uInt16 address, uInt8)
{
  // Switch banks if necessary
  switch(address & 0x1260U)
  {
    case 0x0220:
      // Set the current bank to the lower 4k bank
      bank(mySwappedHotspots ? 1 : 0);
      return true;

    case 0x0240:
      // Set the current bank to the upper 4k bank
      bank(mySwappedHotspots ? 0 : 1);
      return true;

    default:
      break;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeUA::peek(uInt16 address)
{
  address &= myBankMask;

  checkSwitchBank(address, 0);

  // Because of the way accessing is set up, we will only get here
  // when doing a TIA read
  const int hotspot = ((address & 0x80U) >> 7U);
  return myHotSpotPageAccess[hotspot].device->peek(address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeUA::poke(uInt16 address, uInt8 value)
{
  address &= myBankMask;

  checkSwitchBank(address, 0);

  // Because of the way accessing is set up, we will may get here by
  // doing a write to TIA or cart; we ignore the cart write
  if (!(address & 0x1000U))
  {
    const int hotspot = ((address & 0x80U) >> 7U);
    myHotSpotPageAccess[hotspot].device->poke(address, value);
  }

  return false;
}
