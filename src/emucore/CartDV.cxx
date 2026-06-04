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
#include "CartDV.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDV::CartridgeDV(ByteSpan image, string_view md5,
                         const Settings& settings)
  : CartridgeEnhanced(image, md5, settings, 8_KB)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDV::install(System& system)
{
  CartridgeEnhanced::install(system);

  // Save the page access for the hotspot since it overlaps
  // an area within RIOT we'll need to forward requests to RIOT
  myHotSpotPageAccess[0] = mySystem->getPageAccess(0x0FD0);
  myHotSpotPageAccess[1] = mySystem->getPageAccess(0x0FB0);

  // Set the page access method for the hotspot
  const System::PageAccess access(this, System::PageAccessType::READ);
  mySystem->setPageAccess(0x0FD0, access);
  mySystem->setPageAccess(0x0FB0, access);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDV::checkSwitchBank(uInt16 address, uInt8)
{
  // Switch banks if necessary
  switch(address)
  {
    case 0x0FD0:
      bank(0);
      return true;

    case 0x0FB0:
      bank(1);
      return true;

    default:
      break;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeDV::peek(uInt16 address)
{
  address &= myBankMask;

  checkSwitchBank(address, 0);

  if(!(address & 0x1000) && (address == 0x0FD0 || address == 0x0FB0))
  {
    const int hotspot = (address == 0xFD0 ? 0 : 1);
    return myHotSpotPageAccess[hotspot].device->peek(address);
  }

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDV::poke(uInt16 address, uInt8 value)
{
  address &= myBankMask;

  checkSwitchBank(address, 0);

  if(!(address & 0x1000) && (address == 0x0FD0 || address == 0x0FB0))
  {
    const int hotspot = (address == 0xFD0 ? 0 : 1);
    myHotSpotPageAccess[hotspot].device->poke(address, value);
  }

  return false;
}
