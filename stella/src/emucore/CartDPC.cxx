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
// Copyright (c) 1995-2002 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: CartDPC.cxx,v 1.3 2002-03-28 01:48:28 bwmott Exp $
//============================================================================

#include <assert.h>
#include "CartDPC.hxx"
#include "System.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// This class does not emulate the music mode data fetchers of the DPC.  A 
// draft version of this class which does support the music mode data fetchers
// has been developed, however, it hasn't been checked in because additional
// work to the Stella sound system is needed for the sound to work correctly.
// Full support for the DPC will be added in the 1.3 release of Stella.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDPC::CartridgeDPC(const uInt8* image, uInt32 size)
{
  uInt32 addr;

  // Copy the program ROM image into my buffer
  for(addr = 0; addr < 8192; ++addr)
  {
    myProgramImage[addr] = image[addr];
  }

  // Copy the display ROM image into my buffer
  for(addr = 0; addr < 2048; ++addr)
  {
    myDisplayImage[addr] = image[8192 + addr];
  }

  // Initialize the DPC data fetcher registers
  for(uInt16 i = 0; i < 8; ++i)
  {
    myBottoms[i] = myCounters[i] = myFlags[i] = myTops[i] = 0;
  }

  // Initialize the DPC's random number generator register (must be non-zero)
  myRandomNumber = 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDPC::~CartridgeDPC()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* CartridgeDPC::name() const
{
  return "CartridgeDPC";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPC::reset()
{
  // Upon reset we switch to bank 1
  bank(1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPC::install(System& system)
{
  mySystem = &system;
  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  // Make sure the system we're being installed in has a page size that'll work
  assert(((0x1080 & mask) == 0) && ((0x1100 & mask) == 0));

  // Set the page accessing methods for the hot spots
  System::PageAccess access;
  for(uInt32 i = (0x1FF8 & ~mask); i < 0x2000; i += (1 << shift))
  {
    access.directPeekBase = 0;
    access.directPokeBase = 0;
    access.device = this;
    mySystem->setPageAccess(i >> shift, access);
  }

  // Set the page accessing method for the DPC reading & writing pages
  for(uInt32 j = 0x1000; j < 0x1080; j += (1 << shift))
  {
    access.directPeekBase = 0;
    access.directPokeBase = 0;
    access.device = this;
    mySystem->setPageAccess(j >> shift, access);
  }

  // Install pages for bank 1
  bank(1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void CartridgeDPC::clockRandomNumberGenerator()
{
  // Table for computing the input bit of the random number generator's
  // shift register (it's the NOT of the EOR of four bits)
  static uInt8 f[16] = {
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
  };

  // Using bits 7, 5, 4, & 3 of the shift register compute the input
  // bit for the shift register
  uInt8 bit = f[((myRandomNumber >> 3) & 0x07) | 
      ((myRandomNumber & 0x80) ? 0x08 : 0x00)];

  // Update the shift register 
  myRandomNumber = (myRandomNumber << 1) | bit;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeDPC::peek(uInt16 address)
{
  address = address & 0x0FFF;

  // Clock the random number generator.  This should be done for every
  // cartridge access, however, we're only doing it for the DPC and 
  // hot-spot accesses to save time.
  clockRandomNumberGenerator();

  if(address < 0x0040)
  {
    // Get the index of the data fetcher that's being accessed
    uInt32 index = address & 0x07;
    uInt32 function = (address >> 3) & 0x07;

    // Update flag register for this data fetcher
    if((myCounters[index] & 0x00ff) == myTops[index])
    {
      myFlags[index] = 0xff;
    }
    else if((myCounters[index] & 0x00ff) == myBottoms[index])
    {
      myFlags[index] = 0x00;
    }

    switch(function)
    {
      case 0x00:
      {
        // Is this a random number read
        if(index < 4)
        {
          return myRandomNumber;
        }
        else
        {
          // TODO: Replace with sound read code
          return 0;
        }
      }

      // DFx display data read
      case 0x01:
      {
        uInt8 r = myDisplayImage[2047 - myCounters[index]];
        myCounters[index] = (myCounters[index] - 1) & 0x07ff;
        return r;
      }

      // DFx display data read AND'd w/flag
      case 0x02:
      {
        uInt8 r = myDisplayImage[2047 - myCounters[index]] & myFlags[index];
        myCounters[index] = (myCounters[index] - 1) & 0x07ff;
        return r;
      } 

      // DFx flag
      case 0x07:
      {
        uInt8 r = myFlags[index];
        myCounters[index] = (myCounters[index] - 1) & 0x07ff;
        return r;
      }

      default:
      {
        return 0;
      }
    }
  }
  else
  {
    // Switch banks if necessary
    switch(address)
    {
      case 0x0FF8:
        // Set the current bank to the lower 4k bank
        bank(0);
        break;

      case 0x0FF9:
        // Set the current bank to the upper 4k bank
        bank(1);
        break;

      default:
        break;
    }
    return myProgramImage[myCurrentBank * 4096 + address];
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPC::poke(uInt16 address, uInt8 value)
{
  address = address & 0x0FFF;

  // Clock the random number generator.  This should be done for every
  // cartridge access, however, we're only doing it for the DPC and 
  // hot-spot accesses to save time.
  clockRandomNumberGenerator();

  if((address >= 0x0040) && (address < 0x0080))
  {
    // Get the index of the data fetcher that's being accessed
    uInt32 index = address & 0x07;    
    uInt32 function = (address >> 3) & 0x07;

    switch(function)
    {
      // DFx top count
      case 0x00:
      {
        myTops[index] = value;
        break;
      }

      // DFx bottom count
      case 0x01:
      {
        myBottoms[index] = value;
        break;
      }

      // DFx counter low
      case 0x02:
      {
        myCounters[index] = (myCounters[index] & 0x0700) | (uInt16)value;
        break;
      }

      // DFx counter high
      case 0x03:
      {
        myFlags[index] = 0x00;
        myCounters[index] = (((uInt16)value & 0x07) << 8) |
            (myCounters[index] & 0x00ff);
        // TODO: Handle music data fetchers
        break;
      }

      // Random Number Generator Reset
      case 0x06:
      {
        myRandomNumber = 1;
        break;
      }

      default:
      {
        break;
      }
    } 
  }
  else
  {
    // Switch banks if necessary
    switch(address)
    {
      case 0x0FF8:
        // Set the current bank to the lower 4k bank
        bank(0);
        break;

      case 0x0FF9:
        // Set the current bank to the upper 4k bank
        bank(1);
        break;

      default:
        break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPC::bank(uInt16 bank)
{ 
  // Remember what bank we're in
  myCurrentBank = bank;
  uInt16 offset = myCurrentBank * 4096;
  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  // Setup the page access methods for the current bank
  System::PageAccess access;
  access.device = this;
  access.directPokeBase = 0;

  // Map Program ROM image into the system
  for(uInt32 address = 0x1080; address < (0x1FF8U & ~mask);
      address += (1 << shift))
  {
    access.directPeekBase = &myProgramImage[offset + (address & 0x0FFF)];
    mySystem->setPageAccess(address >> shift, access);
  }
}

