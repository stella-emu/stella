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
// Copyright (c) 1995-2010 by Bradford W. Mott, Stephen Anthony
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
#include "CartEFSC.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeEFSC::CartridgeEFSC(const uInt8* image)
{
  // Copy the ROM image into my buffer
  memcpy(myImage, image, 65536);

  // This cart contains 128 bytes extended RAM @ 0x1000
  registerRamArea(0x1000, 128, 0x80, 0x00);

  // Remember startup bank
  myStartBank = 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeEFSC::~CartridgeEFSC()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEFSC::reset()
{
  // Initialize RAM with random values
  for(uInt32 i = 0; i < 128; ++i)
    myRAM[i] = mySystem->randGenerator().next();

  // Upon reset we switch to the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEFSC::install(System& system)
{
  mySystem = &system;
  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  // Make sure the system we're being installed in has a page size that'll work
  assert((0x1000 & mask) == 0);

  // Set the page accessing methods for the hot spots
  System::PageAccess access;
  for(uInt32 i = (0x1FE0 & ~mask); i < 0x2000; i += (1 << shift))
  {
    access.directPeekBase = 0;
    access.directPokeBase = 0;
    access.device = this;
    mySystem->setPageAccess(i >> shift, access);
  }

  // Set the page accessing method for the RAM writing pages
  for(uInt32 j = 0x1000; j < 0x1080; j += (1 << shift))
  {
    access.device = this;
    access.directPeekBase = 0;
    access.directPokeBase = &myRAM[j & 0x007F];
    mySystem->setPageAccess(j >> shift, access);
  }
 
  // Set the page accessing method for the RAM reading pages
  for(uInt32 k = 0x1080; k < 0x1100; k += (1 << shift))
  {
    access.device = this;
    access.directPeekBase = &myRAM[k & 0x007F];
    access.directPokeBase = 0;
    mySystem->setPageAccess(k >> shift, access);
  }

  // Install pages for the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeEFSC::peek(uInt16 address)
{
  uInt16 peekAddress = address;
  address &= 0x0FFF;

  // Switch banks if necessary
  if((address >= 0x0FE0) && (address <= 0x0FEF))
    bank(address - 0x0FE0);

  if(address < 0x0080)  // Write port is at 0xF000 - 0xF080 (128 bytes)
  {
    // Reading from the write port triggers an unwanted write
    uInt8 value = mySystem->getDataBusState(0xFF);

    if(bankLocked())
      return value;
    else
    {
      triggerReadFromWritePort(peekAddress);
      return myRAM[address] = value;
    }
  }  
  else
    return myImage[(myCurrentBank << 12) + address];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEFSC::poke(uInt16 address, uInt8)
{
  address &= 0x0FFF;

  // Switch banks if necessary
  if((address >= 0x0FE0) && (address <= 0x0FEF))
    bank(address - 0x0FE0);

  // NOTE: This does not handle accessing RAM, however, this function
  // should never be called for RAM because of the way page accessing
  // has been setup
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEFSC::bank(uInt16 bank)
{
  if(bankLocked()) return;

  // Remember what bank we're in
  myCurrentBank = bank;
  uInt16 offset = myCurrentBank << 12;
  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  // Setup the page access methods for the current bank
  System::PageAccess access;
  access.device = this;
  access.directPokeBase = 0;

  // Map ROM image into the system
  for(uInt32 address = 0x1100; address < (0x1FE0U & ~mask);
      address += (1 << shift))
  {
    access.directPeekBase = &myImage[offset + (address & 0x0FFF)];
    mySystem->setPageAccess(address >> shift, access);
  }
  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeEFSC::bank() const
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeEFSC::bankCount() const
{
  return 16;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEFSC::patch(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

  if(address < 0x0100)
  {
    // Normally, a write to the read port won't do anything
    // However, the patch command is special in that ignores such
    // cart restrictions
    myRAM[address & 0x007F] = value;
  }
  else
    myImage[(myCurrentBank << 12) + address] = value;

  return myBankChanged = true;
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeEFSC::getImage(int& size) const
{
  size = 65536;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEFSC::save(Serializer& out) const
{
  const string& cart = name();

  try
  {
    out.putString(cart);

    out.putInt(myCurrentBank);
  }
  catch(const char* msg)
  {
    cerr << "ERROR: CartridgeEFSC::save" << endl << "  " << msg << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEFSC::load(Serializer& in)
{
  const string& cart = name();

  try
  {
    if(in.getString() != cart)
      return false;

    myCurrentBank = (uInt16) in.getInt();
  }
  catch(const char* msg)
  {
    cerr << "ERROR: CartridgeEFSC::load" << endl << "  " << msg << endl;
    return false;
  }

  // Remember what bank we were in
  bank(myCurrentBank);

  return true;
}
