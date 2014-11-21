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

#include <cstring>

#include "System.hxx"
#include "CartPPA.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgePPA::CartridgePPA(const uInt8* image, uInt32 size,
                           const Settings& settings)
  : Cartridge(settings)
{
  // Copy the ROM image into my buffer
  memcpy(myImage, image, BSPF_min(8192u, size));
  createCodeAccessBase(8192);

  // Remember startup bank
  myStartBank = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgePPA::~CartridgePPA()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgePPA::reset()
{
  // Initialize RAM
  if(mySettings.getBool("ramrandom"))
    for(uInt32 i = 0; i < 64; ++i)
      myRAM[i] = mySystem->randGenerator().next();
  else
    memset(myRAM, 0, 64);

  // Setup segments to some default slices
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgePPA::install(System& system)
{
  mySystem = &system;

  System::PageAccess access(this, System::PA_READ);

  // Set the page accessing method for the RAM writing pages
  access.type = System::PA_WRITE;
  for(uInt32 j = 0x1040; j < 0x1080; j += (1 << System::PAGE_SHIFT))
  {
    access.directPokeBase = &myRAM[j & 0x003F];
    access.codeAccessBase = &myCodeAccessBase[0x80 + (j & 0x003F)];
    mySystem->setPageAccess(j >> System::PAGE_SHIFT, access);
  }

  // Set the page accessing method for the RAM reading pages
  // The first 48 bytes map directly to RAM
  access.directPeekBase = access.directPokeBase = 0;
  access.type = System::PA_READ;
  for(uInt32 k = 0x1000; k < 0x1030; k += (1 << System::PAGE_SHIFT))
  {
    access.directPeekBase = &myRAM[k & 0x003F];
    access.codeAccessBase = &myCodeAccessBase[k & 0x003F];
    mySystem->setPageAccess(k >> System::PAGE_SHIFT, access);
  }
  access.directPeekBase = access.directPokeBase = 0;
  // The last 16 bytes are hotspots, so they're handled separately
  for(uInt32 k = 0x1030; k < 0x1040; k += (1 << System::PAGE_SHIFT))
  {
    access.codeAccessBase = &myCodeAccessBase[k & 0x003F];
    mySystem->setPageAccess(k >> System::PAGE_SHIFT, access);
  }

  // Setup segments to some default slices
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgePPA::peek(uInt16 address)
{
  uInt16 peekAddress = address;
  address &= 0x0FFF;

  if(address < 0x0040)        // RAM read port
  {
    // Hotspots at $30 - $3F
    bank(address & 0x000F);

    // Read from RAM
    return myRAM[address & 0x003F];
  }
  else if(address < 0x0080)   // RAM write port
  {
    // Reading from the write port @ $1040 - $107F triggers an unwanted write
    uInt8 value = mySystem->getDataBusState(0xFF);

    if(bankLocked())
      return value;
    else
    {
      triggerReadFromWritePort(peekAddress);
      return myRAM[address & 0x003F] = value;
    }
  }

  // NOTE: This does not handle reading from RAM, however, this 
  // function should never be called for ROM because of the
  // way page accessing has been setup
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgePPA::poke(uInt16 address, uInt8)
{
  // NOTE: This does not handle writing to RAM, however, this 
  // function should never be called for RAM because of the
  // way page accessing has been setup
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgePPA::bank(uInt16 bank)
{
  if(bankLocked() || bank > 15) return false;

  myCurrentBank = bank;

  segmentZero(ourBankOrg[bank].zero);
  segmentOne(ourBankOrg[bank].one);
  segmentTwo(ourBankOrg[bank].two);
  segmentThree(ourBankOrg[bank].three, ourBankOrg[bank].map3bytes);

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgePPA::segmentZero(uInt8 slice)
{
cerr << __func__ << " : slice " << (int)slice << endl;
  uInt16 offset = slice << 10;
  System::PageAccess access(this, System::PA_READ);

  // Skip first 128 bytes; it is always RAM
  for(uInt32 address = 0x1080; address < 0x1400;
      address += (1 << System::PAGE_SHIFT))
  {
    access.directPeekBase = &myImage[offset + (address & 0x03FF)];
    access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x03FF)];
    mySystem->setPageAccess(address >> System::PAGE_SHIFT, access);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgePPA::segmentOne(uInt8 slice)
{
cerr << __func__ << "  : slice " << (int)slice << endl;
  uInt16 offset = slice << 10;
  System::PageAccess access(this, System::PA_READ);

  for(uInt32 address = 0x1400; address < 0x1800;
      address += (1 << System::PAGE_SHIFT))
  {
    access.directPeekBase = &myImage[offset + (address & 0x03FF)];
    access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x03FF)];
    mySystem->setPageAccess(address >> System::PAGE_SHIFT, access);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgePPA::segmentTwo(uInt8 slice)
{
cerr << __func__ << "  : slice " << (int)slice << endl;
  uInt16 offset = slice << 10;
  System::PageAccess access(this, System::PA_READ);

  for(uInt32 address = 0x1800; address < 0x1C00;
      address += (1 << System::PAGE_SHIFT))
  {
    access.directPeekBase = &myImage[offset + (address & 0x03FF)];
    access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x03FF)];
    mySystem->setPageAccess(address >> System::PAGE_SHIFT, access);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgePPA::segmentThree(uInt8 slice, bool map3bytes)
{
cerr << __func__ << ": slice " << (int)slice << endl;
  uInt16 offset = slice << 10;

  // Make a copy of the address space pointed to by the slice
  // Then map in the extra 3 bytes, if required
  memcpy(mySegment3, myImage+offset, 1024);
  if(map3bytes)
  {
    mySegment3[0x3FC] = myImage[0x2000+0];
    mySegment3[0x3FD] = myImage[0x2000+1];
    mySegment3[0x3FE] = myImage[0x2000+2];
  }

  System::PageAccess access(this, System::PA_READ);

  for(uInt32 address = 0x1C00; address < 0x2000;
      address += (1 << System::PAGE_SHIFT))
  {
    access.directPeekBase = &mySegment3[address & 0x03FF];
    access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x03FF)];
    mySystem->setPageAccess(address >> System::PAGE_SHIFT, access);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgePPA::getBank() const
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgePPA::bankCount() const
{
  return 8;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgePPA::patch(uInt16 address, uInt8 value)
{
  return false;  // TODO
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgePPA::getImage(int& size) const
{
  size = 8195;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgePPA::save(Serializer& out) const
{
  try
  {
    out.putString(name());
    out.putShort(myCurrentBank);
    out.putByteArray(myRAM, 64);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgePPA::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgePPA::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    myCurrentBank = in.getShort();
    in.getByteArray(myRAM, 64);

    bank(myCurrentBank);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgePPA::load" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgePPA::BankOrg CartridgePPA::ourBankOrg[16] = {
  { 0, 0, 1, 2, false },
  { 0, 1, 3, 2, false },
  { 4, 5, 6, 7, false },
  { 7, 4, 3, 2, false },
  { 0, 0, 6, 7, false },
  { 0, 1, 7, 6, false },
  { 3, 2, 4, 5, false },
  { 6, 0, 5, 1, false },
  { 0, 0, 1, 2, false },
  { 0, 1, 3, 2, false },
  { 4, 5, 6, 7, false },
  { 7, 4, 3, 2, false },
  { 0, 0, 6, 7, true  },
  { 0, 1, 7, 6, true  },
  { 3, 2, 4, 5, true  },
  { 6, 0, 5, 1, true  }
};
