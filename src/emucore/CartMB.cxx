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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <cassert>
#include <cstring>

#include "System.hxx"
#include "CartMB.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeMB::CartridgeMB(const uInt8* image)
{
  // Copy the ROM image into my buffer
  memcpy(myImage, image, 65536);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeMB::~CartridgeMB()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMB::reset()
{
  // Upon reset we switch to bank 1
  myCurrentBank = 0;
  incbank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMB::install(System& system)
{
  mySystem = &system;
  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  // Make sure the system we're being installed in has a page size that'll work
  assert((0x1000 & mask) == 0);

  // Set the page accessing methods for the hot spots
  System::PageAccess access;
  for(uInt32 i = (0x1FF0 & ~mask); i < 0x2000; i += (1 << shift))
  {
    access.directPeekBase = 0;
    access.directPokeBase = 0;
    access.device = this;
    mySystem->setPageAccess(i >> shift, access);
  }

  // Install pages for bank 1
  myCurrentBank = 0;
  incbank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeMB::peek(uInt16 address)
{
  address &= 0x0FFF;

  // Switch to next bank
  if(address == 0x0FF0)
    incbank();

  return myImage[(myCurrentBank << 12) + address];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMB::poke(uInt16 address, uInt8)
{
  address &= 0x0FFF;

  // Switch to next bank
  if(address == 0x0FF0)
    incbank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMB::incbank()
{
  if(myBankLocked) return;

  // Remember what bank we're in
  myCurrentBank ++;
  myCurrentBank &= 0x0F;
  uInt16 offset = myCurrentBank << 12;
  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  // Setup the page access methods for the current bank
  System::PageAccess access;
  access.device = this;
  access.directPokeBase = 0;

  // Map ROM image into the system
  for(uInt32 address = 0x1000; address < (0x1FF0U & ~mask);
      address += (1 << shift))
  {
    access.directPeekBase = &myImage[offset + (address & 0x0FFF)];
    mySystem->setPageAccess(address >> shift, access);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMB::bank(uInt16 bank)
{
  if(myBankLocked) return;

  myCurrentBank = bank - 1;
  incbank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartridgeMB::bank()
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartridgeMB::bankCount()
{
  return 16;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMB::patch(uInt16 address, uInt8 value)
{
  myImage[(myCurrentBank << 12) + (address & 0x0FFF)] = value;
  return true;
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8* CartridgeMB::getImage(int& size)
{
  size = 65536;
  return &myImage[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMB::save(Serializer& out) const
{
  const string& cart = name();

  try
  {
    out.putString(cart);

    out.putInt(myCurrentBank);
  }
  catch(const char* msg)
  {
    cerr << "ERROR: CartridgeMB::save" << endl << "  " << msg << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMB::load(Serializer& in)
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
    cerr << "ERROR: CartridgeMB::load" << endl << "  " << msg << endl;
    return false;
  }

  // Remember what bank we were in
  --myCurrentBank;
  incbank();

  return true;
}
