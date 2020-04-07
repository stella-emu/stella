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
#include "CartEnhanced.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeEnhanced::CartridgeEnhanced(const ByteBuffer& image, size_t size,
                         const string& md5, const Settings& settings)
  : Cartridge(settings, md5),
    mySize(size)
{
  // Allocate array for the ROM image (at least 64 bytzes)
  myImage = make_unique<uInt8[]>(std::max(uInt16(mySize), System::PAGE_SIZE));

  // Copy the ROM image into my buffer
  std::copy_n(image.get(), mySize, myImage.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEnhanced::install(System& system)
{
  // Copy the ROM image into my buffer
  createRomAccessArrays(mySize);

  // calculate bank switching and RAM sizes and masks
  myBankSize = 1 << myBankShift;          // e.g. = 2 ^ 12 = 4K = 0x1000
  myBankMask = myBankSize - 1;            // e.g. = 0x0FFF
  myBankSegs = 1 << (12 - myBankShift);   // e.g. = 1
  myRamMask = myRamSize - 1;              // e.g. = 0xFFFF (doesn't matter for RAM size 0)

  // Allocate array for the current bank segments slices
  myCurrentSegOffset = make_unique<uInt32[]>(myBankSegs);

  // Allocate array for the RAM area
  myRAM = make_unique<uInt8[]>(myRamSize);

  // Setup page access
  mySystem = &system;

  System::PageAccess access(this, System::PageAccessType::READ);

  // Set the page accessing method for the RAM writing pages
  // Map access to this class, since we need to inspect all accesses to
  // check if RWP happens
  access.type = System::PageAccessType::WRITE;
  for(uInt16 addr = 0x1000; addr < 0x1000 + myRamSize; addr += System::PAGE_SIZE)
  {
    uInt16 offset = addr & myRamMask;
    access.romAccessBase = &myRomAccessBase[offset];
    access.romPeekCounter = &myRomAccessCounter[offset];
    access.romPokeCounter = &myRomAccessCounter[offset + myAccessSize];
    mySystem->setPageAccess(addr, access);
  }

  // Set the page accessing method for the RAM reading pages
  access.type = System::PageAccessType::READ;
  for(uInt16 addr = 0x1000 + myRamSize; addr < 0x1000 + myRamSize * 2; addr += System::PAGE_SIZE)
  {
    uInt16 offset = addr & myRamMask;
    access.directPeekBase = &myRAM[offset];
    access.romAccessBase = &myRomAccessBase[myRamSize + offset];
    access.romPeekCounter = &myRomAccessCounter[myRamSize + offset];
    access.romPokeCounter = &myRomAccessCounter[myRamSize + offset + myAccessSize];
    mySystem->setPageAccess(addr, access);
  }

  // Install pages for the startup bank (TODO: currently only in first bank segment)
  bank(startBank(), 0);
  if(mySize >= 4_KB && myBankSegs > 1)
    // Setup the last bank segment to always point to the last ROM segment
    bank(bankCount() - 1, myBankSegs - 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEnhanced::reset()
{
  initializeRAM(myRAM.get(), myRamSize);

  initializeStartBank(getStartBank());

  // Upon reset we switch to the reset bank
  bank(startBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeEnhanced::peek(uInt16 address)
{
  uInt16 peekAddress = address;

  if (hotspot())
    checkSwitchBank(address & 0x0FFF);
  address &= myBankMask;

  if(address < myRamSize)  // Write port is at 0xF000 - 0xF07F (128 bytes)
    return peekRAM(myRAM[address], peekAddress);
  else
    return myImage[myCurrentSegOffset[(peekAddress & 0xFFF) >> myBankShift] + address];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEnhanced::poke(uInt16 address, uInt8 value)
{
  // Switch banks if necessary
  if (checkSwitchBank(address & 0x0FFF, value))
    return false;

  if(myRamSize)
  {
    if(!(address & myRamSize))
    {
      pokeRAM(myRAM[address & myRamMask], address, value);
      return true;
    }
    else
    {
      // Writing to the read port should be ignored, but trigger a break if option enabled
      uInt8 dummy;

      pokeRAM(dummy, address, value);
      myRamWriteAccess = address;
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEnhanced::bank(uInt16 bank, uInt16 segment)
{
  if(bankLocked()) return false;

  // Remember what bank is in which segment
  uInt32 bankOffset = myCurrentSegOffset[segment] = bank << myBankShift;
  uInt16 segmentOffset = segment << myBankShift;
  uInt16 hotspot = this->hotspot();
  uInt16 hotSpotAddr;
  uInt16 fromAddr = (segmentOffset + 0x1000 + myRamSize * 2) & ~System::PAGE_MASK;
  // for ROMs < 4_KB, the whole address space will be mapped.
  uInt16 toAddr = (segmentOffset + 0x1000 + (mySize < 4_KB ? 0x1000 : myBankSize)) & ~System::PAGE_MASK;

  if(hotspot)
    hotSpotAddr = (hotspot & ~System::PAGE_MASK);
  else
    hotSpotAddr = 0xFFFF; // none

  System::PageAccess access(this, System::PageAccessType::READ);

  // Setup the page access methods for the current bank
  for(uInt16 addr = fromAddr; addr < toAddr; addr += System::PAGE_SIZE)
  {
    uInt32 offset = bankOffset + (addr & myBankMask);

    if(myDirectPeek && addr != hotSpotAddr)
      access.directPeekBase = &myImage[offset];
    else
      access.directPeekBase = nullptr;
    access.romAccessBase = &myRomAccessBase[offset];
    access.romPeekCounter = &myRomAccessCounter[offset];
    access.romPokeCounter = &myRomAccessCounter[offset + myAccessSize];
    mySystem->setPageAccess(addr, access);
  }

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeEnhanced::getBank(uInt16 address) const
{
  return myCurrentSegOffset[(address & 0xFFF) >> myBankShift] >> myBankShift;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeEnhanced::getSegmentBank(uInt16 segment) const
{
  return myCurrentSegOffset[segment % myBankSegs] >> myBankShift;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeEnhanced::bankCount() const
{
  return uInt16(mySize >> myBankShift);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEnhanced::patch(uInt16 address, uInt8 value)
{
  if((address & myBankMask) < myRamSize * 2)
  {
    // Normally, a write to the read port won't do anything
    // However, the patch command is special in that ignores such
    // cart restrictions
    myRAM[address & myRamMask] = value;
  }
  else
    myImage[myCurrentSegOffset[(address & 0xFFF) >> myBankShift] + (address & myBankMask)] = value;

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeEnhanced::getImage(size_t& size) const
{
  size = mySize;
  return myImage.get();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEnhanced::save(Serializer& out) const
{
  try
  {
    out.putIntArray(myCurrentSegOffset.get(), myBankSegs);
    if(myRamSize)
      out.putByteArray(myRAM.get(), myRamSize);

  }
  catch(...)
  {
    cerr << "ERROR: << " << name() << "::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEnhanced::load(Serializer& in)
{
  try
  {
    in.getIntArray(myCurrentSegOffset.get(), myBankSegs);
    if(myRamSize)
      in.getByteArray(myRAM.get(), myRamSize);
  }
  catch(...)
  {
    cerr << "ERROR: " << name() << "::load" << endl;
    return false;
  }
  // Restore bank sewgments
  for(uInt16 i = 0; i < myBankSegs; ++i)
    bank(getSegmentBank(i), i);

  return true;
}
