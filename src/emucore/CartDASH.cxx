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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <cassert>
#include <cstring>

#include "System.hxx"
#include "TIA.hxx"
#include "CartDASH.hxx"

//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDASH::CartridgeDASH(const uInt8* image, uInt32 size, const Settings& settings) :
    Cartridge(settings), mySize(size) {

  // Allocate array for the ROM image
  myImage = new uInt8[mySize];

  // Copy the ROM image into my buffer
  memcpy(myImage, image, mySize);
  createCodeAccessBase(mySize + 65536); //RAM_TOTAL_SIZE);    // TODO: how does the RAM write offset affect the size we need?

  // This cart can address 4 banks of RAM, each 512 bytes @ 1000, 1200, 1400, 1600
  // However, it may not be addressable all the time (it may be swapped out)

#if 0
  registerRamArea(0x1000, RAM_BANK_SIZE, 0x00, RAM_WRITE_OFFSET); // 512 bytes RAM @ 0x1000
  registerRamArea(0x1200, RAM_BANK_SIZE, 0x00, RAM_WRITE_OFFSET); // 512 bytes RAM @ 0x1200
  registerRamArea(0x1400, RAM_BANK_SIZE, 0x00, RAM_WRITE_OFFSET); // 512 bytes RAM @ 0x1400
  registerRamArea(0x1600, RAM_BANK_SIZE, 0x00, RAM_WRITE_OFFSET); // 512 bytes RAM @ 0x1600
#endif

  // Remember startup bank (0 per spec, rather than last per 3E scheme).
  // Set this to go to 3rd 1K Bank.
  myStartBank = 0; //(3 << BANK_BITS) | 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDASH::~CartridgeDASH() {
  delete[] myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDASH::reset() {

  // Initialize RAM
  if (mySettings.getBool("ramrandom"))
    for (uInt32 i = 0; i < RAM_TOTAL_SIZE; ++i)
      myRAM[i] = mySystem->randGenerator().next();
  else
    memset(myRAM, 0, RAM_TOTAL_SIZE);

  // We'll map the startup bank (0) from the image into the third 1K bank upon reset

  bankROM(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDASH::install(System& system) {

  mySystem = &system;

  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  // Make sure the system we're being installed in has a page size that'll work
  assert((0x1800 & mask) == 0);

  System::PageAccess access(0, 0, 0, this, System::PA_READWRITE);

  // Set the page accessing methods for the hot spots (for 100% emulation
  // we need to chain any accesses below 0x40 to the TIA. Our poke() method
  // does this via mySystem->tiaPoke(...), at least until we come up with a
  // cleaner way to do it).
  for (uInt32 i = 0x00; i < 0x40; i += (1 << shift))
    mySystem->setPageAccess(i >> shift, access);

  // Setup the last segment (of 4, each 1K) to point to the first ROM slice
  // Actually we DO NOT want "always". It's just on bootup, and can be out switched later

  access.type = System::PA_READ;
  for (uInt32 address = (0x2000 - ROM_BANK_SIZE); address < 0x2000; address += (1 << shift)) {
    access.directPeekBase = &myImage[address & (ROM_BANK_SIZE - 1)];           // from base address 0x0000 in image
    access.codeAccessBase = &myCodeAccessBase[address & (ROM_BANK_SIZE - 1)];
    mySystem->setPageAccess(address >> shift, access);
  }

  // Initialise bank values for the 4x 1K bank areas
  // This is used to reverse-lookup from address to bank location
  for (uInt32 b = 0; b < 8; b++)
    bankInUse[b] = BANK_UNDEFINED;        // bank is undefined and inaccessible!

  // Install pages for the startup bank into the first segment
  bankROM(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeDASH::peek(uInt16 address) {

  uInt8 value = 0;
  uInt32 bank = (address >> (ROM_BANK_TO_POWER - 1)) & 7;   	// convert to 512 byte bank index (0-7)
  Int16 imageBank = bankInUse[bank];        			          // the ROM/RAM bank that's here

  if (imageBank == BANK_UNDEFINED) {						// an uninitialised bank?

    // accessing invalid bank, so return should be... random?
    value = mySystem->randGenerator().next();

  } else if (imageBank & BITMASK_ROMRAM) { // a RAM bank

    Int32 ramBank = imageBank & BIT_BANK_MASK;      // discard irrelevant bits
    Int32 offset = ramBank << RAM_BANK_TO_POWER;    // base bank address in RAM
    offset += (address & BITMASK_RAM_BANK);      // + byte offset in RAM bank
    value = myRAM[offset];

  } else {	// accessing ROM

    Int32 romBank = imageBank & BIT_BANK_MASK;      // discard irrelevant bits
    Int32 offset = romBank << ROM_BANK_TO_POWER;  // base bank address in image
    offset += (address & BITMASK_ROM_BANK);      // + byte offset in image bank
    value = myImage[offset];
  }

  return value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDASH::poke(uInt16 address, uInt8 value) {

  bool myBankChanged = false;

  address &= 0x0FFF;    // restrict to 4K address range

  // Check for write to the bank switch address. RAM/ROM and bank # are encoded in 'value'
  // There are NO mirrored hotspots.

  if (address == BANK_SWITCH_HOTSPOT_RAM)
    myBankChanged = bankRAM(value);

  else if (address == BANK_SWITCH_HOTSPOT_ROM)
    myBankChanged = bankROM(value);

  // Pass the poke through to the TIA. In a real Atari, both the cart and the
  // TIA see the address lines, and both react accordingly. In Stella, each
  // 64-byte chunk of address space is "owned" by only one device. If we
  // don't chain the poke to the TIA, then the TIA can't see it...
  mySystem->tia().poke(address, value);

  return myBankChanged;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDASH::bankRAM(uInt8 bank) {

  bool changed = false;

  uInt16 shift = mySystem->pageShift();

  uInt16 bankNumber = ((bank >> BANK_BITS) & 3) << 1;  // which bank # we are switching TO (BITS D6,D7) to 512byte block
  uInt16 currentBank = bank & BIT_BANK_MASK;          // Wrap around/restrict to valid range

  // Each RAM bank uses two slots, separated by 0x800 in memory -- one read, one write.
  bankInUse[bankNumber] = (Int16) (BITMASK_ROMRAM | currentBank);   // Record which bank switched in (marked as RAM)
  bankInUse[bankNumber + 4] = (Int16) (BITMASK_ROMRAM | currentBank); // Record which (write) bank switched in (marked as RAM)

  // Setup the page access methods for the current bank
  System::PageAccess access(0, 0, 0, this, System::PA_READ);

  uInt32 startCurrentBank = currentBank << RAM_BANK_TO_POWER;       // Effectively * 512 bytes
  uInt32 base = 0x1000 + startCurrentBank;

  for (uInt32 address = base; address < base + RAM_BANK_SIZE; address += (1 << shift)) {
    access.directPeekBase = &myRAM[startCurrentBank + (address & (RAM_BANK_SIZE - 1))];
    access.codeAccessBase = &myCodeAccessBase[65536 + startCurrentBank + (address & (RAM_BANK_SIZE - 1))];
    mySystem->setPageAccess(address >> shift, access);
  }

  access.directPeekBase = 0;
  access.type = System::PA_WRITE;

  base += RAM_WRITE_OFFSET;

  for (uInt32 address = base; address < base + RAM_BANK_SIZE; address += (1 << shift)) {
    access.directPeekBase = &myRAM[startCurrentBank + (address & (RAM_BANK_SIZE - 1))];
    access.codeAccessBase = &myCodeAccessBase[65536 + startCurrentBank + (address & (RAM_BANK_SIZE - 1))];
    mySystem->setPageAccess(address >> shift, access);
  }

  return changed; // TODO: does RAM change banks or not????
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDASH::bankROM(uInt8 bank) {

  bool changed = false;

  if (!bankLocked()) {  // debugger can lock ROM

    uInt16 shift = mySystem->pageShift();

    uInt16 bankNumber = (bank >> BANK_BITS) & 3;    // which bank # we are switching TO (BITS D6,D7)
    uInt16 currentBank = bank & BIT_BANK_MASK;      // Wrap around/restrict to valid range

    // Map ROM bank image into the system into the correct slot
    // Memory map is 1K slots at 0x1000, 0x1400, 0x1800, 0x1C00
    // Each ROM uses 2 consecutive 512 byte slots

    bankInUse[bankNumber * 2] = bankInUse[bankNumber * 2 + 1] = (Int16) currentBank; // Record which bank switched in (as ROM)

    uInt32 startCurrentBank = currentBank << ROM_BANK_TO_POWER;     // Effectively *1K

    // Setup the page access methods for the current bank
    System::PageAccess access(0, 0, 0, this, System::PA_READ);

    uInt32 base = 0x1000 + (bankNumber << ROM_BANK_TO_POWER);
    for (uInt32 address = base; address < base + ROM_BANK_SIZE; address += (1 << shift)) {
      access.directPeekBase = &myImage[startCurrentBank + (address & (ROM_BANK_SIZE - 1))];
      access.codeAccessBase = &myCodeAccessBase[startCurrentBank + (address & (ROM_BANK_SIZE - 1))];
      mySystem->setPageAccess(address >> shift, access);
    }

    changed = true;
  }

  return changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDASH::bank(uInt16 bank) {

  // Doesn't support bankswitching in the normal sense
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeDASH::bank() const {

  // Doesn't support bankswitching in the normal sense
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeDASH::bankCount() const {

  // Doesn't support bankswitching in the normal sense
  return 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDASH::patch(uInt16 address, uInt8 value) {

  // Patch the cartridge ROM (for debugger)

  myBankChanged = true;

  uInt32 bankNumber = (address >> RAM_BANK_TO_POWER) & 7;   // now 512 byte bank # (ie: 0-7)
  Int16 whichBankIsThere = bankInUse[bankNumber];           // ROM or RAM bank reference

  if (whichBankIsThere == BANK_UNDEFINED) {

    // We're trying to access undefined memory (no bank here yet). Fail!
    myBankChanged = false;

  } else if (whichBankIsThere & BITMASK_ROMRAM) { // patching RAM (512 byte banks)

    uInt32 byteOffset = address & BITMASK_RAM_BANK;
    uInt32 baseAddress = ((whichBankIsThere & BIT_BANK_MASK) << RAM_BANK_TO_POWER) + byteOffset;
    myRAM[baseAddress] = value;     // write to RAM

    // TODO: Stephen -- should we set 'myBankChanged' true when there's a RAM write?

  } else {  // patching ROM (1K banks)

    uInt32 byteOffset = address & BITMASK_ROM_BANK;
    uInt32 baseAddress = (whichBankIsThere << ROM_BANK_TO_POWER) + byteOffset;
    myImage[baseAddress] = value;   // write to the image
  }

  return myBankChanged;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeDASH::getImage(int& size) const {
  size = mySize;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDASH::save(Serializer& out) const {
  try {
    out.putString(name());
    for (uInt32 b = 0; b < 8; b++)
      out.putShort(bankInUse[b]);
    out.putByteArray(myRAM, RAM_TOTAL_SIZE);
  } catch (...) {
    cerr << "ERROR: CartridgeDASH::save" << endl;
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDASH::load(Serializer& in) {
  try {
    if (in.getString() != name())
      return false;
    for (uInt32 b = 0; b < 8; b++) {
      bank(bankInUse[b] = in.getShort());     // read, and switch it in
    }
    in.getByteArray(myRAM, RAM_TOTAL_SIZE);
  } catch (...) {
    cerr << "ERROR: CartridgeDASH::load" << endl;
    return false;
  }
  return true;
}
