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
// Copyright (c) 1995-1999 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: CartAR.cxx,v 1.1.1.1 2001-12-27 19:54:19 bwmott Exp $
//============================================================================

#include <assert.h>
#include <string.h>
#include "CartAR.hxx"
#include "M6502Hi.hxx"
#include "Random.hxx"
#include "System.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeAR::CartridgeAR(const uInt8* image, uInt32 size)
    : my6502(0)
{
  uInt32 i;

  // Create a load image buffer and copy the given image
  myLoadImages = new uInt8[size];
  myNumberOfLoadImages = size / 8448;
  memcpy(myLoadImages, image, size);

  // Initialize RAM with random values
  Random random;
  for(i = 0; i < 6 * 1024; ++i)
  {
    myImage[i] = random.next();
  }

  // Initialize ROM with an invalid 6502 opcode 
  for(i = 6 * 1024; i < 8 * 1024; ++i)
  {
    myImage[i] = 0xFF; 
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeAR::~CartridgeAR()
{
  delete[] myLoadImages;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* CartridgeAR::name() const
{
  return "CartridgeAR";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeAR::reset()
{
  // Try to load the first load into RAM upon reset
  loadIntoRAM(0);

  myPower = true;
  myPowerRomCycle = 0;
  myWriteEnabled = false;

  myLastAccess = 0;
  myNumberOfDistinctAccesses = 0;
  myWritePending = false;

  // Set bank configuration upon reset so ROM is selected
  myImageOffset[0] = 0 * 2048;
  myImageOffset[1] = 3 * 2048;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeAR::systemCyclesReset()
{
  // Get the current system cycle
  uInt32 cycles = mySystem->cycles();

  // Adjust cycle values
  myPowerRomCycle -= cycles;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeAR::install(System& system)
{
  mySystem = &system;
  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  my6502 = &(M6502High&)mySystem->m6502();

  // Make sure the system we're being installed in has a page size that'll work
  assert((0x1000 & mask) == 0);

  System::PageAccess access;
  for(uInt32 i = 0x1000; i < 0x2000; i += (1 << shift))
  {
    access.directPeekBase = 0;
    access.directPokeBase = 0;
    access.device = this;
    mySystem->setPageAccess(i >> shift, access);
  }

  bankConfiguration(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeAR::peek(uInt16 addr)
{
  // Check to see if the Supercharger ROM is being accessed?
  if(myImageOffset[1] == 3 * 2048)
  {
    Int32 cycles = mySystem->cycles();

    // Is the tape rewind routine being accessed?
    if((addr & 0x1FFF) == 0x180A)
    {
      // See if the ROM has been powered up long enough
      if(!myPower || (myPower && ((myPowerRomCycle + 7) > cycles)))
      {
        cerr << "ERROR: Supercharger ROM has not been powered up!\n";
      }
      else
      {
        cerr << "ERROR: Supercharger code doesn't handle rewinding tape!\n";
      }
    }
    // Is the multiload routine being accessed?
    else if((addr & 0x1FFF) == 0x1800)
    {
      // See if the ROM has been powered up long enough
      if(!myPower || (myPower && ((myPowerRomCycle + 7) > cycles)))
      {
        cerr << "ERROR: Supercharger ROM has not been powered up!\n";
      }
      else
      {
        // Get the load they're trying to access
        uInt8 load = mySystem->peek(0x00FA);

        // Load the specified load into RAM
        loadIntoRAM(load);

        return myImage[(addr & 0x07FF) + myImageOffset[1]];
      }
    }
  }

  // Are the "value" registers being accessed?
  if(!(addr & 0x0F00) && (!myWriteEnabled || !myWritePending))
  {
    myLastAccess = addr;
    myNumberOfDistinctAccesses = my6502->distinctAccesses();
    myWritePending = true;
  }
  // Is the bank configuration hotspot being accessed?
  else if((addr & 0x1FFF) == 0x1FF8)
  {
    // Yes, so handle bank configuration
    myWritePending = false;
    bankConfiguration(myLastAccess);
  }
  // Handle poke if writing enabled
  else if(myWriteEnabled && myWritePending)
  {
    if(my6502->distinctAccesses() >= myNumberOfDistinctAccesses + 5)
    {
      if(my6502->distinctAccesses() == myNumberOfDistinctAccesses + 5)
      {
        if((addr & 0x0800) == 0)
          myImage[(addr & 0x07FF) + myImageOffset[0]] = myLastAccess;
        else if(myImageOffset[1] != 3 * 2048)    // Can't poke to ROM :-)
          myImage[(addr & 0x07FF) + myImageOffset[1]] = myLastAccess;
      }
      myWritePending = false;
    } 
  }

  return myImage[(addr & 0x07FF) + myImageOffset[(addr & 0x0800) ? 1 : 0]];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeAR::poke(uInt16 addr, uInt8)
{
  // Are the "value" registers being accessed?
  if(!(addr & 0x0F00) && (!myWriteEnabled || !myWritePending))
  {
    myLastAccess = addr;
    myNumberOfDistinctAccesses = my6502->distinctAccesses();
    myWritePending = true;
  }
  // Is the bank configuration hotspot being accessed?
  else if((addr & 0x1FFF) == 0x1FF8)
  {
    // Yes, so handle bank configuration
    myWritePending = false;
    bankConfiguration(myLastAccess);
  }
  // Handle poke if writing enabled
  else if(myWriteEnabled && myWritePending)
  {
    if(my6502->distinctAccesses() >= myNumberOfDistinctAccesses + 5)
    {
      if(my6502->distinctAccesses() == myNumberOfDistinctAccesses + 5)
      {
        if((addr & 0x0800) == 0)
          myImage[(addr & 0x07FF) + myImageOffset[0]] = myLastAccess;
        else if(myImageOffset[1] != 3 * 2048)    // Can't poke to ROM :-)
          myImage[(addr & 0x07FF) + myImageOffset[1]] = myLastAccess;
      }
      myWritePending = false;
    } 
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeAR::bankConfiguration(uInt8 configuration)
{
  // D7-D5 of this byte: Write Pulse Delay (n/a for emulator)
  //
  // D4-D0: RAM/ROM configuration:
  //       $F000-F7FF    $F800-FFFF Address range that banks map into
  //  000wp     2            ROM
  //  001wp     0            ROM
  //  010wp     2            0      as used in Commie Mutants and many others
  //  011wp     0            2      as used in Suicide Mission
  //  100wp     2            ROM
  //  101wp     1            ROM
  //  110wp     2            1      as used in Killer Satellites
  //  111wp     1            2      as we use for 2k/4k ROM cloning
  // 
  //  w = Write Enable (1 = enabled; accesses to $F000-$F0FF cause writes
  //    to happen.  0 = disabled, and the cart acts like ROM.)
  //  p = ROM Power (0 = enabled, 1 = off.)  Only power the ROM if you're
  //    wanting to access the ROM for multiloads.  Otherwise set to 1.

  // Handle ROM power configuration
  myPower = !(configuration & 0x01);

  if(myPower)
  {
    myPowerRomCycle = mySystem->cycles();
  }

  myWriteEnabled = configuration & 0x02;

  switch((configuration >> 2) & 0x07)
  {
    case 0:
    {
      myImageOffset[0] = 2 * 2048;
      myImageOffset[1] = 3 * 2048;
      break;
    }

    case 1:
    {
      myImageOffset[0] = 0 * 2048;
      myImageOffset[1] = 3 * 2048;
      break;
    }

    case 2:
    {
      myImageOffset[0] = 2 * 2048;
      myImageOffset[1] = 0 * 2048;
      break;
    }

    case 3:
    {
      myImageOffset[0] = 0 * 2048;
      myImageOffset[1] = 2 * 2048;
      break;
    }

    case 4:
    {
      myImageOffset[0] = 2 * 2048;
      myImageOffset[1] = 3 * 2048;
      break;
    }

    case 5:
    {
      myImageOffset[0] = 1 * 2048;
      myImageOffset[1] = 3 * 2048;
      break;
    }

    case 6:
    {
      myImageOffset[0] = 2 * 2048;
      myImageOffset[1] = 1 * 2048;
      break;
    }

    case 7:
    {
      myImageOffset[0] = 1 * 2048;
      myImageOffset[1] = 2 * 2048;
      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeAR::setupROM()
{
  static uInt8 dummyROMCode[] = {
    0xa9, 0x0, 0xa2, 0x0, 0x95, 0x80, 0xe8, 0xe0, 
    0x80, 0xd0, 0xf9, 0x4c, 0x2b, 0xfa, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xa9, 0x0, 0xa2, 0x0, 0x95, 0x80, 0xe8, 0xe0, 
    0x1e, 0xd0, 0xf9, 0xa2, 0x0, 0xbd, 0x45, 0xfa, 
    0x95, 0xfa, 0xe8, 0xe0, 0x6, 0xd0, 0xf6, 0xa2, 
    0xff, 0xa0, 0x0, 0xa9, 0x0, 0x85, 0x80, 0xcd, 
    0x0, 0xf0, 0x4c, 0xfa, 0x0, 0xad, 0xf8, 0xff, 
    0x4c, 0x0, 0x0
  };

  int size = sizeof(dummyROMCode);

  // Copy the "dummy" ROM code into the ROM area
  for(int i = 0; i < size; ++i)
  {
    myImage[0x1A00 + i] = dummyROMCode[i];
  }

  // Put a JMP $FA20 at multiload entry point ($F800)
  myImage[0x1800] = 0x4C;
  myImage[0x1801] = 0x20;
  myImage[0x1802] = 0xFA;

  // Update ROM code to have the correct reset address and bank configuration
  myImage[0x1A00 + size - 2] = myHeader[0];
  myImage[0x1A00 + size - 1] = myHeader[1];
  myImage[0x1A00 + size - 11] = myHeader[2];
  myImage[0x1A00 + size - 15] = myHeader[2];

  // Finally set 6502 vectors to point to this "dummy" code at 0xFA00
  myImage[3 * 2048 + 2044] = 0x00;
  myImage[3 * 2048 + 2045] = 0xFA;
  myImage[3 * 2048 + 2046] = 0x00;
  myImage[3 * 2048 + 2047] = 0xFA;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeAR::checksum(uInt8* s, uInt16 length)
{
  uInt8 sum = 0;

  for(uInt32 i = 0; i < length; ++i)
  {
    sum += s[i];
  }

  return sum;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeAR::loadIntoRAM(uInt8 load)
{
  uInt16 image;

  // Scan through all of the loads to see if we find the one we're looking for
  for(image = 0; image < myNumberOfLoadImages; ++image)
  {
    // Is this the correct load?
    if(myLoadImages[(image * 8448) + 8192 + 5] == load)
    {
      // Copy the load's header
      memcpy(myHeader, myLoadImages + (image * 8448) + 8192, 256);

      // Verify the load's header 
      if(checksum(myHeader, 8) != 0x55)
      {
        cerr << "WARNING: The Supercharger header checksum is invalid...\n";
      }

      // Load all of the pages from the load
      bool invalidPageChecksumSeen = false;
      for(uInt32 j = 0; j < myHeader[3]; ++j)
      {
        uInt32 bank = myHeader[16 + j] & 0x03;
        uInt32 page = (myHeader[16 + j] >> 2) & 0x07;
        uInt8* src = myLoadImages + (image * 8448) + (j * 256);
        uInt8 sum = checksum(src, 256) + myHeader[16 + j] + myHeader[64 + j];

        if(!invalidPageChecksumSeen && (sum != 0x55))
        {
          cerr << "WARNING: Some Supercharger page checksums are invalid...\n";
          invalidPageChecksumSeen = true;
        }

        // Copy page to Supercharger RAM
        memcpy(myImage + (bank * 2048) + (page * 256), src, 256);
      }

      // Make sure the "dummy" ROM is installed
      setupROM();
      return;
    }
  }

  // TODO: Should probably switch to an internal ROM routine to display
  // this message to the user...
  cerr << "ERROR: Supercharger load is missing from ROM image...\n";
}

