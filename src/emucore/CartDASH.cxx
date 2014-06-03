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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDASH::CartridgeDASH(const uInt8* image, uInt32 size, const Settings& settings)
  : Cartridge(settings),
    mySize(size)
{
  // Allocate array for the ROM image
  myImage = new uInt8[mySize];

  // Copy the ROM image into my buffer
  memcpy(myImage, image, mySize);
  createCodeAccessBase(mySize + RAM_TOTAL_SIZE);

  // This cart can address 4 banks of RAM, each 512 bytes @ 1000, 1200, 1400, 1600
  // However, it may not be addressable all the time (it may be swapped out)

  // TODO: Stephen -- is this correct for defining 4 separate RAM areas, or can it be done as one block?

  registerRamArea(0x1000, RAM_BANK_SIZE, 0x00, RAM_WRITE_OFFSET); // 512 bytes RAM @ 0x1000
  registerRamArea(0x1200, RAM_BANK_SIZE, 0x00, RAM_WRITE_OFFSET); // 512 bytes RAM @ 0x1200
  registerRamArea(0x1400, RAM_BANK_SIZE, 0x00, RAM_WRITE_OFFSET); // 512 bytes RAM @ 0x1400
  registerRamArea(0x1600, RAM_BANK_SIZE, 0x00, RAM_WRITE_OFFSET); // 512 bytes RAM @ 0x1600

  // Remember startup bank (0 per spec, rather than last per 3E scheme).
  // Set this to go to 3rd 1K Bank.
  myStartBank = (3 << BANK_BITS) | 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDASH::~CartridgeDASH()
{
  delete[] myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDASH::reset()
{
  // Initialize RAM
  if (mySettings.getBool("ramrandom"))
    for (uInt32 i = 0; i < RAM_TOTAL_SIZE; ++i)
      myRAM[i] = mySystem->randGenerator().next();
  else
    memset(myRAM, 0, RAM_TOTAL_SIZE);

  // We'll map the startup bank (0) from the image into the third 1K bank upon reset

  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDASH::install(System& system)
{
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
  for (uInt32 byte = 0; byte < ROM_BANK_SIZE; byte++)
  {
    uInt32 address = (0x1000 - ROM_BANK_SIZE) + (byte<<shift);  // which byte in last bank of 2600 address space
    access.directPeekBase = &myImage[byte];           // from base address 0x0000 in image, so just use 'byte'
    access.codeAccessBase = &myCodeAccessBase[byte];
    mySystem->setPageAccess(address>>shift, access);

    // TODO: Stephen: in this and other implementations we appear to be using "shift" as a system-dependant mangle for
    // different access types (byte/int/32bit) on different architectures. I think I understand that much. However,
    // I have an issue with how these loops are encoded in all the bank schemes I've seen. The issue being that
    // the loop inits set some address (e.g., 0x1800) and then add a shifted value to it every loop. But when
    // they want to get the downshifted value, they just shift down the whole value. Which will also shift down that base address
    // and that makes no sense at all to me. I don't understand it.
  }

  // Initialise bank values for the 4x 1K bank areas
  // This is used to reverse-lookup from address to bank location
  for(uInt32 b = 0; b < 3; b++)
    bankInUse[b] = BANK_UNDEFINED;        // bank is undefined and inaccessible!

  // Install pages for the startup bank into the first segment
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeDASH::peek(uInt16 address)
{
  uInt8 value = 0;
  uInt32 bank = (address >> ROM_BANK_TO_POWER) & 3;   	// convert to 1K bank index (0-3)
  Int16 imageBank = bankInUse[bank];        			// the ROM/RAM bank that's here

  if(imageBank == BANK_UNDEFINED)						// an uninitialised bank?
  {
    // accessing invalid bank, so return should be... random?
    // TODO: Stephen -- throw some sort of error; looking at undefined data

    assert(false);
    value = mySystem->randGenerator().next();
  }
  else if (imageBank & ROMRAM) // a RAM bank
  {
	Int32 ramBank = imageBank & BIT_BANK_MASK;		// discard irrelevant bits
	Int32 offset = ramBank << RAM_BANK_TO_POWER;  // base bank address in RAM
	offset += (address & (RAM_BANK_SIZE-1));      // + byte offset in RAM bank
	value = myRAM[offset];
  }
  else	// accessing ROM
  {
    Int32 offset = imageBank << ROM_BANK_TO_POWER;  // base bank address in image
    offset += (address & (ROM_BANK_SIZE-1));        // + byte offset in image bank
    value = myImage[offset];
  }

  return value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDASH::poke(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;    // restrict to 4K address range

  // Check for write to the bank switch address. RAM/ROM and bank # are encoded in 'value'
  // There are NO mirrored hotspots.

  if ( address == BANK_SWITCH_HOTSPOT)
	  bank(value);

  // Pass the poke through to the TIA. In a real Atari, both the cart and the
  // TIA see the address lines, and both react accordingly. In Stella, each
  // 64-byte chunk of address space is "owned" by only one device. If we
  // don't chain the poke to the TIA, then the TIA can't see it...
  mySystem->tia().poke(address, value);

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDASH::bank(uInt16 bank)
{
  if(bankLocked())
    return false;   // TODO: Stephen -- ? no idea

  uInt16 shift = mySystem->pageShift();

  uInt16 bankNumber = (bank >> BANK_BITS) & 3;  // which bank # we are switching TO (BITS D6,D7)
  uInt16 bankID = bank & BIT_BANK_MASK;         // The actual bank # to switch in (BITS D5-D0)

  if(bank & ROMRAM)   // switching to a 512 byte RAM bank
  {
    // Wrap around/restrict to valid range
    uInt16 currentBank = (bank & BIT_BANK_MASK) % RAM_BANK_COUNT;
    // Record which bank switched in (marked as RAM)
    myCurrentBank = bankInUse[bankNumber] = (Int16) (ROM_BANK_COUNT + currentBank);
    // Effectively * 512 bytes
    uInt32 startCurrentBank = currentBank << RAM_BANK_TO_POWER;

    // Setup the page access methods for the current bank
    System::PageAccess access(0, 0, 0, this, System::PA_READ);

    // Map read-port RAM image into the system
    for(uInt32 byte = 0; byte < RAM_BANK_SIZE; byte += (1 << shift))
    {
      access.directPeekBase = &myRAM[startCurrentBank + byte];

      // TODO: Stephen please explain/review the use of mySize as an offset for RAM access here....
      access.codeAccessBase = &myCodeAccessBase[mySize + startCurrentBank + byte];  //?eh

      mySystem->setPageAccess((startCurrentBank + byte) >> shift, access);
    }

    access.directPeekBase = 0;
    access.type = System::PA_WRITE;

    // Map write-port RAM image into the system
    for (uInt32 byte = 0; byte < RAM_BANK_SIZE; byte += (1 << shift))
    {
      access.directPokeBase = &myRAM[startCurrentBank + RAM_WRITE_OFFSET + byte];
      access.codeAccessBase = &myCodeAccessBase[mySize + startCurrentBank + RAM_WRITE_OFFSET + byte];
      mySystem->setPageAccess((startCurrentBank + byte) >> shift, access);
    }
  }
  else  // ROM 1K banks
  {
    // Map ROM bank image into the system into the correct slot
    // Memory map is 1K slots at 0x1000, 0x1400, 0x1800, 0x1C00

    // Record which bank switched in (as ROM)
    myCurrentBank = bankInUse[bankNumber] = (Int16) bankID;
    // Effectively *1K
    uInt32 startCurrentBank = bankID << ROM_BANK_TO_POWER;

    // Setup the page access methods for the current bank
    System::PageAccess access(0, 0, 0, this, System::PA_READ);

    uInt32 bankStart = 0x1000 + (bankNumber << ROM_BANK_TO_POWER); // *1K

    for (uInt32 byte = 0; byte < ROM_BANK_SIZE; byte += (1 << shift))
    {
      access.directPeekBase = &myImage[startCurrentBank + byte];
      access.codeAccessBase = &myCodeAccessBase[startCurrentBank + byte];
      mySystem->setPageAccess((bankStart + byte) >> shift, access);
    }
  }

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeDASH::bank() const
{
  // TODO: Stephen -- what to do here?  We don't really HAVE a "current bank"; we have 4 banks
  // and they are defined in bankInUse[...].
  // What I've done is kept track of the last switched bank, and return that. But that doesn't tell us WHERE. :(

  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeDASH::bankCount() const
{
  // Because the RAM banks always start above the ROM banks (see ROM_BANK_COUNT) for value,
  // we require the number of ROM banks to be == ROM_BANK_COUNT. Banks are therefore 0-63 ROM 64-127 RAM
  // TODO: Stephen -- ROM banks are 1K. RAM banks are 512 bytes. How does this affect what this routine should return?
  return ROM_BANK_COUNT + RAM_BANK_COUNT;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDASH::patch(uInt16 address, uInt8 value)
{
  // Patch the cartridge ROM
  // TODO: Stephen... I assume this is for some sort of debugger support....?

  myBankChanged = true;

  uInt32 bankNumber = (address >> 10) & 3;        // now 1K bank # (ie: 0-3)
  Int32 whichBankIsThere = bankInUse[bankNumber]; // ROM or RAM bank reference

  if(whichBankIsThere <= BANK_UNDEFINED)  
  {
    // We're trying to access undefined memory (no bank here yet)

    // TODO: Stephen -- what to do here? Fail silently?
    // want to throw some sort of Stella error -- trying to patch an unswitched in bank!

    assert(false);
    myBankChanged = false;
  }
  else if(whichBankIsThere < ROM_BANK_COUNT)  // patching ROM (1K banks)
  {
    uInt32 byteOffset = address & (ROM_BANK_SIZE-1);
    uInt32 baseAddress = (whichBankIsThere << ROM_BANK_TO_POWER) + byteOffset;
    myImage[baseAddress] = value;   // write to the image
  }
  else  // patching RAM (512 byte banks)
  {
    uInt32 byteOffset = address & (RAM_BANK_SIZE-1);
    uInt32 baseAddress = ((whichBankIsThere - ROM_BANK_COUNT) << RAM_BANK_TO_POWER) + byteOffset;
    myRAM[baseAddress] = value;     // write to RAM
  }

  return myBankChanged;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeDASH::getImage(int& size) const
{
  size = mySize;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDASH::save(Serializer& out) const
{
  try
  {
    out.putString(name());
    out.putShort(myCurrentBank);
    for(uInt32 bank = 0; bank < 4; bank++)
      out.putShort(bankInUse[bank]);
    out.putByteArray(myRAM, RAM_TOTAL_SIZE);
  }
  catch (...)
  {
    cerr << "ERROR: CartridgeDASH::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDASH::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;
    myCurrentBank = in.getShort();
    for(uInt32 bank = 0; bank < 4; bank++)
      bankInUse[bank] = in.getShort();
    in.getByteArray(myRAM, RAM_TOTAL_SIZE);
  }
  catch (...)
  {
    cerr << "ERROR: CartridgeDASH::load" << endl;
    return false;
  }

  // Now, go to the current bank
  bank(myCurrentBank);

  return true;
}
