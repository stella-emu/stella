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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Cart4A50.cxx,v 1.7 2008-01-24 20:43:41 stephena Exp $
//============================================================================

#include <cassert>

#include "Random.hxx"
#include "System.hxx"
#include "Cart4A50.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge4A50::Cartridge4A50(const uInt8* image)
{
  // Copy the ROM image into my buffer
  for(uInt32 addr = 0; addr < 65536; ++addr)
  {
    myImage[addr] = image[addr];
  }

  // Initialize RAM with random values
  class Random random;
  for(uInt32 i = 0; i < 32768; ++i)
  {
    myRAM[i] = random.next();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge4A50::~Cartridge4A50()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge4A50::reset()
{
  mySliceLow = mySliceMiddle = mySliceHigh = 0;
  myIsRomLow = myIsRomMiddle = myIsRomHigh = true;

  myLastData    = 0xff;
  myLastAddress = 0xffff;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge4A50::install(System& system)
{
  mySystem = &system;
  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  // Make sure the system we're being installed in has a page size that'll work
  assert((0x1000 & mask) == 0);

  // Map all of the accesses to call peek and poke
  System::PageAccess access;
  for(uInt32 i = 0x1000; i < 0x2000; i += (1 << shift))
  {
    access.directPeekBase = 0;
    access.directPokeBase = 0;
    access.device = this;
    mySystem->setPageAccess(i >> shift, access);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Cartridge4A50::peek(uInt16 address)
{
  uInt8 value = 0;

  if(!(address & 0x1000))
  {
    // ReadHardware();
    bank(address);
  }
  else
  {
    if((address & 0x1800) == 0x1000)
    {
      value = myIsRomLow ?
        myImage[(address & 0x7ff) + mySliceLow] : 
        myRAM[(address & 0x7ff) + mySliceLow];
    }
    else if(((address & 0x1fff) >= 0x1800) && ((address & 0x1fff) <= 0x1dff))
    {
      value = myIsRomMiddle ?
        myImage[(address & 0x7ff) + mySliceMiddle] :
        myRAM[(address & 0x7ff) + mySliceMiddle];
    }
    else if((address & 0x1f00) == 0x1e00)
    {
      value = myIsRomHigh ?
        myImage[(address & 0xff) + mySliceHigh] :
        myRAM[(address & 0xff) + mySliceHigh];
    }
    else if((address & 0x1f00) == 0x1f00)
    {
      value = myImage[(address & 0xff) + 0xff00];
      if(((myLastData & 0xe0) == 0x60) &&
         ((myLastAddress >= 0x1000) || (myLastAddress < 0x200)))
        mySliceHigh = (mySliceHigh & 0xf0ff) | ((address & 0x8) << 8) |
                      ((address & 0x70) << 4);
    }
  }
  myLastData = value;
  myLastAddress = address & 0x1fff;

  return value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge4A50::poke(uInt16 address, uInt8 value)
{
  if(!(address & 0x1000))
  {
    // WriteHardware();
    bank(address);
  }
  else
  {
    if((address & 0x1800) == 0x1000)
    {
      if(!myIsRomLow)
        myRAM[(address & 0x7ff) + mySliceLow] = value;
    }
    else if(((address & 0x1fff) >= 0x1800) && ((address & 0x1fff) <= 0x1dff))
    {
      if(!myIsRomMiddle)
        myRAM[(address & 0x7ff) + mySliceMiddle] = value;
    }
    else if((address & 0x1f00) == 0x1e00)
    {
      if(!myIsRomHigh)
        myRAM[(address & 0xff) + mySliceHigh] = value;
    }
    else if((address & 0x1f00) == 0x1f00)
    {
      if(((myLastData & 0xe0) == 0x60) &&
         ((myLastAddress >= 0x1000) || (myLastAddress < 0x200)))
        mySliceHigh = (mySliceHigh & 0xf0ff) | ((address & 0x8) << 8) | ((address & 0x70) << 4);
    }
  }

  myLastData = value;
  myLastAddress = address & 0x1fff;
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge4A50::bank(uInt16 address)
{
  // Not bankswitching in the normal sense
  // This scheme contains so many hotspots that it's easier to just check
  // all of them
  if(((myLastData & 0xe0) == 0x60) &&
     ((myLastAddress >= 0x1000) || (myLastAddress < 0x200)))
  {
    if((address & 0x0f00) == 0x0c00)
    {
      myIsRomHigh = true;
      mySliceHigh = (address & 0xff) << 8;
    }
    else if((address & 0x0f00) == 0x0d00)
    {
      myIsRomHigh = false;
      mySliceHigh = (address & 0x7f) << 8;
    }
    else if((address & 0x0f40) == 0x0e00)
    {
      myIsRomLow = true;
      mySliceLow = (address & 0x1f) << 11;
    }
    else if((address & 0x0f40) == 0x0e40)
    {
      myIsRomLow = false;
      mySliceLow = (address & 0xf) << 11;
    }
    else if((address & 0x0f40) == 0x0f00)
    {
      myIsRomMiddle = true;
      mySliceMiddle = (address & 0x1f) << 11;
    }
    else if((address & 0x0f50) == 0x0f40)
    {
      myIsRomMiddle = false;
      mySliceMiddle = (address & 0xf) << 11;
    }
    else if((address & 0x0f00) == 0x0400)
    {
      mySliceLow = mySliceLow ^ 0x800;
    }
    else if((address & 0x0f00) == 0x0500)
    {
      mySliceLow = mySliceLow ^ 0x1000;
    }
    else if((address & 0x0f00) == 0x0800)
    {
      mySliceMiddle = mySliceMiddle ^ 0x800;
    }
    else if((address & 0x0f00) == 0x0900)
    {
      mySliceMiddle = mySliceMiddle ^ 0x1000;
    }
  }

  if((address & 0xf75) == 0x74)
  {
    myIsRomHigh = true;
    mySliceHigh = value << 8;
  }
  else if((address & 0xf75) == 0x75)
  {
    myIsRomHigh = false;
    mySliceHigh = (value & 0x7f) << 8;
  }
  else if((address & 0xf7c) == 0x78)
  {
    if((value & 0xf0) == 0)
    {
      myIsRomLow = true;
      mySliceLow = (value & 0xf) << 11;            
    }
    else if((value & 0xf0) == 0x40)
    {
      myIsRomLow = false;
      mySliceLow = (value & 0xf) << 11;            
    }
    else if((value & 0xf0) == 0x90)
    {
      myIsRomMiddle = true;
      mySliceMiddle = ((value & 0xf) | 0x10) << 11;                            
    }
    else if((value & 0xf0) == 0xc0)
    {
      myIsRomMiddle = false;
      mySliceMiddle = (value & 0xf) << 11;                         
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Cartridge4A50::bank()
{
  // Doesn't support bankswitching in the normal sense
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Cartridge4A50::bankCount()
{
  return 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge4A50::patch(uInt16 address, uInt8 value)
{
  return false;
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8* Cartridge4A50::getImage(int& size)
{
  size = 0;
  return 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge4A50::save(Serializer& out) const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge4A50::load(Deserializer& in)
{
  return true;
}
