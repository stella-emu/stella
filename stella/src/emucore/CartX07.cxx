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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: 
//============================================================================

#include <cassert>

#include "System.hxx"
#include "M6532.hxx"
#include "TIA.hxx"
#include "CartX07.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeX07::CartridgeX07(const uInt8* image)
{
  // Copy the ROM image into my buffer
  for(uInt32 addr = 0; addr < 65536; ++addr)
  {
    myImage[addr] = image[addr];
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeX07::~CartridgeX07()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeX07::reset()
{
  // Upon reset we switch to bank 0
  bank(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeX07::install(System& system)
{
  mySystem = &system;
  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  // Make sure the system we're being installed in has a page size that'll work
  assert((0x1000 & mask) == 0);

  // Set the page accessing methods for the hot spots
  // The hotspots use almost all addresses below 0x1000, so we simply grab them
  // all and forward the TIA/RIOT calls from the peek and poke methods.
  System::PageAccess access;
  for(uInt32 i = 0x00; i < 0x1000; i += (1 << shift))
  {
    access.directPeekBase = 0;
    access.directPokeBase = 0;
    access.device = this;
    mySystem->setPageAccess(i >> shift, access);
  }

  // Install pages for bank 0
  bank(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeX07::peek(uInt16 address)
{
  uInt8 value = 0;

  // Check for RAM or TIA mirroring
  uInt16 lowAddress = address & 0x3ff;
  if(lowAddress & 0x80)
    value = mySystem->m6532().peek(address);
  else if(!(lowAddress & 0x200))
    value = mySystem->tia().peek(address);

  // Switch banks if necessary
  if((address & 0x180f) == 0x080d) bank((address & 0xf0) >> 4);
  else if((address & 0x1880) == 0)
  {
    if((myCurrentBank & 0xe) == 0xe)
      bank(((address & 0x40) >> 6) | (myCurrentBank & 0xe));
  }

  return value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeX07::poke(uInt16 address, uInt8 value)
{
  // Check for RAM or TIA mirroring
  uInt16 lowAddress = address & 0x3ff;
  if(lowAddress & 0x80)
    mySystem->m6532().poke(address, value);
  else if(!(lowAddress & 0x200))
    mySystem->tia().poke(address, value);

  // Switch banks if necessary
  if((address & 0x180f) == 0x080d) bank((address & 0xf0) >> 4);
  else if((address & 0x1880) == 0)
  {
    if((myCurrentBank & 0xe) == 0xe)
      bank(((address & 0x40) >> 6) | (myCurrentBank & 0xe));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeX07::bank(uInt16 bank)
{ 
  if(myBankLocked) return;

  // Remember what bank we're in
  myCurrentBank = (bank & 0x0f);
  uInt32 offset = myCurrentBank * 4096;
  uInt16 shift = mySystem->pageShift();

  // Setup the page access methods for the current bank
  System::PageAccess access;
  access.device = this;
  access.directPokeBase = 0;

  // Map ROM image into the system
  for(uInt32 address = 0x1000; address < 0x2000; address += (1 << shift))
  {
    access.directPeekBase = &myImage[offset + (address & 0x0FFF)];
    mySystem->setPageAccess(address >> shift, access);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartridgeX07::bank()
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartridgeX07::bankCount()
{
  return 16;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeX07::patch(uInt16 address, uInt8 value)
{
  address &= 0x0fff;
  myImage[myCurrentBank * 4096] = value;
  bank(myCurrentBank); // TODO: see if this is really necessary
  return true;
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8* CartridgeX07::getImage(int& size)
{
  size = 65536;
  return &myImage[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeX07::save(Serializer& out) const
{
  string cart = name();

  try
  {
    out.putString(cart);
    out.putInt(myCurrentBank);
  }
  catch(char *msg)
  {
    cerr << msg << endl;
    return false;
  }
  catch(...)
  {
    cerr << "Unknown error in save state for " << cart << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeX07::load(Deserializer& in)
{
  string cart = name();

  try
  {
    if(in.getString() != cart)
      return false;

    myCurrentBank = (uInt16)in.getInt();
  }
  catch(char *msg)
  {
    cerr << msg << endl;
    return false;
  }
  catch(...)
  {
    cerr << "Unknown error in load state for " << cart << endl;
    return false;
  }

  // Remember what bank we were in
  bank(myCurrentBank);

  return true;
}
