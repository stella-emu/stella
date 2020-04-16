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
#include "TIA.hxx"
#include "Cart3EPlus.hxx"

//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3EPlus::Cartridge3EPlus(const ByteBuffer& image, size_t size,
                                 const string& md5, const Settings& settings)
  : CartridgeEnhanced(image, size, md5, settings)

{
  myBankShift = BANK_SHIFT;
  myRamSize = RAM_SIZE;
  myRamBankCount = RAM_BANKS;
  myRamWpHigh = RAM_HIGH_WP;

  //// Allocate array for the ROM image
  //myImage = make_unique<uInt8[]>(mySize);

  //// Copy the ROM image into my buffer
  //std::copy_n(image.get(), mySize, myImage.get());
  //createRomAccessArrays(mySize + myRAM.size());
}

//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//void Cartridge3EPlus::reset()
//{
//  initializeRAM(myRAM.data(), myRAM.size());
//
//  // Remember startup bank (0 per spec, rather than last per 3E scheme).
//  // Set this to go to 3rd 1K Bank.
//  initializeStartBank(0);
//
//  // Initialise bank values for all ROM/RAM access
//  // This is used to reverse-lookup from address to bank location
//  for(auto& b: bankInUse)
//    b = BANK_UNDEFINED;     // bank is undefined and inaccessible!
//
//  initializeBankState();
//
//  // We'll map the startup banks 0 and 3 from the image into the third 1K bank upon reset
//  bankROM((0 << BANK_BITS) | 0);
//  bankROM((3 << BANK_BITS) | 0);
//}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3EPlus::reset()
{
  CartridgeEnhanced::reset();

  bank(mySystem->randGenerator().next() % romBankCount(), 1);
  bank(mySystem->randGenerator().next() % romBankCount(), 2);
  bank(startBank(), 3); // Stella reads the PC vector always from here (TODO?)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3EPlus::install(System& system)
{
  CartridgeEnhanced::install(system);

  System::PageAccess access(this, System::PageAccessType::WRITE);

  // The hotspots are in TIA address space, so we claim it here
  for(uInt16 addr = 0x00; addr < 0x40; addr += System::PAGE_SIZE)
    mySystem->setPageAccess(addr, access);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3EPlus::checkSwitchBank(uInt16 address, uInt8 value)
{
  // Switch banks if necessary
  if(address == 0x003F) {
    // Switch ROM bank into segment 0
    bank(value & 0b111111, value >> 6);
    return true;
  }
  else if(address == 0x003E)
  {
    // Switch RAM bank into segment 0
    bank((value & 0b111111) + romBankCount(), value >> 6);
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Cartridge3EPlus::peek(uInt16 address)
{
  uInt16 peekAddress = address;
  address &= ROM_MASK;

  if(address < 0x0040)  // TIA peek
    return mySystem->tia().peek(address);

  return CartridgeEnhanced::peek(peekAddress);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3EPlus::poke(uInt16 address, uInt8 value)
{
  if(CartridgeEnhanced::poke(address, value))
    return true;

  if(address < 0x0040)  // TIA poke
    // Handle TIA space that we claimed above
    return mySystem->tia().poke(address, value);

  return false;
}
