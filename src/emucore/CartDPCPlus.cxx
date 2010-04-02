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
#include "CartDPCPlus.hxx"

// TODO - INC AUDV0+$40 music support

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDPCPlus::CartridgeDPCPlus(const uInt8* image, uInt32 size)
  : myFastFetch(false),
    mySystemCycles(0),
    myFractionalClocks(0.0)
{
  // Make a copy of the entire image as-is, for use by getImage()
  // (this wastes 29K of RAM, should be controlled by a #ifdef)
  memcpy(myImageCopy, image, size);

  // Copy the program ROM image into my buffer
  memcpy(myProgramImage, image, 4096 * 6);

  // Copy the display ROM image into my buffer
  memcpy(myDisplayImage, image + 4096 * 6, 4096);
  
  // Copy the Frequency ROM image into my buffer
  memcpy(myFrequencyImage, image + 4096 * 6 + 4096, 1024);

  // Initialize the DPC data fetcher registers
  for(uInt16 i = 0; i < 8; ++i)
    myTops[i] = myBottoms[i] = myCounters[i] = myFlags[i] = myFractionalIncrements[i] = 0;

  // None of the data fetchers are in music mode
  myMusicMode[0] = myMusicMode[1] = myMusicMode[2] = false;

  // Initialize the DPC's random number generator register (must be non-zero)
  myRandomNumber = 0x2B435044; // "DPC+"

  // Remember startup bank
  myStartBank = 5;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDPCPlus::~CartridgeDPCPlus()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPCPlus::reset()
{
  // Update cycles to the current system cycles
  mySystemCycles = mySystem->cycles();
  myFractionalClocks = 0.0;

  // Upon reset we switch to the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPCPlus::systemCyclesReset()
{
  // Get the current system cycle
  uInt32 cycles = mySystem->cycles();

  // Adjust the cycle counter so that it reflects the new value
  mySystemCycles -= cycles;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPCPlus::install(System& system)
{
  mySystem = &system;
  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();
  
  // Make sure the system we're being installed in has a page size that'll work
  assert(((0x1080 & mask) == 0) && ((0x1100 & mask) == 0));
  
  // Map all of the accesses to call peek and poke
  System::PageAccess access;
  access.directPeekBase = 0;
  access.directPokeBase = 0;
  access.device = this;
  for(uInt32 i = 0x1000; i < 0x2000; i += (1 << shift))
    mySystem->setPageAccess(i >> shift, access);
  
  // Install pages for the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void CartridgeDPCPlus::clockRandomNumberGenerator()
{
  // Update random number generator (32-bit LFSR)
  myRandomNumber = ((myRandomNumber & 1) ? 0xa260012b: 0x00) ^ ((myRandomNumber >> 1) & 0x7FFFFFFF);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void CartridgeDPCPlus::priorClockRandomNumberGenerator()
{
  // Update random number generator (32-bit LFSR, reversed)
  myRandomNumber = ((myRandomNumber & (1<<31)) ? 0x44c00257: 0x00) ^ (myRandomNumber << 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void CartridgeDPCPlus::updateMusicModeDataFetchers()
{
  // Calculate the number of cycles since the last update
  Int32 cycles = mySystem->cycles() - mySystemCycles;
  mySystemCycles = mySystem->cycles();

  // Calculate the number of DPC OSC clocks since the last update
  double clocks = ((20000.0 * cycles) / 1193191.66666667) + myFractionalClocks;
  Int32 wholeClocks = (Int32)clocks;
  myFractionalClocks = clocks - (double)wholeClocks;

  if(wholeClocks <= 0)
  {
    return;
  }

  // Let's update counters and flags of the music mode data fetchers
  for(int x = 5; x <= 7; ++x)
  {
    // Update only if the data fetcher is in music mode
    if(myMusicMode[x - 5])
    {
      myMusicCounter[x - 5] += myMusicFrequency[x - 5];
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeDPCPlus::peek(uInt16 address)
{
  address &= 0x0FFF;

  uInt8 peekvalue = myProgramImage[(myCurrentBank << 12) + address];
  
  // In debugger/bank-locked mode, we ignore all hotspots and in general
  // anything that can change the internal state of the cart
  if(bankLocked())
    return peekvalue;

  if(myFastFetch && myLDAimmediate)
  {
    peekvalue = myProgramImage[(myCurrentBank << 12) + address];
    if(peekvalue < 0x0028)
      address = peekvalue;
  }
  myLDAimmediate = false;
  
  if(address < 0x0028)
  {
    uInt8 result = 0;
    
    // Get the index of the data fetcher that's being accessed
    uInt32 index = address & 0x07;
    uInt32 function = (address >> 3) & 0x07;
    
    // Update flag register for selected data fetcher
    if((myCounters[index] & 0x00ff) == ((myTops[index]+1) & 0xff))
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
        switch(index)
        {
          case 0x00:  // advance and return byte 0 of random
            clockRandomNumberGenerator();
            result = myRandomNumber & 0xFF;
            break;
            
          case 0x01:  // return to prior and return byte 0 of random
            priorClockRandomNumberGenerator();
            result = myRandomNumber & 0xFF;
            break;
            
          case 0x02:
            result = (myRandomNumber>>8) & 0xFF;
            break;
            
          case 0x03:
            result = (myRandomNumber>>16) & 0xFF;
            break;
            
          case 0x04:
            result = (myRandomNumber>>24) & 0xFF;
            break;
            
          case 0x05: // No, it's a music read
          {
            static const uInt8 musicAmplitudes[8] = {
              0x00, 0x04, 0x05, 0x09, 0x06, 0x0a, 0x0b, 0x0f
            };
            
            // Update the music data fetchers (counter & flag)
            updateMusicModeDataFetchers();
            
            uInt8 i = 0;
            if(myMusicMode[0] && (myMusicCounter[0]>>31))
            {
              i |= 0x01;
            }
            if(myMusicMode[1] && (myMusicCounter[1]>>31))
            {
              i |= 0x02;
            }
            if(myMusicMode[2] && (myMusicCounter[2]>>31))
            {
              i |= 0x04;
            }
            
            result = musicAmplitudes[i];
            break;          
          }

          case 0x06:  // reserved
          case 0x07:  // reserved
            break;
        }
        break;
      }
        
      // DFx display data read
      case 0x01:
      {
        result = myDisplayImage[myCounters[index]];
        myCounters[index] = (myCounters[index] + 0x1) & 0x0fff;   
        break;
      }
        
      // DFx display data read AND'd w/flag
      case 0x02:
      {
        result = myDisplayImage[myCounters[index]] & myFlags[index];
        myCounters[index] = (myCounters[index] + 0x1) & 0x0fff;   
        break;
      } 
        
      // DFx display data read w/fractional increment
      case 0x03:
      {
        result = myDisplayImage[myFractionalCounters[index] >> 8];
        myFractionalCounters[index] = (myFractionalCounters[index] + myFractionalIncrements[index]) & 0x0fffff;   
        break;
      }     

      case 0x04:
      {
        switch (index)
        {
          case 0x00:
          case 0x01:
          case 0x02:
          case 0x03:
          {
            result = myFlags[index];
            break;
          }
          case 0x04:  // reserved
          case 0x05:  // reserved
          case 0x06:  // reserved
          case 0x07:  // reserved
            break;
        }
        break;        
      }
        
      default:
      {
        result = 0;
      }
    }
    
    return result;
  }
  else
  {
    // Switch banks if necessary
    switch(address)
    {
      case 0x0FF6:
        // Set the current bank to the first 4k bank
        bank(0);
        break;
        
      case 0x0FF7:
        // Set the current bank to the second 4k bank
        bank(1);
        break;
        
      case 0x0FF8:
        // Set the current bank to the third 4k bank
        bank(2);
        break;
        
      case 0x0FF9:
        // Set the current bank to the fourth 4k bank
        bank(3);
        break;
        
      case 0x0FFA:
        // Set the current bank to the fifth 4k bank
        bank(4);
        break;
        
      case 0x0FFB:
        // Set the current bank to the last 4k bank
        bank(5);
        break;
        
      default:
        break;
    }
    
    peekvalue = myProgramImage[(myCurrentBank << 12) + address];
    if(myFastFetch)
      myLDAimmediate = (peekvalue == 0xA9);

    return peekvalue;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDPCPlus::poke(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;
  
  if((address >= 0x0028) && (address < 0x0080))  
  {
    // Get the index of the data fetcher that's being accessed
    uInt32 index = address & 0x07;    
    uInt32 function = ((address - 0x28) >> 3) & 0x0f;
    
    switch(function)
    {
      //DFxFrac counter low
      case 0x00:
      {
        myFractionalCounters[index] = (myFractionalCounters[index] & 0x0F0000) | ((uInt16)value << 8);
        break;
      }
        
      // DFxFrac counter high
      case 0x01:
      {
        myFractionalCounters[index] = (((uInt16)value & 0x0F) << 16) | (myFractionalCounters[index] & 0x00ffff);
        break;
      }
        
      case 0x02:
      {
        myFractionalIncrements[index] = value;
        myFractionalCounters[index] = myFractionalCounters[index] & 0x0FFF00;
        break;
      }

      // DFx top count
      case 0x03:
      {
        myTops[index] = value;
        myFlags[index] = 0x00;
        break;
      }
        
      // DFx bottom count
      case 0x04:
      {
        myBottoms[index] = value;
        break;
      }
        
      // DFx counter low
      case 0x05:
      {
        myCounters[index] = (myCounters[index] & 0x0F00) | value ;
        break;
      }
        
      // Control registers
      case 0x06:
      {
        switch (index)
        {
            // FastFetch control
          case 0x00:
          {
            myFastFetch = (value == 0);
            break;
          }
          case 0x01:  // reserved
          case 0x02:  // reserved
          case 0x03:  // reserved
          case 0x04:  // reserved
            break;
          case 0x05:
          case 0x06:
          case 0x07:
          {
            myMusicMode[index - 5] = (value & 0x10);
            break;
          }
          break;
        }
        break;
      }

      // DF Push
      case 0x07:
      {
        myCounters[index] = (myCounters[index] - 0x1) & 0x0fff;
        myDisplayImage[myCounters[index]] = value;
        break;
      }
        
      // DFx counter hi, same as counter high, but w/out music mode
      case 0x08:
      {
        myCounters[index] = (((uInt16)value & 0x0F) << 8) | (myCounters[index] & 0x00ff);
        break;
      }     
        
      // Random Number Generator Reset
      case 0x09:
      {
        switch (index)
        {
          case 0x00:
          {
            myRandomNumber = 0x2B435044; // "DPC+"
            break;
          }
          case 0x01:
          {
            myRandomNumber = (myRandomNumber & 0xFFFFFF00) | value;
            break;
          }
          case 0x02:
          {
            myRandomNumber = (myRandomNumber & 0xFFFF00FF) | (value<<8);
            break;
          }
          case 0x03:
          {
            myRandomNumber = (myRandomNumber & 0xFF00FFFF) | (value<<16);
            break;
          }
          case 0x04:
          {
            myRandomNumber = (myRandomNumber & 0x00FFFFFF) | (value<<24);
            break;
          }
          case 0x05:
          case 0x06:
          case 0x07: 
          { 
            myMusicFrequency[index-5] = myFrequencyImage[(value<<2)] +
            (myFrequencyImage[(value<<2)+1]<<8) +
            (myFrequencyImage[(value<<2)+2]<<16) +
            (myFrequencyImage[(value<<2)+3]<<24);
            break;
          }
          default:
            break;
        }
        break;
      }
        
      // DF Write
      case 0x0a:
      {
        myDisplayImage[myCounters[index]] = value;
        myCounters[index] = (myCounters[index] + 0x1) & 0x0fff;
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
      case 0x0FF6:
        // Set the current bank to the first 4k bank
        bank(0);
        break;
        
      case 0x0FF7:
        // Set the current bank to the second 4k bank
        bank(1);
        break;
        
      case 0x0FF8:
        // Set the current bank to the third 4k bank
        bank(2);
        break;
        
      case 0x0FF9:
        // Set the current bank to the fourth 4k bank
        bank(3);
        break;
        
      case 0x0FFA:
        // Set the current bank to the fifth 4k bank
        bank(4);
        break;
        
      case 0x0FFB:
        // Set the current bank to the last 4k bank
        bank(5);
        break;
        
      default:
        break;
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPCPlus::bank(uInt16 bank)
{ 
  if(bankLocked()) return;

  // Remember what bank we're in
  myCurrentBank = bank;

  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartridgeDPCPlus::bank()
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartridgeDPCPlus::bankCount()
{
  return 6;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDPCPlus::patch(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

  // For now, we ignore attempts to patch the DPC address space
  if(address >= 0x0080)
  {
    myProgramImage[(myCurrentBank << 12) + (address & 0x0FFF)] = value;
    return myBankChanged = true;
  }
  else
    return false;
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8* CartridgeDPCPlus::getImage(int& size)
{
  size = 4096 * 6 + 4096 + 255;
  return myImageCopy;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDPCPlus::save(Serializer& out) const
{
  const string& cart = name();

  try
  {
    uInt32 i;

    out.putString(cart);

    // Indicates which bank is currently active
    out.putInt(myCurrentBank);

    // The top registers for the data fetchers
    out.putInt(8);
    for(i = 0; i < 8; ++i)
      out.putByte((char)myTops[i]);

    // The bottom registers for the data fetchers
    out.putInt(8);
    for(i = 0; i < 8; ++i)
      out.putByte((char)myBottoms[i]);

    // The counter registers for the data fetchers
    out.putInt(8);
    for(i = 0; i < 8; ++i)
      out.putInt(myCounters[i]);
    
    // The counter registers for the fractional data fetchers
    out.putInt(8);
    for(i = 0; i < 8; ++i)
      out.putInt(myFractionalCounters[i]);

    // The fractional registers for the data fetchers
    out.putInt(8);
    for(i = 0; i < 8; ++i)
      out.putByte((char)myFractionalIncrements[i]);

    // The flag registers for the data fetchers
    out.putInt(8);
    for(i = 0; i < 8; ++i)
      out.putByte((char)myFlags[i]);

    // The Fast Fetcher Enabled flag
    out.putBool(myFastFetch);
    out.putBool(myLDAimmediate);

    // The music mode flags for the data fetchers
    out.putInt(3);
    for(i = 0; i < 3; ++i)
      out.putBool(myMusicMode[i]);
    
    // The music mode counters for the data fetchers
    out.putInt(3);
    for(i = 0; i < 3; ++i)
      out.putInt(myMusicCounter[i]);    

    // The music mode frequency addends for the data fetchers
    out.putInt(3);
    for(i = 0; i < 3; ++i)
      out.putInt(myMusicFrequency[i]);
    
    // The random number generator register
    out.putInt(myRandomNumber);

    out.putInt(mySystemCycles);
    out.putInt((uInt32)(myFractionalClocks * 100000000.0));
  }
  catch(const char* msg)
  {
    cerr << "ERROR: CartridgeDPCPlus::save" << endl << "  " << msg << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDPCPlus::load(Serializer& in)
{
  const string& cart = name();

  try
  {
    if(in.getString() != cart)
      return false;

    uInt32 i, limit;

    // Indicates which bank is currently active
    myCurrentBank = (uInt16) in.getInt();

    // The top registers for the data fetchers
    limit = (uInt32) in.getInt();
    for(i = 0; i < limit; ++i)
      myTops[i] = (uInt8) in.getByte();

    // The bottom registers for the data fetchers
    limit = (uInt32) in.getInt();
    for(i = 0; i < limit; ++i)
      myBottoms[i] = (uInt8) in.getByte();

    // The counter registers for the data fetchers
    limit = (uInt32) in.getInt();
    for(i = 0; i < limit; ++i)
      myCounters[i] = (uInt16) in.getInt();

    // The counter registers for the fractional data fetchers
    limit = (uInt32) in.getInt();
    for(i = 0; i < limit; ++i)
      myFractionalCounters[i] = (uInt32) in.getInt();

    // The fractional registers for the data fetchers
    limit = (uInt32) in.getInt();
    for(i = 0; i < limit; ++i)
      myFractionalIncrements[i] = (uInt8) in.getByte();

    // The flag registers for the data fetchers
    limit = (uInt32) in.getInt();
    for(i = 0; i < limit; ++i)
      myFlags[i] = (uInt8) in.getByte();

    // The Fast Fetcher Enabled flag
    myFastFetch = in.getBool();
    myLDAimmediate = in.getBool();

    // The music mode flags for the data fetchers
    limit = (uInt32) in.getInt();
    for(i = 0; i < limit; ++i)
      myMusicMode[i] = in.getBool();
    
    // The music mode counters for the data fetchers
    limit = (uInt32) in.getInt();
    for(i = 0; i < limit; ++i)
      myMusicCounter[i] = (uInt32) in.getInt();   
    
    // The music mode frequency addends for the data fetchers
    limit = (uInt32) in.getInt();
    for(i = 0; i < limit; ++i)
      myMusicFrequency[i] = (uInt32) in.getInt();     

    // The random number generator register
    myRandomNumber = (uInt32) in.getInt();

    // Get system cycles and fractional clocks
    mySystemCycles = in.getInt();
    myFractionalClocks = (double)in.getInt() / 100000000.0;
  }
  catch(const char* msg)
  {
    cerr << "ERROR: CartridgeDPCPlus::load" << endl << "  " << msg << endl;
    return false;
  }

  // Now, go to the current bank
  bank(myCurrentBank);

  return true;
}
