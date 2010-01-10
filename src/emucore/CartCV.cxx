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
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <cassert>
#include <cstring>

#include "System.hxx"
#include "CartCV.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCV::CartridgeCV(const uInt8* image, uInt32 size)
  : myROM(0),
    mySize(size)
{
  myROM = new uInt8[mySize];
  memcpy(myROM, image, mySize);

  // This cart contains 1024 bytes extended RAM @ 0x1000
  registerRamArea(0x1000, 1024, 0x00, 0x400);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCV::~CartridgeCV()
{
  delete[] myROM;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCV::reset()
{
  if(mySize == 2048)
  {
    // Copy the ROM data into my buffer
    memcpy(myImage, myROM, 2048);

    // Initialize RAM with random values
    for(uInt32 i = 0; i < 1024; ++i)
      myRAM[i] = mySystem->randGenerator().next();
  }
  else if(mySize == 4096)
  {
    // The game has something saved in the RAM
    // Useful for MagiCard program listings

    // Copy the ROM data into my buffer
    memcpy(myImage, myROM + 2048, 2048);

    // Copy the RAM image into my buffer
    memcpy(myRAM, myROM, 1024);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCV::install(System& system)
{
  mySystem = &system;
  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  // Make sure the system we're being installed in has a page size that'll work
  assert((0x1800 & mask) == 0);

  System::PageAccess access;

  // Map ROM image into the system
  for(uInt32 address = 0x1800; address < 0x2000; address += (1 << shift))
  {
    access.device = this;
    access.directPeekBase = &myImage[address & 0x07FF];
    access.directPokeBase = 0;
    mySystem->setPageAccess(address >> mySystem->pageShift(), access);
  }

  // Set the page accessing method for the RAM writing pages
  for(uInt32 j = 0x1400; j < 0x1800; j += (1 << shift))
  {
    access.device = this;
    access.directPeekBase = 0;
    access.directPokeBase = &myRAM[j & 0x03FF];
    mySystem->setPageAccess(j >> shift, access);
  }

  // Set the page accessing method for the RAM reading pages
  for(uInt32 k = 0x1000; k < 0x1400; k += (1 << shift))
  {
    access.device = this;
    access.directPeekBase = &myRAM[k & 0x03FF];
    access.directPokeBase = 0;
    mySystem->setPageAccess(k >> shift, access);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeCV::peek(uInt16 address)
{
  if((address & 0x0FFF) < 0x0800)  // Write port is at 0xF400 - 0xF800 (1024 bytes)
  {                                // Read port is handled in ::install()
    // Reading from the write port triggers an unwanted write
    uInt8 value = mySystem->getDataBusState(0xFF);

    if(myBankLocked) return value;
    else return myRAM[address & 0x03FF] = value;
  }  
  else
  {
    return myImage[address & 0x07FF];
  }  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCV::poke(uInt16, uInt8)
{
  // This is ROM so poking has no effect :-)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCV::bank(uInt16 bank)
{
  // Doesn't support bankswitching
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartridgeCV::bank()
{
  // Doesn't support bankswitching
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartridgeCV::bankCount()
{
  return 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCV::patch(uInt16 address, uInt8 value)
{
  myImage[address & 0x07FF] = value;
  return true;
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8* CartridgeCV::getImage(int& size)
{
  size = 2048;
  return &myImage[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCV::save(Serializer& out) const
{
  const string& cart = name();

  try
  {
    out.putString(cart);

    // Output RAM
    out.putInt(1024);
    for(uInt32 addr = 0; addr < 1024; ++addr)
      out.putByte((char)myRAM[addr]);
  }
  catch(const char* msg)
  {
    cerr << "ERROR: CartridgeCV::save" << endl << "  " << msg << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCV::load(Serializer& in)
{
  const string& cart = name();

  try
  {
    if(in.getString() != cart)
      return false;

    // Input RAM
    uInt32 limit = (uInt32) in.getInt();
    for(uInt32 addr = 0; addr < limit; ++addr)
      myRAM[addr] = (uInt8) in.getByte();
  }
  catch(const char* msg)
  {
    cerr << "ERROR: CartridgeCV::load" << endl << "  " << msg << endl;
    return false;
  }

  return true;
}
