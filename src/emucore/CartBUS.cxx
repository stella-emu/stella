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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <cstring>

#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
#endif
#include "System.hxx"
#include "M6532.hxx"
#include "TIA.hxx"
#include "Thumbulator.hxx"
#include "CartBUS.hxx"

// Location of data within the RAM copy of the BUS Driver.
#define DSxPTR        0x06E0
#define DSxINC        0x0720
#define DSMAPS        0x0760
#define WAVEFORM      0x07F4
#define DSRAM         0x0800


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeBUS::CartridgeBUS(const uInt8* image, uInt32 size,
                                   const Settings& settings)
  : Cartridge(settings),
    mySystemCycles(0),
    myFractionalClocks(0.0)
{
  // Copy the ROM image into my buffer
  memcpy(myImage, image, std::min(32768u, size));

  // even though the ROM is 32K, only 28K is accessible to the 6507
  createCodeAccessBase(4096 * 7);

  // Pointer to the program ROM (28K @ 0 byte offset)
  // which starts after the 2K BUS Driver and 2K C Code
  myProgramImage = myImage + 4096;

  // Pointer to BUS driver in RAM
  myBusDriverImage = myBUSRAM;

  // Pointer to the display RAM
  myDisplayImage = myBUSRAM + DSRAM;

#ifdef THUMB_SUPPORT
  // Create Thumbulator ARM emulator
  myThumbEmulator = make_ptr<Thumbulator>((uInt16*)myImage, (uInt16*)myBUSRAM,
    settings.getBool("thumb.trapfatal"), Thumbulator::ConfigureFor::BUS, this);
#endif
  setInitialState();

  // BUS always starts in bank 6
  myStartBank = 6;

  // bus stuffing is off by default
  myBusStuff = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::reset()
{
  // Initialize RAM
  if(mySettings.getBool("ramrandom"))
    initializeRAM(myBUSRAM+2048, 8192-2048);
  else
    memset(myBUSRAM+2048, 0, 8192-2048);

  // Update cycles to the current system cycles
  mySystemCycles = mySystem->cycles();
  myFractionalClocks = 0.0;

  setInitialState();

  // Upon reset we switch to the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::setInitialState()
{
  // Copy initial BUS driver to Harmony RAM
  memcpy(myBusDriverImage, myImage, 0x0800);

  for (int i=0; i < 3; ++i)
    myMusicWaveformSize[i] = 27;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::consoleChanged(ConsoleTiming timing)
{
#ifdef THUMB_SUPPORT
  myThumbEmulator->setConsoleTiming(timing);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::systemCyclesReset()
{
  // Adjust the cycle counter so that it reflects the new value
  mySystemCycles -= mySystem->cycles();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::install(System& system)
{
  mySystem = &system;

  // Map all of the accesses to call peek and poke
  System::PageAccess access(this, System::PA_READ);
  for(uInt32 i = 0x1000; i < 0x1040; i += (1 << System::PAGE_SHIFT))
    mySystem->setPageAccess(i >> System::PAGE_SHIFT, access);

  // Mirror all access in TIA and RIOT; by doing so we're taking responsibility
  // for that address space in peek and poke below.
  mySystem->tia().installDelegate(system, *this);
  mySystem->m6532().installDelegate(system, *this);

  // Install pages for the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void CartridgeBUS::updateMusicModeDataFetchers()
{
  // Calculate the number of cycles since the last update
  Int32 cycles = mySystem->cycles() - mySystemCycles;
  mySystemCycles = mySystem->cycles();

  // Calculate the number of BUS OSC clocks since the last update
  double clocks = ((20000.0 * cycles) / 1193191.66666667) + myFractionalClocks;
  Int32 wholeClocks = Int32(clocks);
  myFractionalClocks = clocks - double(wholeClocks);

  if(wholeClocks <= 0)
  {
    return;
  }

  // Let's update counters and flags of the music mode data fetchers
  for(int x = 0; x <= 2; ++x)
  {
    myMusicCounters[x] += myMusicFrequencies[x];
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void CartridgeBUS::callFunction(uInt8 value)
{
  switch (value)
  {
  #ifdef THUMB_SUPPORT
      // Call user written ARM code (will most likely be C compiled for ARM)
    case 254: // call with IRQ driven audio, no special handling needed at this
              // time for Stella as ARM code "runs in zero 6507 cycles".
    case 255: // call without IRQ driven audio
      try {
        myThumbEmulator->run();
      }
      catch(const runtime_error& e) {
        if(!mySystem->autodetectMode())
        {
      #ifdef DEBUGGER_SUPPORT
          Debugger::debugger().startWithFatalError(e.what());
      #else
          cout << e.what() << endl;
      #endif
        }
      }
      break;
  #endif
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeBUS::peek(uInt16 address)
{
  if(!(address & 0x1000))                      // Hotspots below 0x1000
  {
    // Check for RAM or TIA mirroring
    uInt16 lowAddress = address & 0x3ff;
    if(lowAddress & 0x80)
      return mySystem->m6532().peek(address);
    else if(!(lowAddress & 0x200))
      return mySystem->tia().peek(address);
  }
  else
  {
    address &= 0x0FFF;

    uInt8 peekvalue = myProgramImage[(myCurrentBank << 12) + address];

    // In debugger/bank-locked mode, we ignore all hotspots and in general
    // anything that can change the internal state of the cart
    if(bankLocked())
      return peekvalue;

    // save the STY's zero page address
    if (getBusStuffFlag() && mySTYZeroPage)
      myBusOverdriveAddress =  peekvalue;

    mySTYZeroPage = false;

    if(address < 0x20)
    {
      uInt8 result = 0;

      // Get the index of the data fetcher that's being accessed
      uInt32 index = address & 0x0f;
      uInt32 function = (address >> 4) & 0x01;

      switch(function)
      {
        case 0x00:  // read from a datastream
        {
          result = readFromDatastream(index);
          break;
        }
        case 0x01:  // misc read registers
        {
          switch(index)
          {
              // the following are POKE ONLY
            case 0x00:  // 0x10 DF0WRITE
            case 0x01:  // 0x11 DF1WRITE
            case 0x02:  // 0x12 DF2WRITE
            case 0x03:  // 0x13 DF3WRITE
            case 0x04:  // 0x14 DF0PTR
            case 0x05:  // 0x15 DF1PTR
            case 0x06:  // 0x16 DF2PTR
            case 0x07:  // 0x17 DF3PTR
            case 0x09:  // 0x19 STUFFMODE
            case 0x0a:  // 0x1A CALLFN
              break;

            case 0x08:  // 0x18 = AMPLITUDE
              // Update the music data fetchers (counter & flag)
              updateMusicModeDataFetchers();

              // using myDisplayImage[] instead of myProgramImage[] because waveforms
              // can be modified during runtime.
              uInt32 i = myDisplayImage[(getWaveform(0) ) + (myMusicCounters[0] >> myMusicWaveformSize[0])] +
              myDisplayImage[(getWaveform(1) ) + (myMusicCounters[1] >> myMusicWaveformSize[1])] +
              myDisplayImage[(getWaveform(2) ) + (myMusicCounters[2] >> myMusicWaveformSize[2])];

              result = uInt8(i);
              break;
          }
          break;
        }
      }

      return result;
    }
    else
    {
      // Switch banks if necessary
      switch(address)
      {
        case 0xFF5:
          // Set the current bank to the first 4k bank
          bank(0);
          break;

        case 0x0FF6:
          // Set the current bank to the second 4k bank
          bank(1);
          break;

        case 0x0FF7:
          // Set the current bank to the third 4k bank
          bank(2);
          break;

        case 0x0FF8:
          // Set the current bank to the fourth 4k bank
          bank(3);
          break;

        case 0x0FF9:
          // Set the current bank to the fifth 4k bank
          bank(4);
          break;

        case 0x0FFA:
          // Set the current bank to the sixth 4k bank
          bank(5);
          break;

        case 0x0FFB:
          // Set the current bank to the last 4k bank
          bank(6);
          break;

        default:
          break;
      }

      // this might not work right for STY $84
      if (getBusStuffFlag())
        mySTYZeroPage = (peekvalue == 0x84);

      return peekvalue;
    }
  }

  return 0;  // make compiler happy
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBUS::poke(uInt16 address, uInt8 value)
{
  if (!(address & 0x1000))
  {
    value &= busOverdrive(address);

    // Check for RAM or TIA mirroring
    uInt16 lowAddress = address & 0x3ff;
    if(lowAddress & 0x80)
      mySystem->m6532().poke(address, value);
    else if(!(lowAddress & 0x200))
      mySystem->tia().poke(address, value);
  }
  else
  {
    address &= 0x0FFF;

    if ((address >= 0x10) && (address <= 0x1F))
    {
      // Get the index of the data fetcher that's being accessed
      uInt32 index = address & 0x0f;
      uInt32 pointer;

      switch (index)
      {
        case 0x00:  // DS0WRITE
        case 0x01:  // DS1WRITE
        case 0x02:  // DS2WRITE
        case 0x03:  // DS3WRITE
          // Pointers are stored as:
          // PPPFF---
          //
          // P = Pointer
          // F = Fractional

          pointer = getDatastreamPointer(index);
          myDisplayImage[ pointer >> 20 ] = value;
          pointer += 0x100000;  // always increment by 1 when writing
          setDatastreamPointer(index, pointer);
          break;

        case 0x04:  // 0x14 DS0PTR
        case 0x05:  // 0x15 DS1PTR
        case 0x06:  // 0x16 DS2PTR
        case 0x07:  // 0x17 DS3PTR
          // Pointers are stored as:
          // PPPFF---
          //
          // P = Pointer
          // F = Fractional

          index &= 0x03;
          pointer = getDatastreamPointer(index);
          pointer <<=8;
          pointer &= 0xf0000000;
          pointer |= (value << 20);
          setDatastreamPointer(index, pointer);
          break;

        case 0x09:  // 0x19 turn on STY ZP bus stuffing if value is 0
          setBusStuffFlag(value==0);
          break;

        case 0x0A:  // 0x1A CALLFUNCTION
          callFunction(value);
          break;
      }
    }
    else
    {
      // Switch banks if necessary
      switch(address)
      {
        case 0xFF5:
          // Set the current bank to the first 4k bank
          bank(0);
          break;

        case 0x0FF6:
          // Set the current bank to the second 4k bank
          bank(1);
          break;

        case 0x0FF7:
          // Set the current bank to the third 4k bank
          bank(2);
          break;

        case 0x0FF8:
          // Set the current bank to the fourth 4k bank
          bank(3);
          break;

        case 0x0FF9:
          // Set the current bank to the fifth 4k bank
          bank(4);
          break;

        case 0x0FFA:
          // Set the current bank to the sixth 4k bank
          bank(5);
          break;

        case 0x0FFB:
          // Set the current bank to the last 4k bank
          bank(6);
          break;

        default:
          break;
      }
    }
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBUS::bank(uInt16 bank)
{
  if(bankLocked()) return false;

  // Remember what bank we're in
  myCurrentBank = bank;
  uInt16 offset = myCurrentBank << 12;

  // Setup the page access methods for the current bank
  System::PageAccess access(this, System::PA_READ);

  // Map Program ROM image into the system
  for(uInt32 address = 0x1040; address < 0x2000;
      address += (1 << System::PAGE_SHIFT))
  {
    access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x0FFF)];
    mySystem->setPageAccess(address >> System::PAGE_SHIFT, access);
  }
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeBUS::getBank() const
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeBUS::bankCount() const
{
  return 7;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBUS::patch(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

  // For now, we ignore attempts to patch the BUS address space
  if(address >= 0x0040)
  {
    myProgramImage[(myCurrentBank << 12) + (address & 0x0FFF)] = value;
    return myBankChanged = true;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeBUS::getImage(int& size) const
{
  size = 32768;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeBUS::busOverdrive(uInt16 address)
{
  uInt8 overdrive = 0xff;

  // Not sure how to do this, check with stephena.
  //
  // Per discussion with cd-w, have this routine check that the Y register has a
  // value of 0xFF.  Of it doesn't then "crash the emulation".


  // only overdrive if the address matches
  if (address == myBusOverdriveAddress)
  {
    uInt8 map = address & 0x7f;
    if (map <= 0x24) // map TIA registers VSYNC thru HMBL inclusive
    {
      uInt32 alldatastreams = getAddressMap(map);
      uInt8 datastream = alldatastreams & 0x0f;  // lowest nybble has the current datastream to use
      overdrive = readFromDatastream(datastream);

      // rotate map nybbles for next time
      alldatastreams >>= 4;
      alldatastreams |= (datastream << 28);
      setAddressMap(map, alldatastreams);

      // overdrive |= 0x7c; // breaks bus stuffing to match hobo's system
    }
  }

  myBusOverdriveAddress = 0xff; // turns off overdrive for next poke event

  return overdrive;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeBUS::thumbCallback(uInt8 function, uInt32 value1, uInt32 value2)
{
  switch (function)
  {
    case 0:
      // _SetNote - set the note/frequency
      myMusicFrequencies[value1] = value2;
      break;

      // _ResetWave - reset counter,
      // used to make sure digital samples start from the beginning
    case 1:
      myMusicCounters[value1] = 0;
      break;

      // _GetWavePtr - return the counter
    case 2:
      return myMusicCounters[value1];
      break;

      // _SetWaveSize - set size of waveform buffer
    case 3:
      myMusicWaveformSize[value1] = value2;
      break;
  }

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBUS::save(Serializer& out) const
{
  try
  {
    out.putString(name());

    // Indicates which bank is currently active
    out.putShort(myCurrentBank);

    // Harmony RAM
    out.putByteArray(myBUSRAM, 8192);

    out.putInt(mySystemCycles);
    out.putInt((uInt32)(myFractionalClocks * 100000000.0));
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeBUS::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBUS::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    // Indicates which bank is currently active
    myCurrentBank = in.getShort();

    // Harmony RAM
    in.getByteArray(myBUSRAM, 8192);

    // Get system cycles and fractional clocks
    mySystemCycles = (Int32)in.getInt();
    myFractionalClocks = (double)in.getInt() / 100000000.0;
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeBUS::load" << endl;
    return false;
  }

  // Now, go to the current bank
  bank(myCurrentBank);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeBUS::getDatastreamPointer(uInt8 index) const
{
//  index &= 0x0f;

  return myBUSRAM[DSxPTR + index*4 + 0]        +  // low byte
        (myBUSRAM[DSxPTR + index*4 + 1] << 8)  +
        (myBUSRAM[DSxPTR + index*4 + 2] << 16) +
        (myBUSRAM[DSxPTR + index*4 + 3] << 24) ;  // high byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::setDatastreamPointer(uInt8 index, uInt32 value)
{
//  index &= 0x0f;
  myBUSRAM[DSxPTR + index*4 + 0] = value & 0xff;          // low byte
  myBUSRAM[DSxPTR + index*4 + 1] = (value >> 8) & 0xff;
  myBUSRAM[DSxPTR + index*4 + 2] = (value >> 16) & 0xff;
  myBUSRAM[DSxPTR + index*4 + 3] = (value >> 24) & 0xff;  // high byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeBUS::getDatastreamIncrement(uInt8 index) const
{
//  index &= 0x0f;
  return myBUSRAM[DSxINC + index*4 + 0]        +   // low byte
        (myBUSRAM[DSxINC + index*4 + 1] << 8)  +
        (myBUSRAM[DSxINC + index*4 + 2] << 16) +
        (myBUSRAM[DSxINC + index*4 + 3] << 24) ;   // high byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::setDatastreamIncrement(uInt8 index, uInt32 value)
{
//  index &= 0x0f;
  myBUSRAM[DSxINC + index*4 + 0] = value & 0xff;          // low byte
  myBUSRAM[DSxINC + index*4 + 1] = (value >> 8) & 0xff;
  myBUSRAM[DSxINC + index*4 + 2] = (value >> 16) & 0xff;
  myBUSRAM[DSxINC + index*4 + 3] = (value >> 24) & 0xff;  // high byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeBUS::getAddressMap(uInt8 index) const
{
  //  index &= 0x0f;
  return myBUSRAM[DSMAPS + index*4 + 0]        +   // low byte
        (myBUSRAM[DSMAPS + index*4 + 1] << 8)  +
        (myBUSRAM[DSMAPS + index*4 + 2] << 16) +
        (myBUSRAM[DSMAPS + index*4 + 3] << 24) ;   // high byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeBUS::getWaveform(uInt8 index) const
{
  // instead of 0, 1, 2, etc. this returned
  // 0x40000800 for 0
  // 0x40000820 for 1
  // 0x40000840 for 2
  // ...

//  return myBUSRAM[WAVEFORM + index*4 + 0]        +   // low byte
//        (myBUSRAM[WAVEFORM + index*4 + 1] << 8)  +
//        (myBUSRAM[WAVEFORM + index*4 + 2] << 16) +
//        (myBUSRAM[WAVEFORM + index*4 + 3] << 24) -   // high byte
//         0x40000800;

  uInt32 result;

  result = myBUSRAM[WAVEFORM + index*4 + 0]        +   // low byte
          (myBUSRAM[WAVEFORM + index*4 + 1] << 8)  +
          (myBUSRAM[WAVEFORM + index*4 + 2] << 16) +
          (myBUSRAM[WAVEFORM + index*4 + 3] << 24);

  result -= 0x40000800;

  if (result >= 4096)
    result = 0;

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeBUS::getWaveformSize(uInt8 index) const
{
  return myMusicWaveformSize[index];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::setAddressMap(uInt8 index, uInt32 value)
{
  //  index &= 0x0f;
  myBUSRAM[DSMAPS + index*4 + 0] = value & 0xff;          // low byte
  myBUSRAM[DSMAPS + index*4 + 1] = (value >> 8) & 0xff;
  myBUSRAM[DSMAPS + index*4 + 2] = (value >> 16) & 0xff;
  myBUSRAM[DSMAPS + index*4 + 3] = (value >> 24) & 0xff;  // high byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBUS::getBusStuffFlag(void) const
{
  return myBusStuff;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::setBusStuffFlag(bool value)
{
  myBusStuff = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeBUS::readFromDatastream(uInt8 index)
{
  // Pointers are stored as:
  // PPPFF---
  //
  // Increments are stored as
  // ----IIFF
  //
  // P = Pointer
  // I = Increment
  // F = Fractional

  uInt32 pointer = getDatastreamPointer(index);
  uInt16 increment = getDatastreamIncrement(index);
  uInt8 value = myDisplayImage[ pointer >> 20 ];
  pointer += (increment << 12);
  setDatastreamPointer(index, pointer);
  return value;
}
