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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: CartSB.cxx,v 1.0 2007/10/11
//============================================================================

#include <cassert>

#include "System.hxx"
#include "CartSB.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeSB::CartridgeSB(const uInt8* image, uInt32 size)
  : mySize(size)
{
  // Allocate array for the ROM image
  myImage = new uInt8[mySize];

  // Copy the ROM image into my buffer
  for(uInt32 addr = 0; addr < mySize; ++addr)
  {
    myImage[addr] = image[addr];
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeSB::~CartridgeSB()
{
  delete[] myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeSB::reset()
{
  // Upon reset we switch to bank 0
  bank(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeSB::install(System& system)
{
  mySystem = &system;
  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  // Make sure the system we're being installed in has a page size that'll work
  assert((0x1000 & mask) == 0);

  // Get the page accessing methods for the hot spots since they overlap
  // areas within the TIA we'll need to forward requests to the TIA

  myHotSpotPageAccess0 = mySystem->getPageAccess(0x0800 >> shift);
  myHotSpotPageAccess1 = mySystem->getPageAccess(0x0900 >> shift);
  myHotSpotPageAccess2 = mySystem->getPageAccess(0x0A00 >> shift);
  myHotSpotPageAccess3 = mySystem->getPageAccess(0x0B00 >> shift);
  myHotSpotPageAccess4 = mySystem->getPageAccess(0x0C00 >> shift);
  myHotSpotPageAccess5 = mySystem->getPageAccess(0x0D00 >> shift);
  myHotSpotPageAccess6 = mySystem->getPageAccess(0x0E00 >> shift);
  myHotSpotPageAccess7 = mySystem->getPageAccess(0x0F00 >> shift);

  // Set the page accessing methods for the hot spots
  System::PageAccess access;

  for(uInt32 i = 0x0800; i < 0x0FFF; i += (1 << shift))
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
uInt8 CartridgeSB::peek(uInt16 address)
{
  address = address & (0x17FF+(mySize>>12));
  // Switch banks if necessary
  if ((address & 0x1800) == 0x0800) bank(address & ((mySize>>12)-1));

  if(!(address & 0x1000))
  {
    switch (address & 0x0F00)
    {
      case 0x0800:
        return myHotSpotPageAccess0.device->peek(address);
	break;
      case 0x0900:
        return myHotSpotPageAccess1.device->peek(address);
	break;
      case 0x0A00:
        return myHotSpotPageAccess2.device->peek(address);
	break;
      case 0x0B00:
        return myHotSpotPageAccess3.device->peek(address);
	break;
      case 0x0C00:
        return myHotSpotPageAccess4.device->peek(address);
	break;
      case 0x0D00:
        return myHotSpotPageAccess5.device->peek(address);
	break;
      case 0x0E00:
        return myHotSpotPageAccess6.device->peek(address);
	break;
      case 0x0F00:
        return myHotSpotPageAccess7.device->peek(address);
	break;
      default:
        break;
    }
  }

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeSB::poke(uInt16 address, uInt8 value)
{
  address = address & (0x17FF+(mySize>>12));
  // Switch banks if necessary
  if ((address & 0x1800) == 0x0800) bank(address & ((mySize>>12)-1));

  if(!(address & 0x1000))
  {

    switch (address & 0x0F00)
    {
      case 0x0800:
        return myHotSpotPageAccess0.device->poke(address, value);
	break;
      case 0x0900:
        return myHotSpotPageAccess1.device->poke(address, value);
	break;
      case 0x0A00:
        return myHotSpotPageAccess2.device->poke(address, value);
	break;
      case 0x0B00:
        return myHotSpotPageAccess3.device->poke(address, value);
	break;
      case 0x0C00:
        return myHotSpotPageAccess4.device->poke(address, value);
	break;
      case 0x0D00:
        return myHotSpotPageAccess5.device->poke(address, value);
	break;
      case 0x0E00:
        return myHotSpotPageAccess6.device->poke(address, value);
	break;
      case 0x0F00:
        return myHotSpotPageAccess7.device->poke(address, value);
	break;
      default:
        break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeSB::bank(uInt16 bank)
{ 
  if(bankLocked) return;

  // Remember what bank we're in
  myCurrentBank = bank;
  uInt32 offset = myCurrentBank * 4096;
  uInt16 shift = mySystem->pageShift();
//  uInt16 mask = mySystem->pageMask();

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
int CartridgeSB::bank()
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartridgeSB::bankCount()
{
  return mySize>>12;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeSB::patch(uInt16 address, uInt8 value)
{
  address &= 0x0fff;
  myImage[myCurrentBank * 4096] = value;
  bank(myCurrentBank); // TODO: see if this is really necessary
  return true;
} 


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8* CartridgeSB::getImage(int& size)
{
  size = mySize;
  return &myImage[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeSB::save(Serializer& out) const
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
bool CartridgeSB::load(Deserializer& in)
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
