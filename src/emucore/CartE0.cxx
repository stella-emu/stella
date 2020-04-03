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
  : Cartridge(settings, md5)
{
  // Copy the ROM image into my buffer
  std::copy_n(image.get(), std::min(myImage.size(), size), myImage.begin());
  createRomAccessArrays(myImage.size());
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
  myCurrentBank[3] = bankCount() - 1; // fixed

  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE0::install(System& system)
{
  mySystem = &system;

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
uInt16 CartridgeE0::getBank(uInt16 address) const
{
  return myCurrentBank[(address & 0xFFF) >> 10]; // 1K slices
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeE0::bankCount() const
{
  return 8;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeE0::peek(uInt16 address)
{
  address &= 0x0FFF;

  // Switch banks if necessary
  if((address >= 0x0FE0) && (address <= 0x0FE7))
  {
    bank(address & 0x0007, 0);
  }
  else if((address >= 0x0FE8) && (address <= 0x0FEF))
  {
    bank(address & 0x0007, 1);
  }
  else if((address >= 0x0FF0) && (address <= 0x0FF7))
  {
    bank(address & 0x0007, 2);
  }

  return myImage[(myCurrentBank[address >> 10] << 10) + (address & 0x03FF)];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeE0::poke(uInt16 address, uInt8)
{
  address &= 0x0FFF;

  // Switch banks if necessary
  if((address >= 0x0FE0) && (address <= 0x0FE7))
  {
    bank(address & 0x0007, 0);
  }
  else if((address >= 0x0FE8) && (address <= 0x0FEF))
  {
    bank(address & 0x0007, 1);
  }
  else if((address >= 0x0FF0) && (address <= 0x0FF7))
  {
    bank(address & 0x0007, 2);
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE0::bank(uInt16 bank, uInt16 slice)
{
  if(bankLocked()) return;

  // Remember the new slice
  myCurrentBank[slice] = bank;
  uInt16 sliceOffset = slice * (1 << 10);
  uInt16 bankOffset = bank << 10;

  // Setup the page access methods for the current bank
  System::PageAccess access(this, System::PageAccessType::READ);

  for(uInt16 addr = 0x1000 + sliceOffset; addr < 0x1000 + sliceOffset + 0x400; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[bankOffset + (addr & 0x03FF)];
    access.romAccessBase = &myRomAccessBase[bankOffset + (addr & 0x03FF)];
    access.romPeekCounter = &myRomAccessCounter[bankOffset + (addr & 0x03FF)];
    access.romPokeCounter = &myRomAccessCounter[bankOffset + (addr & 0x03FF) + myAccessSize];
    mySystem->setPageAccess(addr, access);
  }
  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeE0::patch(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;
  myImage[(myCurrentBank[address >> 10] << 10) + (address & 0x03FF)] = value;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeE0::getImage(size_t& size) const
{
  size = myImage.size();
  return myImage.data();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeE0::save(Serializer& out) const
{
  try
  {
    out.putShortArray(myCurrentBank.data(), myCurrentBank.size());
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeE0::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeE0::load(Serializer& in)
{
  try
  {
    in.getShortArray(myCurrentBank.data(), myCurrentBank.size());
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeE0::load" << endl;
    return false;
  }

  return true;
}
