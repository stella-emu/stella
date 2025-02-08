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

#include "System.hxx"
#include "Cart3E.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3E::Cartridge3E(const ByteBuffer& image, size_t size,
                         string_view md5, const Settings& settings,
                         size_t bsSize)
  : CartridgeEnhanced(image, size, md5, settings,
                      bsSize == 0 ? BSPF::nextMultipleOf(size, 2_KB) : bsSize)
{
  myBankShift = BANK_SHIFT;
  myRamSize = RAM_SIZE;
  myRamBankCount = RAM_BANKS;
  myRamWpHigh = RAM_HIGH_WP;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3E::install(System& system)
{
  CartridgeEnhanced::install(system);

  const System::PageAccess access(this, System::PageAccessType::READWRITE);

  // The hotspots ($3E and $3F) are in TIA address space (plus mirrors), so we claim them here
  for(uInt16 addr = 0x0000; addr < 0x0800; addr += 0x100)
    mySystem->setPageAccess(addr, access);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3E::checkSwitchBank(uInt16 address, uInt8 value)
{
  // Tigervision bankswitching checks only A12, A11, A7, A6 and A0
  address &= 0b1100011000001; // 0x18c1;

  // Switch banks if necessary
  if(address == 0x01) // $3F
  {
    // Switch ROM bank into segment <value>
    bank(value);
    return true;
  }
  if(address == 0x00) // $3E
  {
    // Switch RAM bank into segment <value>
    bank(value + romBankCount());
    return true;
  }
  return false;
}