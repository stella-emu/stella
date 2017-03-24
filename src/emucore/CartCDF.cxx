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
#include "Thumbulator.hxx"
#include "CartCDF.hxx"
#include "TIA.hxx"

// Location of data within the RAM copy of the CDF Driver.
#define DSxPTR        0x06E0
#define DSxINC        0x0760
#define WAVEFORM      0x07E0
#define DSRAM         0x0800

#define FAST_FETCH_ON ((myMode & 0x0F) == 0)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCDF::CartridgeCDF(const uInt8* image, uInt32 size,
                           const Settings& settings)
  : Cartridge(settings),
    mySystemCycles(0),
    myARMCycles(0),
    myFractionalClocks(0.0)
{
  // Copy the ROM image into my buffer
  memcpy(myImage, image, std::min(32768u, size));

  // even though the ROM is 32K, only 28K is accessible to the 6507
  createCodeAccessBase(4096 * 7);

  // Pointer to the program ROM (28K @ 0 byte offset)
  // which starts after the 2K CDF Driver and 2K C Code
  myProgramImage = myImage + 4096;

  // Pointer to CDF driver in RAM
  myBusDriverImage = myCDFRAM;

  // Pointer to the display RAM
  myDisplayImage = myCDFRAM + DSRAM;
#ifdef THUMB_SUPPORT
  // Create Thumbulator ARM emulator
  myThumbEmulator = make_ptr<Thumbulator>((uInt16*)myImage, (uInt16*)myCDFRAM,
      settings.getBool("thumb.trapfatal"), Thumbulator::ConfigureFor::CDF, this);
#endif
  setInitialState();

  // CDF always starts in bank 6
  myStartBank = 6;

  // Assuming mode starts out with Fast Fetch off and 3-Voice music,
  // need to confirm with Chris
  myMode = 0xFF;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::reset()
{
  // Initialize RAM
  if(mySettings.getBool("ramrandom"))
    initializeRAM(myCDFRAM+2048, 8192-2048);
  else
    memset(myCDFRAM+2048, 0, 8192-2048);

  // Update cycles to the current system cycles
  mySystemCycles = mySystem->cycles();
  myARMCycles = mySystem->cycles();
  myFractionalClocks = 0.0;

  setInitialState();

  // Upon reset we switch to the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::setInitialState()
{
  // Copy initial CDF driver to Harmony RAM
  memcpy(myBusDriverImage, myImage, 0x0800);

  for (int i=0; i < 3; ++i)
    myMusicWaveformSize[i] = 27;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::consoleChanged(ConsoleTiming timing)
{
#ifdef THUMB_SUPPORT
  myThumbEmulator->setConsoleTiming(timing);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::systemCyclesReset()
{
  // Adjust the cycle counter so that it reflects the new value
  mySystemCycles -= mySystem->cycles();
  myARMCycles -= mySystem->cycles();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::install(System& system)
{
  mySystem = &system;

  // Map all of the accesses to call peek and poke
  System::PageAccess access(this, System::PA_READ);
  for(uInt32 i = 0x1000; i < 0x1040; i += (1 << System::PAGE_SHIFT))
    mySystem->setPageAccess(i >> System::PAGE_SHIFT, access);

  // Install pages for the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void CartridgeCDF::updateMusicModeDataFetchers()
{
  // Calculate the number of cycles since the last update
  Int32 cycles = mySystem->cycles() - mySystemCycles;
  mySystemCycles = mySystem->cycles();

  // Calculate the number of CDF OSC clocks since the last update
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
inline void CartridgeCDF::callFunction(uInt8 value)
{
  switch (value)
  {
#ifdef THUMB_SUPPORT
    // Call user written ARM code (will most likely be C compiled for ARM)
    case 254: // call with IRQ driven audio, no special handling needed at this
              // time for Stella as ARM code "runs in zero 6507 cycles".
    case 255: // call without IRQ driven audio
      try {
        Int32 cycles = mySystem->cycles() - myARMCycles;
        myARMCycles = mySystem->cycles();

        myThumbEmulator->run(cycles);
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
uInt8 CartridgeCDF::peek(uInt16 address)
{
  address &= 0x0FFF;

  uInt8 peekvalue = myProgramImage[(myCurrentBank << 12) + address];

  // In debugger/bank-locked mode, we ignore all hotspots and in general
  // anything that can change the internal state of the cart
  if(bankLocked())
    return peekvalue;

  // Check if we're in Fast Fetch mode and the prior byte was an A9 (LDA #value)
  if(FAST_FETCH_ON && myLDAimmediateOperandAddress == address)
  {
    if(peekvalue < 0x0028)
      // if #value is a read-register then we want to use that as the address
      address = peekvalue;
  }
  myLDAimmediateOperandAddress = 0;

  if(address <= 0x20)
  {
    uInt8 result = 0;

    // Get the index of the data fetcher that's being accessed
    uInt32 index = address & 0x1f;
    uInt32 function = (address >> 5) & 0x01;

    switch(function)
    {
      case 0x00:  // read from a datastream
      {
        result = readFromDatastream(index);
        break;
      }
      case 0x02:  // misc read registers
      {
        // index will be 0 for address 0x20 = AMPLITUDE
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

    if(FAST_FETCH_ON && peekvalue == 0xA9)
      myLDAimmediateOperandAddress = address + 1;

    return peekvalue;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCDF::poke(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

  if ((address >= 0x21) && (address <= 0x2B))
  {
    // Get the index of the data fetcher that's being accessed
    uInt32 index = address & 0x0f;
    uInt32 pointer;
    uInt32 stream = address & 0x03;

    switch (index)
    {
      case 0x00:  // 0x20 AMPLITUDE - read register
        break;

      case 0x01:  // 0x21 SETMODE
        myMode = value;
        break;

      case 0x02:  // 0x22 CALLFN
        callFunction(value);
        break;

      case 0x03:  // 0x23 RESERVED
        break;


      case 0x04:  // 0x24 DS0WRITE
      case 0x05:  // 0x25 DS1WRITE
      case 0x06:  // 0x26 DS2WRITE
      case 0x07:  // 0x27 DS3WRITE
        // Pointers are stored as:
        // PPPFF---
        //
        // P = Pointer
        // F = Fractional

        pointer = getDatastreamPointer(stream);
        myDisplayImage[ pointer >> 20 ] = value;
        pointer += 0x100000;  // always increment by 1 when writing
        setDatastreamPointer(stream, pointer);
        break;

      case 0x08:  // 0x28 DS0PTR
      case 0x09:  // 0x29 DS1PTR
      case 0x0A:  // 0x2A DS2PTR
      case 0x0B:  // 0x2B DS3PTR
        // Pointers are stored as:
        // PPPFF---
        //
        // P = Pointer
        // F = Fractional

        pointer = getDatastreamPointer(stream);
        pointer <<=8;
        pointer &= 0xf0000000;
        pointer |= (value << 20);
        setDatastreamPointer(stream, pointer);
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

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCDF::bank(uInt16 bank)
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
uInt16 CartridgeCDF::getBank() const
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeCDF::bankCount() const
{
  return 7;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCDF::patch(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

  // For now, we ignore attempts to patch the CDF address space
  if(address >= 0x0040)
  {
    myProgramImage[(myCurrentBank << 12) + (address & 0x0FFF)] = value;
    return myBankChanged = true;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeCDF::getImage(int& size) const
{
  size = 32768;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

uInt32 CartridgeCDF::thumbCallback(uInt8 function, uInt32 value1, uInt32 value2)
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
bool CartridgeCDF::save(Serializer& out) const
{
  try
  {
    out.putString(name());

    // Indicates which bank is currently active
    out.putShort(myCurrentBank);

    // Harmony RAM
    out.putByteArray(myCDFRAM, 8192);

    out.putInt(mySystemCycles);
    out.putInt((uInt32)(myFractionalClocks * 100000000.0));
    out.putInt(myARMCycles);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeCDF::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCDF::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    // Indicates which bank is currently active
    myCurrentBank = in.getShort();

    // Harmony RAM
    in.getByteArray(myCDFRAM, 8192);

    // Get system cycles and fractional clocks
    mySystemCycles = (Int32)in.getInt();
    myFractionalClocks = (double)in.getInt() / 100000000.0;

    myARMCycles = (Int32)in.getInt();
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeCDF::load" << endl;
    return false;
  }

  // Now, go to the current bank
  bank(myCurrentBank);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeCDF::getDatastreamPointer(uInt8 index) const
{
  //  index &= 0x0f;

  return myCDFRAM[DSxPTR + index*4 + 0]        +  // low byte
        (myCDFRAM[DSxPTR + index*4 + 1] << 8)  +
        (myCDFRAM[DSxPTR + index*4 + 2] << 16) +
        (myCDFRAM[DSxPTR + index*4 + 3] << 24) ;  // high byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::setDatastreamPointer(uInt8 index, uInt32 value)
{
  //  index &= 0x1f;
  myCDFRAM[DSxPTR + index*4 + 0] = value & 0xff;          // low byte
  myCDFRAM[DSxPTR + index*4 + 1] = (value >> 8) & 0xff;
  myCDFRAM[DSxPTR + index*4 + 2] = (value >> 16) & 0xff;
  myCDFRAM[DSxPTR + index*4 + 3] = (value >> 24) & 0xff;  // high byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeCDF::getDatastreamIncrement(uInt8 index) const
{
  //  index &= 0x1f;
  return myCDFRAM[DSxINC + index*4 + 0]        +   // low byte
        (myCDFRAM[DSxINC + index*4 + 1] << 8)  +
        (myCDFRAM[DSxINC + index*4 + 2] << 16) +
        (myCDFRAM[DSxINC + index*4 + 3] << 24) ;   // high byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::setDatastreamIncrement(uInt8 index, uInt32 value)
{
  //  index &= 0x1f;
  myCDFRAM[DSxINC + index*4 + 0] = value & 0xff;          // low byte
  myCDFRAM[DSxINC + index*4 + 1] = (value >> 8) & 0xff;
  myCDFRAM[DSxINC + index*4 + 2] = (value >> 16) & 0xff;
  myCDFRAM[DSxINC + index*4 + 3] = (value >> 24) & 0xff;  // high byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeCDF::getWaveform(uInt8 index) const
{
  // instead of 0, 1, 2, etc. this returned
  // 0x40000800 for 0
  // 0x40000820 for 1
  // 0x40000840 for 2
  // ...

  //  return myCDFRAM[WAVEFORM + index*4 + 0]        +   // low byte
  //        (myCDFRAM[WAVEFORM + index*4 + 1] << 8)  +
  //        (myCDFRAM[WAVEFORM + index*4 + 2] << 16) +
  //        (myCDFRAM[WAVEFORM + index*4 + 3] << 24) -   // high byte
  //         0x40000800;

  uInt32 result;

  result = myCDFRAM[WAVEFORM + index*4 + 0]        +   // low byte
          (myCDFRAM[WAVEFORM + index*4 + 1] << 8)  +
          (myCDFRAM[WAVEFORM + index*4 + 2] << 16) +
          (myCDFRAM[WAVEFORM + index*4 + 3] << 24);

  result -= 0x40000800;

  if (result >= 4096)
    result = 0;

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeCDF::getWaveformSize(uInt8 index) const
{
  return myMusicWaveformSize[index];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeCDF::readFromDatastream(uInt8 index)
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
