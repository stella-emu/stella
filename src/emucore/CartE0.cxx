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

#include "System.hxx"
#include "CartE0.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeE0::CartridgeE0(const ByteBuffer& image, size_t size,
                         const string& md5, const Settings& settings)
  : CartridgeEnhanced(image, size, md5, settings)
{
  myBankShift = BANK_SHIFT;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE0::reset()
{
  // Setup segments to some default slices
  if(randomStartBank())
  {
    bank(mySystem->randGenerator().next() % 8, 0);
    bank(mySystem->randGenerator().next() % 8, 1);
    bank(mySystem->randGenerator().next() % 8, 2);
  }
  else
  {
    bank(4, 0);
    bank(5, 1);
    bank(6, 2);
  }
  myCurrentSegOffset[3] = (bankCount() - 1) << myBankShift; // fixed

  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE0::install(System& system)
{
  //// Allocate array for the current bank segments slices
  //myCurrentSegOffset = make_unique<uInt16[]>(myBankSegs);

  //// Setup page access
  //mySystem = &system;

  CartridgeEnhanced::install(system);

  System::PageAccess access(this, System::PageAccessType::READ);

  // Set the page acessing methods for the first part of the last segment
  for(uInt16 addr = 0x1C00; addr < static_cast<uInt16>(0x1FE0U & ~System::PAGE_MASK);
      addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[0x1C00 + (addr & 0x03FF)];
    access.romAccessBase = &myRomAccessBase[0x1C00 + (addr & 0x03FF)];
    access.romPeekCounter = &myRomAccessCounter[0x1C00 + (addr & 0x03FF)];
    access.romPokeCounter = &myRomAccessCounter[0x1C00 + (addr & 0x03FF) + myAccessSize];
    mySystem->setPageAccess(addr, access);
  }

  // Set the page accessing methods for the hot spots in the last segment
  access.directPeekBase = nullptr;
  access.romAccessBase = &myRomAccessBase[0x1FC0];
  access.romPeekCounter = &myRomAccessCounter[0x1FC0];
  access.romPokeCounter = &myRomAccessCounter[0x1FC0 + myAccessSize];
  access.type = System::PageAccessType::READ;
  for(uInt16 addr = (0x1FE0 & ~System::PAGE_MASK); addr < 0x2000;
      addr += System::PAGE_SIZE)
    mySystem->setPageAccess(addr, access);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeE0::checkSwitchBank(uInt16 address, uInt8)
{
  // Switch banks if necessary
  if((address >= 0x0FE0) && (address <= 0x0FE7))
  {
    bank(address & 0x0007, 0);
    return true;
  }
  else if((address >= 0x0FE8) && (address <= 0x0FEF))
  {
    bank(address & 0x0007, 1);
    return true;
  }
  else if((address >= 0x0FF0) && (address <= 0x0FF7))
  {
    bank(address & 0x0007, 2);
    return true;
  }
  return false;
}
