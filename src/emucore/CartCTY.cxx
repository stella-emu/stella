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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <cassert>
#include <cstring>

#include "OSystem.hxx"
#include "Serializer.hxx"
#include "System.hxx"
#include "CartCTYTunes.hxx"
#include "CartCTY.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCTY::CartridgeCTY(const uInt8* image, uInt32 size, const OSystem& osystem)
  : Cartridge(osystem.settings()),
    myOSystem(osystem),
    myOperationType(0),
    myRamAccessTimeout(0)
{
  // Copy the ROM image into my buffer
  memcpy(myImage, image, BSPF_min(32768u, size));
  createCodeAccessBase(32768);

  // This cart contains 64 bytes extended RAM @ 0x1000
  registerRamArea(0x1000, 64, 0x40, 0x00);

  // Remember startup bank (not bank 0, since that's ARM code)
  myStartBank = 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCTY::~CartridgeCTY()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCTY::reset()
{
  // Initialize RAM
  if(mySettings.getBool("ramrandom"))
    for(uInt32 i = 0; i < 64; ++i)
      myRAM[i] = mySystem->randGenerator().next();
  else
    memset(myRAM, 0, 64);

  myRAM[0] = myRAM[1] = myRAM[2] = myRAM[3] = 0xFF;

  // Upon reset we switch to the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCTY::install(System& system)
{
  mySystem = &system;
  uInt16 mask = mySystem->pageMask();
  uInt16 shift = mySystem->pageShift();

  // Make sure the system we're being installed in has a page size that'll work
  assert(((0x1000 & mask) == 0) && ((0x1080 & mask) == 0));

  // Map all RAM accesses to call peek and poke
  System::PageAccess access(0, 0, 0, this, System::PA_READ);
  for(uInt32 i = 0x1000; i < 0x1080; i += (1 << shift))
    mySystem->setPageAccess(i >> shift, access);

  // Install pages for the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeCTY::peek(uInt16 address)
{
  uInt16 peekAddress = address;
  address &= 0x0FFF;
  uInt8 peekValue = myImage[(myCurrentBank << 12) + address];

  // In debugger/bank-locked mode, we ignore all hotspots and in general
  // anything that can change the internal state of the cart
  if(bankLocked())
    return peekValue;

  if(address < 0x0040)  // Write port is at $1000 - $103F (64 bytes)
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
  else if(address < 0x0080)  // Read port is at $1040 - $107F (64 bytes)
  {
    address -= 0x40;
    switch(address)  // FIXME for actual return values (0-3)
    {
      case 0x00:  // Error code after operation
        return myRAM[0];
      case 0x01:  // Get next Random Number (8-bit LFSR)
        cerr << "Get next Random Number (8-bit LFSR)\n";
        return 0xFF;
      case 0x02:  // Get Tune position (low byte)
        cerr << "Get Tune position (low byte)\n";
        return 0x00;
      case 0x03:  // Get Tune position (high byte)
        cerr << "Get Tune position (high byte)\n";
        return 0x00;
      default:
        return myRAM[address];
    }
  }
  else  // Check hotspots
  {
    switch(address)
    {
      case 0x0FF4:
        // Bank 0 is ARM code and not actually accessed
        return ramReadWrite();
      case 0x0FF5:
      case 0x0FF6:
      case 0x0FF7:
      case 0x0FF8:
      case 0x0FF9:
      case 0x0FFA:
      case 0x0FFB:
        // Banks 1 through 7
        bank(address - 0x0FF4);
        break;
      default:
        break;
    }
    return myImage[(myCurrentBank << 12) + address];
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCTY::poke(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

//cerr << "POKE: address=" << HEX4 << address << ", value=" << HEX2 << value << endl;
  if(address < 0x0040)  // Write port is at $1000 - $103F (64 bytes)
  {
    switch(address)  // FIXME for functionality
    {
      case 0x00:  // Operation type for $1FF4
        myOperationType = value;
        break;
      case 0x01:  // Set Random seed value
        cerr << "Set random seed value = " << HEX2 << (int)value << endl;
        break;
      case 0x02:  // Reset fetcher to beginning of tune
        cerr << "Reset fetcher to beginning of tune\n";
        break;
      case 0x03:  // Advance fetcher to next tune position
        cerr << "Advance fetcher to next tune position\n";
        break;
      default:
        myRAM[address] = value;
        break;
    }
  }
  else  // Check hotspots
  {
    switch(address)
    {
      case 0x0FF4:
        // Bank 0 is ARM code and not actually accessed
        ramReadWrite();
        break;
      case 0x0FF5:
      case 0x0FF6:
      case 0x0FF7:
      case 0x0FF8:
      case 0x0FF9:
      case 0x0FFA:
      case 0x0FFB:
        // Banks 1 through 7
        bank(address - 0x0FF4);
        break;
      default:
        break;
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCTY::bank(uInt16 bank)
{ 
  if(bankLocked()) return false;

  // Remember what bank we're in
  myCurrentBank = bank;
  uInt16 offset = myCurrentBank << 12;
  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  System::PageAccess access(0, 0, 0, this, System::PA_READ);

  // Set the page accessing methods for the hot spots
  for(uInt32 i = (0x1FF4 & ~mask); i < 0x2000; i += (1 << shift))
  {
    access.codeAccessBase = &myCodeAccessBase[offset + (i & 0x0FFF)];
    mySystem->setPageAccess(i >> shift, access);
  }

  // Setup the page access methods for the current bank
  for(uInt32 address = 0x1080; address < (0x1FF4U & ~mask);
      address += (1 << shift))
  {
    access.directPeekBase = &myImage[offset + (address & 0x0FFF)];
    access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x0FFF)];
    mySystem->setPageAccess(address >> shift, access);
  }
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeCTY::bank() const
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeCTY::bankCount() const
{
  return 8;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCTY::patch(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

  if(address < 0x0080)
  {
    // Normally, a write to the read port won't do anything
    // However, the patch command is special in that ignores such
    // cart restrictions
    myRAM[address & 0x003F] = value;
  }
  else
    myImage[(myCurrentBank << 12) + address] = value;

  return myBankChanged = true;
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeCTY::getImage(int& size) const
{
  size = 32768;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCTY::save(Serializer& out) const
{
  try
  {
    out.putString(name());
    out.putShort(myCurrentBank);
    out.putByte(myOperationType);
    out.putByteArray(myRAM, 64);
  }
  catch(const char* msg)
  {
    cerr << "ERROR: CartridgeCTY::save" << endl << "  " << msg << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCTY::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    myCurrentBank = in.getShort();
    myOperationType = in.getByte();
    in.getByteArray(myRAM, 64);
  }
  catch(const char* msg)
  {
    cerr << "ERROR: CartridgeCTY::load" << endl << "  " << msg << endl;
    return false;
  }

  // Remember what bank we were in
  bank(myCurrentBank);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCTY::setRomName(const string& name)
{
  myEEPROMFile = myOSystem.eepromDir() + name + "_eeprom.dat";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeCTY::ramReadWrite()
{
  /* The following algorithm implements accessing Harmony cart EEPROM

    1. Wait for an access to hotspot location $1FF4 (return 1 in bit 6
       while busy).

    2. Determine operation from myOperationType.

    3. Save or load relevant EEPROM memory to/from a file.

    4. Set byte 0 of RAM+ memory to zero to indicate success (will
       always happen in emulation).

    5. Return 0 (in bit 6) on the next access to $1FF4, if enough time has
       passed to complete the operation on a real system (0.5 s for read,
       1 s for write).
  */

  if(bankLocked()) return 0xff;

  // First access sets the timer
  if(myRamAccessTimeout == 0)
  {
    // Opcode and value in form of XXXXYYYY (from myOperationType), where:
    //    XXXX = index and YYYY = operation
    uInt8 index = myOperationType >> 4;
    switch(myOperationType & 0xf)
    {
      case 1:  // Load tune (index = tune)
        if(index < 7)
        {
          // Add 0.5 s delay for read
          myRamAccessTimeout = myOSystem.getTicks() + 500000;
          loadTune(index);
        }
        break;
      case 2:  // Load score table (index = table)
        if(index < 4)
        {
          // Add 0.5 s delay for read
          myRamAccessTimeout = myOSystem.getTicks() + 500000;
          loadScore(index);
        }
        break;
      case 3:  // Save score table (index = table)
        if(index < 4)
        {
          // Add 1 s delay for write
          myRamAccessTimeout = myOSystem.getTicks() + 1000000;
          saveScore(index);
        }
        break;
      case 4:  // Wipe all score tables
        // Add 1 s delay for write
        myRamAccessTimeout = myOSystem.getTicks() + 1000000;
        wipeAllScores();
        break;
    }
    // Bit 6 is 1, busy
    return myImage[(myCurrentBank << 12) + 0xFF4] | 0x40;
  }
  else
  {
    // Have we reached the timeout value yet?
    if(myOSystem.getTicks() >= myRamAccessTimeout)
    {
      myRamAccessTimeout = 0;  // Turn off timer
      myRAM[0] = 0;            // Successful operation

      // Bit 6 is 0, ready/success
      return myImage[(myCurrentBank << 12) + 0xFF4] & ~0x40;
    }
    else
      // Bit 6 is 1, busy
      return myImage[(myCurrentBank << 12) + 0xFF4] | 0x40;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCTY::loadTune(uInt8 index)
{
  cerr << "load tune " << (int)index << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCTY::loadScore(uInt8 index)
{
  Serializer serializer(myEEPROMFile, true);
  if(serializer.isValid())
  {
    uInt8 scoreRAM[256];
    try
    {
      serializer.getByteArray(scoreRAM, 256);
    }
    catch(const char* msg)
    {
      memset(scoreRAM, 0, 256);
    }
    // Grab 64B slice @ given index
    memcpy(myRAM, scoreRAM + (index << 6), 64);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCTY::saveScore(uInt8 index)
{
  Serializer serializer(myEEPROMFile);
  if(serializer.isValid())
  {
    // Load score RAM
    uInt8 scoreRAM[256];
    try
    {
      serializer.getByteArray(scoreRAM, 256);
    }
    catch(const char* msg)
    {
      memset(scoreRAM, 0, 256);
    }

    // Add 64B RAM to score table @ given index
    memcpy(scoreRAM + (index << 6), myRAM, 64);

    // Save score RAM
    serializer.reset();
    try
    {
      serializer.putByteArray(scoreRAM, 256);
    }
    catch(const char* msg)
    {
      // Maybe add logging here that save failed?
      cerr << name() << ": ERROR saving score table " << (int)index << endl;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCTY::wipeAllScores()
{
  Serializer serializer(myEEPROMFile);
  if(serializer.isValid())
  {
    // Erase score RAM
    uInt8 scoreRAM[256];
    memset(scoreRAM, 0, 256);
    try
    {
      serializer.putByteArray(scoreRAM, 256);
    }
    catch(const char* msg)
    {
      // Maybe add logging here that save failed?
      cerr << name() << ": ERROR wiping score tables" << endl;
    }
  }
}
