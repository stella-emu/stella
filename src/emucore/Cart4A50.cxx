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
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <cassert>
#include <cstring>

#include "System.hxx"
#include "M6532.hxx"
#include "TIA.hxx"
#include "Cart4A50.hxx"

// TODO - properly handle read from write port functionality
//        Note: do r/w port restrictions even exist for this scheme??
//        Port to new CartDebug/disassembler scheme

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge4A50::Cartridge4A50(const uInt8* image, uInt32 size)
{
  // Copy the ROM image into my buffer
  // Supported file sizes are 32/64/128K, which are duplicated if necessary
  if(size < 65536)        size = 32768;
  else if(size < 131072)  size = 65536;
  else                    size = 131072;
  for(uInt32 slice = 0; slice < 131072 / size; ++slice)
    memcpy(myImage + (slice*size), image, size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge4A50::~Cartridge4A50()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge4A50::reset()
{
  // Initialize RAM with random values
  for(uInt32 i = 0; i < 32768; ++i)
    myRAM[i] = mySystem->randGenerator().next();

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

  // Mirror all access in TIA and RIOT; by doing so we're taking responsibility
  // for that address space in peek and poke below.
  mySystem->tia().install(system, *this);
  mySystem->m6532().install(system, *this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Cartridge4A50::peek(uInt16 address)
{
  uInt8 value = 0;

  if(!(address & 0x1000))                      // Hotspots below 0x1000
  {
    // Check for RAM or TIA mirroring
    uInt16 lowAddress = address & 0x3ff;
    if(lowAddress & 0x80)
      value = mySystem->m6532().peek(address);
    else if(!(lowAddress & 0x200))
      value = mySystem->tia().peek(address);

    checkBankSwitch(address, value);
  }
  else
  {
    if((address & 0x1800) == 0x1000)           // 2K region from 0x1000 - 0x17ff
    {
      value = myIsRomLow ? myImage[(address & 0x7ff) + mySliceLow]
                         : myRAM[(address & 0x7ff) + mySliceLow];
    }
    else if(((address & 0x1fff) >= 0x1800) &&  // 1.5K region from 0x1800 - 0x1dff
            ((address & 0x1fff) <= 0x1dff))
    {
      value = myIsRomMiddle ? myImage[(address & 0x7ff) + mySliceMiddle + 0x10000]
                            : myRAM[(address & 0x7ff) + mySliceMiddle];
    }
    else if((address & 0x1f00) == 0x1e00)      // 256B region from 0x1e00 - 0x1eff
    {
      value = myIsRomHigh ? myImage[(address & 0xff) + mySliceHigh + 0x10000]
                          : myRAM[(address & 0xff) + mySliceHigh];
    }
    else if((address & 0x1f00) == 0x1f00)      // 256B region from 0x1f00 - 0x1fff
    {
      value = myImage[(address & 0xff) + 0x1ff00];
      if(!myBankLocked && ((myLastData & 0xe0) == 0x60) &&
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
  if(!(address & 0x1000))                      // Hotspots below 0x1000
  {
    // Check for RAM or TIA mirroring
    uInt16 lowAddress = address & 0x3ff;
    if(lowAddress & 0x80)
      mySystem->m6532().poke(address, value);
    else if(!(lowAddress & 0x200))
      mySystem->tia().poke(address, value);

    checkBankSwitch(address, value);
  }
  else
  {
    if((address & 0x1800) == 0x1000)           // 2K region at 0x1000 - 0x17ff
    {
      if(!myIsRomLow)
        myRAM[(address & 0x7ff) + mySliceLow] = value;
    }
    else if(((address & 0x1fff) >= 0x1800) &&  // 1.5K region at 0x1800 - 0x1dff
            ((address & 0x1fff) <= 0x1dff))
    {
      if(!myIsRomMiddle)
        myRAM[(address & 0x7ff) + mySliceMiddle] = value;
    }
    else if((address & 0x1f00) == 0x1e00)      // 256B region at 0x1e00 - 0x1eff
    {
      if(!myIsRomHigh)
        myRAM[(address & 0xff) + mySliceHigh] = value;
    }
    else if((address & 0x1f00) == 0x1f00)      // 256B region at 0x1f00 - 0x1fff
    {
      if(!myBankLocked && ((myLastData & 0xe0) == 0x60) &&
         ((myLastAddress >= 0x1000) || (myLastAddress < 0x200)))
        mySliceHigh = (mySliceHigh & 0xf0ff) | ((address & 0x8) << 8) |
                      ((address & 0x70) << 4);
    }
  }
  myLastData = value;
  myLastAddress = address & 0x1fff;
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge4A50::checkBankSwitch(uInt16 address, uInt8 value)
{
  if(myBankLocked) return;

  // This scheme contains so many hotspots that it's easier to just check
  // all of them
  if(((myLastData & 0xe0) == 0x60) &&      // Switch lower/middle/upper bank
     ((myLastAddress >= 0x1000) || (myLastAddress < 0x200)))
  {
    if((address & 0x0f00) == 0x0c00)       // Enable 256B of ROM at 0x1e00 - 0x1eff
    {
      myIsRomHigh = true;
      mySliceHigh = (address & 0xff) << 8;
    }
    else if((address & 0x0f00) == 0x0d00)  // Enable 256B of RAM at 0x1e00 - 0x1eff
    {
      myIsRomHigh = false;
      mySliceHigh = (address & 0x7f) << 8;
    }
    else if((address & 0x0f40) == 0x0e00)  // Enable 2K of ROM at 0x1000 - 0x17ff
    {
      myIsRomLow = true;
      mySliceLow = (address & 0x1f) << 11;
    }
    else if((address & 0x0f40) == 0x0e40)  // Enable 2K of RAM at 0x1000 - 0x17ff
    {
      myIsRomLow = false;
      mySliceLow = (address & 0xf) << 11;
    }
    else if((address & 0x0f40) == 0x0f00)  // Enable 1.5K of ROM at 0x1800 - 0x1dff
    {
      myIsRomMiddle = true;
      mySliceMiddle = (address & 0x1f) << 11;
    }
    else if((address & 0x0f50) == 0x0f40)  // Enable 1.5K of RAM at 0x1800 - 0x1dff
    {
      myIsRomMiddle = false;
      mySliceMiddle = (address & 0xf) << 11;
    }

    // Stella helper functions
    else if((address & 0x0f00) == 0x0400)   // Toggle bit A11 of lower block address
    {
      mySliceLow = mySliceLow ^ 0x800;
    }
    else if((address & 0x0f00) == 0x0500)   // Toggle bit A12 of lower block address
    {
      mySliceLow = mySliceLow ^ 0x1000;
    }
    else if((address & 0x0f00) == 0x0800)   // Toggle bit A11 of middle block address
    {
      mySliceMiddle = mySliceMiddle ^ 0x800;
    }
    else if((address & 0x0f00) == 0x0900)   // Toggle bit A12 of middle block address
    {
      mySliceMiddle = mySliceMiddle ^ 0x1000;
    }
  }

  // Zero-page hotspots for upper page
  //   0xf4, 0xf6, 0xfc, 0xfe for ROM
  //   0xf5, 0xf7, 0xfd, 0xff for RAM
  //   0x74 - 0x7f (0x80 bytes lower)
  if((address & 0xf75) == 0x74)         // Enable 256B of ROM at 0x1e00 - 0x1eff
  {
    myIsRomHigh = true;
    mySliceHigh = value << 8;
  }
  else if((address & 0xf75) == 0x75)    // Enable 256B of RAM at 0x1e00 - 0x1eff
  {
    myIsRomHigh = false;
    mySliceHigh = (value & 0x7f) << 8;
  }

  // Zero-page hotspots for lower and middle blocks
  //   0xf8, 0xf9, 0xfa, 0xfb
  //   0x78, 0x79, 0x7a, 0x7b (0x80 bytes lower)
  else if((address & 0xf7c) == 0x78)
  {
    if((value & 0xf0) == 0)           // Enable 2K of ROM at 0x1000 - 0x17ff
    {
      myIsRomLow = true;
      mySliceLow = (value & 0xf) << 11;
    }
    else if((value & 0xf0) == 0x40)   // Enable 2K of RAM at 0x1000 - 0x17ff
    {
      myIsRomLow = false;
      mySliceLow = (value & 0xf) << 11;
    }
    else if((value & 0xf0) == 0x90)   // Enable 1.5K of ROM at 0x1800 - 0x1dff
    {
      myIsRomMiddle = true;
      mySliceMiddle = ((value & 0xf) | 0x10) << 11;
    }
    else if((value & 0xf0) == 0xc0)   // Enable 1.5K of RAM at 0x1800 - 0x1dff
    {
      myIsRomMiddle = false;
      mySliceMiddle = (value & 0xf) << 11;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge4A50::bank(uInt16)
{
  // Doesn't support bankswitching in the normal sense
  // TODO - add support for debugger
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Cartridge4A50::bank()
{
  // Doesn't support bankswitching in the normal sense
  // TODO - add support for debugger
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Cartridge4A50::bankCount()
{
  // Doesn't support bankswitching in the normal sense
  // TODO - add support for debugger
  return 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge4A50::patch(uInt16 address, uInt8 value)
{
  // Doesn't support bankswitching in the normal sense
  // TODO - add support for debugger
  return false;
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8* Cartridge4A50::getImage(int& size)
{
  size = 131072;
  return &myImage[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge4A50::save(Serializer& out) const
{
  const string& cart = name();

  try
  {
    out.putString(cart);

    // The 32K bytes of RAM
    out.putInt(32768);
    for(uInt32 i = 0; i < 32768; ++i)
      out.putByte((char)myRAM[i]);

    // Index pointers
    out.putInt(mySliceLow);
    out.putInt(mySliceMiddle);
    out.putInt(mySliceHigh);

    // Whether index pointers are for ROM or RAM
    out.putBool(myIsRomLow);
    out.putBool(myIsRomMiddle);
    out.putBool(myIsRomHigh);

    // Last address and data values
    out.putByte(myLastData);
    out.putInt(myLastAddress);
  }
  catch(const char* msg)
  {
    cerr << "ERROR: Cartridge4A40::save" << endl << "  " << msg << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge4A50::load(Serializer& in)
{
  const string& cart = name();

  try
  {
    if(in.getString() != cart)
      return false;

    uInt32 limit = (uInt32) in.getInt();
    for(uInt32 i = 0; i < limit; ++i)
      myRAM[i] = (uInt8) in.getByte();

    // Index pointers
    mySliceLow = (uInt16) in.getInt();
    mySliceMiddle = (uInt16) in.getInt();
    mySliceHigh = (uInt16) in.getInt();

    // Whether index pointers are for ROM or RAM
    myIsRomLow = in.getBool();
    myIsRomMiddle = in.getBool();
    myIsRomHigh = in.getBool();

    // Last address and data values
    myLastData = (uInt8) in.getByte();
    myLastAddress = (uInt16) in.getInt();
  }
  catch(const char* msg)
  {
    cerr << "ERROR: Cartridge4A50::load" << endl << "  " << msg << endl;
    return false;
  }

  return true;
}
