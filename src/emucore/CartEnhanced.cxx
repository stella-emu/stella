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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Logger.hxx"
#include "System.hxx"
#include "PlusROM.hxx"
#include "CartEnhanced.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeEnhanced::CartridgeEnhanced(const ByteBuffer& image, size_t size,
                                     const string& md5, const Settings& settings,
                                     size_t bsSize)
  : Cartridge(settings, md5)
{
  // ROMs are not always at the 'legal' size for their associated
  // bankswitching scheme; here we deal with the differing sizes

  // Is the ROM too large?  If so, we cap it
  if(size > bsSize)
  {
    ostringstream buf;
    buf << "ROM larger than expected (" << size << " > " << bsSize
        << "), truncating " << (size - bsSize) << " bytes\n";
    Logger::info(buf.str());
  }
  else if(size < mySize)
  {
    ostringstream buf;
    buf << "ROM smaller than expected (" << mySize << " > " << size
        << "), appending " << (mySize - size) << " bytes\n";
    Logger::info(buf.str());
  }

  mySize = bsSize;

  // Initialize ROM with all 0's, to fill areas that the ROM may not cover
  myImage = make_unique<uInt8[]>(mySize);
  std::fill_n(myImage.get(), mySize, 0);

  // Directly copy the ROM image into the buffer
  // Only copy up to the amount of data the ROM provides; extra unused
  // space will be filled with 0's from above
  std::copy_n(image.get(), std::min(mySize, size), myImage.get());

#if 0
  // Determine whether we have a PlusROM cart
  // PlusROM needs to call peek() method, so disable direct peeks
  if(myPlusROM.initialize(myImage, mySize))
    myDirectPeek = false;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEnhanced::install(System& system)
{
  // limit banked RAM size to the size of one RAM bank
  const uInt16 ramSize = myRamBankCount > 0 ? 1 << (myBankShift - 1) : uInt16(myRamSize);

  // calculate bank switching and RAM sizes and masks
  myBankSize = 1 << myBankShift;                    // e.g. = 2 ^ 12 = 4K = 0x1000
  myBankMask = myBankSize - 1;                      // e.g. = 0x0FFF
  // Either the bankswitching supports multiple segments
  //  or the ROM is < 4K (-> 1 segment)
  myBankSegs = std::min(1 << (MAX_BANK_SHIFT - myBankShift),
                        int(mySize) / myBankSize);  // e.g. = 1
  // ROM has an offset if RAM inside a bank (e.g. for F8SC)
  myRomOffset = myRamBankCount > 0U ? 0U : static_cast<uInt16>(myRamSize * 2);
  myRamMask = ramSize - 1;                          // e.g. = 0xFFFF (doesn't matter for RAM size 0)
  myWriteOffset = myRamWpHigh ? ramSize : 0;        // e.g. = 0x0000
  myReadOffset  = myRamWpHigh ? 0 : ramSize;        // e.g. = 0x0080
  // Allocate more space only if RAM has its own bank(s)
  createRomAccessArrays(mySize + (myRomOffset > 0 ? 0 : myRamSize));

  // Allocate array for the segment's current bank offset
  myCurrentSegOffset = make_unique<uInt32[]>(myBankSegs);

  // Allocate array for the RAM area
  if(myRamSize > 0)
    myRAM = make_unique<uInt8[]>(myRamSize);

  mySystem = &system;

  if(myRomOffset > 0)
  {
    // Setup page access for extended RAM; banked RAM will be setup in bank()
    System::PageAccess access(this, System::PageAccessType::READ);

    // Set the page accessing method for the RAM writing pages
    // Note: Writes are mapped to poke() (NOT using directPokeBase) to check for read from write port (RWP)
    access.type = System::PageAccessType::WRITE;
    for(uInt16 addr = ROM_OFFSET + myWriteOffset; addr < ROM_OFFSET + myWriteOffset + myRamSize; addr += System::PAGE_SIZE)
    {
      const uInt16 offset = addr & myRamMask;

      access.romAccessBase = &myRomAccessBase[myWriteOffset + offset];
      access.romPeekCounter = &myRomAccessCounter[myWriteOffset + offset];
      access.romPokeCounter = &myRomAccessCounter[myWriteOffset + offset + myAccessSize];
      mySystem->setPageAccess(addr, access);
    }

    // Set the page accessing method for the RAM reading pages
    access.type = System::PageAccessType::READ;
    for(uInt16 addr = ROM_OFFSET + myReadOffset; addr < ROM_OFFSET + myReadOffset + myRamSize; addr += System::PAGE_SIZE)
    {
      const uInt16 offset = addr & myRamMask;

      access.directPeekBase = &myRAM[offset];
      access.romAccessBase = &myRomAccessBase[myReadOffset + offset];
      access.romPeekCounter = &myRomAccessCounter[myReadOffset + offset];
      access.romPokeCounter = &myRomAccessCounter[myReadOffset + offset + myAccessSize];
      mySystem->setPageAccess(addr, access);
    }
  }

  // Install pages for the startup bank (TODO: currently only in first bank segment)
  bank(startBank(), 0);
  if(mySize >= 4_KB && myBankSegs > 1)
    // Setup the last bank segment to always point to the last ROM segment
    bank(romBankCount() - 1, myBankSegs - 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEnhanced::reset()
{
  if(myRamSize > 0)
    initializeRAM(myRAM.get(), myRamSize);

  initializeStartBank(getStartBank());

  // Upon reset we switch to the reset bank
  bank(startBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeEnhanced::peek(uInt16 address)
{
  const uInt16 peekAddress = address;

#if 0
  // Is this a PlusROM?
  if(myPlusROM.isValid())
  {
    uInt8 value = 0;
    if(myPlusROM.peekHotspot(address, value))
      return value;
  }
#endif

  // hotspots in TIA range are reacting to pokes only
  if (hotspot() >= 0x80)
    checkSwitchBank(address & ADDR_MASK);

  if(isRamBank(address))
  {
    address &= myRamMask;

    // This is a read access to a write port!
    // Reading from the write port triggers an unwanted write
    // The RAM banks follow the ROM banks and are half the size of a ROM bank
    return peekRAM(myRAM[ramAddressSegmentOffset(peekAddress) + address], peekAddress);
  }
  address &= ROM_MASK;

  // Write port is e.g. at 0xF000 - 0xF07F (128 bytes)
  if(address < myReadOffset + myRamSize && address >= myReadOffset)
  {
    // This is a read access to a write port!
    // Reading from the write port triggers an unwanted write
    return peekRAM(myRAM[address], peekAddress);
  }

  return myImage[romAddressSegmentOffset(peekAddress) + (peekAddress & myBankMask)];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEnhanced::poke(uInt16 address, uInt8 value)
{
#if 0
  // Is this a PlusROM?
  if(myPlusROM.isValid() && myPlusROM.pokeHotspot(address, value))
    return true;
#endif

  // Switch banks if necessary
  if (checkSwitchBank(address & ADDR_MASK, value))
    return false;

  if(myRamSize > 0)
  {
    // Code should never get here (System::PageAccess::directPoke() handles this)
    const uInt16 pokeAddress = address;

    if(isRamBank(address))
    {
      if(bool(address & (myBankSize >> 1)) == myRamWpHigh)
      {
        address &= myRamMask;
        // The RAM banks follow the ROM banks and are half the size of a ROM bank
        pokeRAM(myRAM[ramAddressSegmentOffset(pokeAddress) + address],
                pokeAddress, value);
        return true;
      }
    }
    else
    {
      //address &= myBankMask;
      if(bool(address & myRamSize) == myRamWpHigh)
      {
        pokeRAM(myRAM[address & myRamMask], pokeAddress, value);
        return true;
      }
    }
    // Writing to the read port should be ignored, but trigger a break if option enabled
    uInt8 dummy;

    pokeRAM(dummy, pokeAddress, value);
    myRamWriteAccess = pokeAddress;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEnhanced::bank(uInt16 bank, uInt16 segment)
{
  if(bankLocked()) return false;

  const uInt16 segmentOffset = segment << myBankShift;

  if(myRamBankCount == 0 || bank < romBankCount())
  {
    // Setup ROM bank
    const uInt16 romBank = bank % romBankCount();
    // Remember what bank is in this segment
    const uInt32 bankOffset = myCurrentSegOffset[segment] = romBank << myBankShift;
    const uInt16 hotspot = this->hotspot();
    uInt16 hotSpotAddr;
    // Skip extra RAM; if existing it is only mapped into first segment
    const uInt16 fromAddr = (ROM_OFFSET + segmentOffset + (segment == 0 ? myRomOffset : 0)) & ~System::PAGE_MASK;
    // for ROMs < 4_KB, the whole address space will be mapped.
    const uInt16 toAddr   = (ROM_OFFSET + segmentOffset + (mySize < 4_KB ? 4_KB : myBankSize)) & ~System::PAGE_MASK;

    if(hotspot & 0x1000)
      hotSpotAddr = (hotspot & ~System::PAGE_MASK);
    else
      hotSpotAddr = 0xFFFF; // none

    System::PageAccess access(this, System::PageAccessType::READ);
    // Setup the page access methods for the current bank
    for(uInt16 addr = fromAddr; addr < toAddr; addr += System::PAGE_SIZE)
    {
      const uInt32 offset = bankOffset + (addr & myBankMask);

      if(myDirectPeek && addr != hotSpotAddr)
        access.directPeekBase = &myImage[offset];
      else
        access.directPeekBase = nullptr;
      access.romAccessBase = &myRomAccessBase[offset];
      access.romPeekCounter = &myRomAccessCounter[offset];
      access.romPokeCounter = &myRomAccessCounter[offset + myAccessSize];
      mySystem->setPageAccess(addr, access);
    }
  }
  else
  {
    // Setup RAM bank
    const uInt16 ramBank = (bank - romBankCount()) % myRamBankCount;
    // The RAM banks follow the ROM banks and are half the size of a ROM bank
    const uInt32 bankOffset = uInt32(mySize) + (ramBank << (myBankShift - 1));

    // Remember what bank is in this segment
    myCurrentSegOffset[segment] = uInt32(mySize) + (ramBank << myBankShift);

    // Set the page accessing method for the RAM writing pages
    // Note: Writes are mapped to poke() (NOT using directPokeBase) to check for read from write port (RWP)
    uInt16 fromAddr = (ROM_OFFSET + segmentOffset + myWriteOffset) & ~System::PAGE_MASK;
    uInt16 toAddr   = (ROM_OFFSET + segmentOffset + myWriteOffset + (myBankSize >> 1)) & ~System::PAGE_MASK;
    System::PageAccess access(this, System::PageAccessType::WRITE);

    for(uInt16 addr = fromAddr; addr < toAddr; addr += System::PAGE_SIZE)
    {
      const uInt32 offset = bankOffset + (addr & myRamMask);

      access.romAccessBase = &myRomAccessBase[offset];
      access.romPeekCounter = &myRomAccessCounter[offset];
      access.romPokeCounter = &myRomAccessCounter[offset + myAccessSize];
      mySystem->setPageAccess(addr, access);
    }

    // Set the page accessing method for the RAM reading pages
    fromAddr = (ROM_OFFSET + segmentOffset + myReadOffset) & ~System::PAGE_MASK;
    toAddr   = (ROM_OFFSET + segmentOffset + myReadOffset + (myBankSize >> 1)) & ~System::PAGE_MASK;
    access.type = System::PageAccessType::READ;

    for(uInt16 addr = fromAddr; addr < toAddr; addr += System::PAGE_SIZE)
    {
      const uInt32 offset = bankOffset + (addr & myRamMask);

      access.directPeekBase = &myRAM[offset - mySize];
      access.romAccessBase = &myRomAccessBase[offset];
      access.romPeekCounter = &myRomAccessCounter[offset];
      access.romPokeCounter = &myRomAccessCounter[offset + myAccessSize];
      mySystem->setPageAccess(addr, access);
    }
  }
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeEnhanced::getBank(uInt16 address) const
{
  return romAddressSegmentOffset(address) >> myBankShift;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeEnhanced::getSegmentBank(uInt16 segment) const
{
  return myCurrentSegOffset[segment % myBankSegs] >> myBankShift;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeEnhanced::romBankCount() const
{
  return uInt16(mySize >> myBankShift);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeEnhanced::ramBankCount() const
{
  return myRamBankCount;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEnhanced::isRamBank(uInt16 address) const
{
  return myRamBankCount > 0 ? getBank(address) >= romBankCount() : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEnhanced::patch(uInt16 address, uInt8 value)
{
  if(isRamBank(address))
  {
    myRAM[ramAddressSegmentOffset(address) + (address & myRamMask)] = value;
  }
  else
  {
    if(static_cast<size_t>(address & myBankMask) < myRamSize * 2)
    {
      // Normally, a write to the read port won't do anything
      // However, the patch command is special in that ignores such
      // cart restrictions
      myRAM[address & myRamMask] = value;
    }
    else
      myImage[romAddressSegmentOffset(address) + (address & myBankMask)] = value;
  }

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteBuffer& CartridgeEnhanced::getImage(size_t& size) const
{
  size = mySize;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEnhanced::save(Serializer& out) const
{
  try
  {
    out.putIntArray(myCurrentSegOffset.get(), myBankSegs);
    if(myRamSize > 0)
      out.putByteArray(myRAM.get(), myRamSize);
#if 0
    if(myPlusROM.isValid() && !myPlusROM.save(out))
      return false;
#endif
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
    if(myRamSize > 0)
      in.getByteArray(myRAM.get(), myRamSize);
#if 0
    if(myPlusROM.isValid() && !myPlusROM.load(in))
      return false;
#endif
  }
  catch(...)
  {
    cerr << "ERROR: " << name() << "::load" << endl;
    return false;
  }
  // Restore bank segments
  for(uInt16 i = 0; i < myBankSegs; ++i)
    bank(getSegmentBank(i), i);

  return true;
}
