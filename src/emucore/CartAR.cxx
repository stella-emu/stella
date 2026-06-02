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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

// Activate dr_libs implementations in this translation unit only
#define DR_WAV_IMPLEMENTATION
#include "dr_libs/dr_wav_lib.hxx"
#define DR_MP3_IMPLEMENTATION
#include "dr_libs/dr_mp3_lib.hxx"

#include <cmath>
#include <numeric>

#include "M6502.hxx"
#include "System.hxx"
#include "Settings.hxx"
#include "CartAR.hxx"

namespace {
  // Compute the sum of the array of bytes
  constexpr uInt8 checksum(ByteSpan s) {
    return static_cast<uInt8>(std::accumulate(s.begin(), s.end(), 0));
  }
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeAR::CartridgeAR(ByteSpan image, string_view md5,
                         const Settings& settings)
  : Cartridge(settings, md5)
{
  const size_t loadSize = std::max(image.size(), LOAD_SIZE);
  myLoadImages.assign(loadSize, 0);

  // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
  myNumberOfLoadImages = static_cast<uInt8>(myLoadImages.size() / LOAD_SIZE);

  // Copy the given image and add header if not present
  std::ranges::copy(image, myLoadImages.begin());
  if(image.size() < LOAD_SIZE)
    std::ranges::copy(ourDefaultHeader, myLoadImages.begin() + myImage.size());

  // We use System::PageAccess.romAccessBase, but don't allow its use
  // through a pointer, since the AR scheme doesn't support bankswitching
  // in the normal sense
  //
  // Instead, access will be through the getAccessFlags and setAccessFlags
  // methods below
  createRomAccessArrays(myLoadImages.size());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeAR::CartridgeAR(ByteSpan biosImage, vector<float> pcmData,
                         uInt32 sampleRate, string_view md5,
                         const Settings& settings)
  : Cartridge(settings, md5),
    myNumberOfLoadImages{1},
    myPCMData{std::move(pcmData)},
    myPCMSampleRate{sampleRate},
    myPCMSamplesPerCycle{static_cast<double>(sampleRate) / 1190000.0},
    myIsSoundLoad{true}
{
  // Pre-allocate one load image slot so getImage(), the debugger, and the
  // state serialiser all see non-empty data from the start.  RAM bytes are
  // mirrored from myImage as the real BIOS writes pages; the header area
  // starts with ourDefaultHeader and is patched when the tape is exhausted.
  myLoadImages.assign(LOAD_SIZE, 0);
  std::ranges::copy(ourDefaultHeader, myLoadImages.begin() + myImage.size());

  // Access arrays need to cover the full 8K image (6K RAM + 2K ROM)
  createRomAccessArrays(myImage.size());

  // Install the real BIOS into the ROM area
  std::ranges::copy(biosImage, myImage.begin() + RAM_SIZE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeAR::reset()
{
  // Zero the RAM banks; preserve the ROM area (real BIOS in sound-load mode,
  // or rewritten by initializeROM() in fast-load mode)
  std::fill(myImage.begin(), myImage.begin() + RAM_SIZE, 0);

  if(!myIsSoundLoad)
    initializeROM();

  myWriteEnabled = false;
  myPower = true;
  myPCMStarted = false;
  myPCMStartCycle = 0;
  myPCMLoadDelay = 0;

  myDataHoldRegister = 0;
  myNumberOfDistinctAccesses = 0;
  myWritePending = false;

  // Set bank configuration upon reset so ROM is selected and powered up
  bankConfiguration(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeAR::install(System& system)
{
  mySystem = &system;

  // Map all of the accesses to call peek and poke (we don't yet indicate RAM areas)
  const System::PageAccess access(this, System::PageAccessType::READ);
  for(uInt16 addr = 0x1000; addr < 0x2000; addr += System::PAGE_SIZE)
    mySystem->setPageAccess(addr, access);

  bankConfiguration(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeAR::peek(uInt16 addr)
{
  // In debugger/bank-locked mode, we ignore all hotspots and in general
  // anything that can change the internal state of the cart
  if(hotspotsLocked())
    return myImage[(addr & 0x07FF) + myImageOffset[(addr & 0x0800) ? 1 : 0]];

  // In sound-load mode, feed the current PCM bit to the real BIOS via $1FF9.
  //
  // Play-delay phase: return 0x00 (tape active) for the first kLoadDelay reads
  // without advancing the PCM index.  This gives the BIOS time to display
  // "REWIND TAPE / PRESS PLAY" and enter its sync loop before PCM streaming
  // starts from position 0.
  if(myIsSoundLoad && (addr & 0x1FFF) == 0x1FF9)
  {
    if(myPCMData.empty())
      return 0x01;

    constexpr int kLoadDelay = 30000;
    if(myPCMLoadDelay < kLoadDelay)
    {
      ++myPCMLoadDelay;
      return 0x00;  // tape active: tells BIOS the tape is playing
    }

    const uInt64 now = mySystem->cycles();
    if(!myPCMStarted)
    {
      myPCMStarted = true;
      myPCMStartCycle = now;
      cerr << std::format("CartridgeAR: PCM stream started at cycle {}, "
                          "{} samples @ {} Hz\n",
                          myPCMStartCycle, myPCMData.size(), myPCMSampleRate);
    }

    const uInt64 elapsed = now - myPCMStartCycle;
    const double rawIdx = static_cast<double>(elapsed) * myPCMSamplesPerCycle;
    if(rawIdx >= static_cast<double>(myPCMData.size()))
    {
      finalizeSoundLoad();
      return 0x01;
    }
    return (myPCMData[static_cast<size_t>(rawIdx)] >= 0.F) ? 0x01 : 0x00;
  }

  // Fake-BIOS fast-load hotspot (not used in sound-load mode)
  if(!myIsSoundLoad && ((addr & 0x1FFF) == 0x1850) && (myImageOffset[1] == RAM_SIZE))
  {
    // BIOS places load number at 0x80
    loadIntoRAM(mySystem->peek(0x0080));
    return myImage[(addr & 0x07FF) + myImageOffset[1]];
  }

  if(handleHotspot(addr))
    mySystem->setDirtyPage(addr);

  return myImage[(addr & 0x07FF) + myImageOffset[(addr & 0x0800) ? 1 : 0]];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeAR::poke(uInt16 addr, uInt8)
{
  return handleHotspot(addr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeAR::finalizeSoundLoad()
{
  // Synthesise a valid myLoadImages header that maps all 24 pages (6K RAM)
  // in linear bank/page order, matching how writes were mirrored into
  // myLoadImages.  This lets loadIntoRAM() correctly restore myImage if the
  // game ever requests a reload after the tape is exhausted.
  myHeader = {};

  // Try to recover the bank-switch config and start address the real BIOS
  // stored in zero-page RAM; fall back to 0 if unavailable.
  myHeader[0] = mySystem->peek(0x00fe);  // bank-switch byte
  myHeader[1] = mySystem->peek(0x00ff);  // start address lo
  myHeader[2] = mySystem->peek(0x00fd);  // start address hi (convention)

  static constexpr size_t NUM_PAGES = 24;  // 3 banks × 8 pages
  myHeader[3] = static_cast<uInt8>(NUM_PAGES);

  // Page-map: page j in myLoadImages lives at bank (j/8), page (j%8) in bank
  for(size_t j = 0; j < NUM_PAGES; ++j)
    myHeader[16 + j] = static_cast<uInt8>(((j % 8) << 2) | (j / 8));

  // Per-page checksums: must satisfy checksum(data) + map + ck == 0x55
  for(size_t j = 0; j < NUM_PAGES; ++j)
  {
    const ByteSpan src = ByteSpan{myLoadImages}.subspan(j * 256, 256);
    myHeader[64 + j] = static_cast<uInt8>(
      0x55U - checksum(src) - myHeader[16 + j]);
  }

  // Header checksum: first 8 bytes must sum to 0x55; patch byte 7
  const auto partial = static_cast<uInt8>(
    myHeader[0] + myHeader[1] + myHeader[2] + myHeader[3] +
    myHeader[4] + myHeader[5] + myHeader[6]);
  myHeader[7] = static_cast<uInt8>(0x55U - partial);

  // Commit header into myLoadImages
  std::ranges::copy(myHeader, myLoadImages.begin() + myImage.size());

  cerr << std::format("CartridgeAR: PCM exhausted at cycle {}, "
                      "finalising load image\n", mySystem->cycles());

  // Free the (potentially large) PCM buffer; myIsSoundLoad stays true so that
  // the real BIOS remains active and the fake $1850 hotspot stays suppressed.
  // myPCMData.empty() signals the exhausted state to future $1FF9 reads.
  myPCMData.clear();
  myPCMData.shrink_to_fit();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeAR::handleHotspot(uInt16 addr)
{
  // Cancel any pending write if more than 5 distinct accesses have occurred
  if(myWritePending &&
      (mySystem->m6502().distinctAccesses() > myNumberOfDistinctAccesses + 5))
  {
    myWritePending = false;
  }

  // Is the data hold register being set?
  if(!(addr & 0x0F00) && (!myWriteEnabled || !myWritePending))
  {
    myDataHoldRegister = static_cast<uInt8>(addr);
    myNumberOfDistinctAccesses = mySystem->m6502().distinctAccesses();
    myWritePending = true;
  }
  // Is the bank configuration hotspot being accessed?
  else if((addr & 0x1FFF) == 0x1FF8)
  {
    myWritePending = false;
    bankConfiguration(myDataHoldRegister);
  }
  // Commit write if exactly 5 distinct accesses have passed
  else if(myWriteEnabled && myWritePending &&
      (mySystem->m6502().distinctAccesses() == (myNumberOfDistinctAccesses + 5)))
  {
    bool written = false;
    if((addr & 0x0800) == 0)
    {
      const size_t offset = (addr & 0x07FF) + myImageOffset[0];
      myImage[offset] = myDataHoldRegister;
      if(myIsSoundLoad && offset < RAM_SIZE)
        myLoadImages[offset] = myDataHoldRegister;
      written = true;
    }
    else if(myImageOffset[1] != (3 * BANK_SIZE))    // Can't poke to ROM :-)
    {
      const size_t offset = (addr & 0x07FF) + myImageOffset[1];
      myImage[offset] = myDataHoldRegister;
      if(myIsSoundLoad && offset < RAM_SIZE)
        myLoadImages[offset] = myDataHoldRegister;
      written = true;
    }
    myWritePending = false;
    return written;
  }

  return false;
}

#ifdef DEBUGGER_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Device::AccessFlags CartridgeAR::getAccessFlags(uInt16 address) const
{
  return myRomAccessBase[(address & 0x07FF) +
           myImageOffset[(address & 0x0800) ? 1 : 0]];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeAR::setAccessFlags(uInt16 address, Device::AccessFlags flags)
{
  myRomAccessBase[(address & 0x07FF) +
    myImageOffset[(address & 0x0800) ? 1 : 0]] |= flags;
}
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeAR::bankConfiguration(uInt8 configuration)
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
  static constexpr uInt32 OFFSET_0[8] = {
    2 * BANK_SIZE, 0 * BANK_SIZE, 2 * BANK_SIZE, 0 * BANK_SIZE,
    2 * BANK_SIZE, 1 * BANK_SIZE, 2 * BANK_SIZE, 1 * BANK_SIZE
  };
  static constexpr uInt32 OFFSET_1[8] = {
    3 * BANK_SIZE, 3 * BANK_SIZE, 0 * BANK_SIZE, 2 * BANK_SIZE,
    3 * BANK_SIZE, 3 * BANK_SIZE, 1 * BANK_SIZE, 2 * BANK_SIZE
  };
  const int bankConfig = (configuration & 0b11100) >> 2;

  myCurrentBank = configuration & 0b11111; // remember for the bank() method

  // Handle ROM power configuration
  myPower = !(configuration & 0b00001);

  myWriteEnabled = configuration & 0b00010;

  myImageOffset[0] = OFFSET_0[bankConfig];
  myImageOffset[1] = OFFSET_1[bankConfig];

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeAR::initializeROM()
{
  // Note that the following offsets depend on the 'scrom.asm' file
  // in src/tools.  If that file is ever recompiled (and its
  // contents placed in the ourDummyROMCode array), the offsets will
  // almost definitely change

  // The scrom.asm code checks a value at offset 109 as follows:
  //   0xFF -> do a complete jump over the SC BIOS progress bars code
  //   0x00 -> show SC BIOS progress bars as normal
  ourDummyROMCode[109] = mySettings.getBool("fastscbios") ? 0xFF : 0x00;

  // The accumulator should contain a random value after exiting the
  // SC BIOS code - a value placed in offset 281 will be stored in A
  ourDummyROMCode[281] = mySystem->randGenerator().next();

  // Initialize ROM with illegal 6502 opcode that causes a real 6502 to jam
  std::fill_n(myImage.begin() + RAM_SIZE, BANK_SIZE, 0x02);

  // Copy the "dummy" Supercharger BIOS code into the ROM area
  std::ranges::copy(ourDummyROMCode, myImage.begin() + RAM_SIZE);

  // Finally set 6502 vectors to point to initial load code at 0xF80A of BIOS
  myImage[RAM_SIZE + BANK_SIZE - 4] = 0x0A;
  myImage[RAM_SIZE + BANK_SIZE - 3] = 0xF8;
  myImage[RAM_SIZE + BANK_SIZE - 2] = 0x0A;
  myImage[RAM_SIZE + BANK_SIZE - 1] = 0xF8;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeAR::loadIntoRAM(uInt8 load)
{
  bool success = true;

  // Scan through all of the loads to see if we find the one we're looking for
  for(uInt16 image = 0; image < myNumberOfLoadImages; ++image)
  {
    const size_t image_off = image * LOAD_SIZE;

    // Is this the correct load?
    if(myLoadImages[image_off + myImage.size() + 5] == load)
    {
      // Copy the load's header
      std::ranges::copy(ByteSpan{myLoadImages}.subspan(image_off + myImage.size(),
                        myHeader.size()), myHeader.begin());

      // Verify the load's header
      if(checksum(ByteSpan{myHeader}.first(8)) != 0x55)
      {
        myMsgCallback(std::format(
          "Supercharger load #{} done with header checksum error", load));
        success = false;
      }

      // Load all of the pages from the load
      bool invalidPageChecksumSeen = false;
      for(size_t j = 0; j < myHeader[3]; ++j)
      {
        const size_t bank = myHeader[16 + j] & 0b00011;
        const size_t page = (myHeader[16 + j] & 0b11100) >> 2;
        const ByteSpan src = ByteSpan{myLoadImages}.subspan(image_off + j * 256, 256);
        const uInt8 sum = checksum(src) + myHeader[16 + j] + myHeader[64 + j];

        if(!invalidPageChecksumSeen && (sum != 0x55))
        {
          myMsgCallback(std::format(
            "Supercharger load #{} done with page #{} checksum error", load, j));
          invalidPageChecksumSeen = true;
        }

        // Copy page to Supercharger RAM (don't allow a copy into ROM area)
        if(bank < 3)
          std::ranges::copy(src, myImage.begin() + (bank * BANK_SIZE) + (page * 256));
      }
      success &= !invalidPageChecksumSeen;

      // Copy the bank switching byte and starting address into the 2600's
      // RAM for the "dummy" SC BIOS to access it
      mySystem->pokeOob(0xfe, myHeader[0]);
      mySystem->pokeOob(0xff, myHeader[1]);
      mySystem->pokeOob(0x80, myHeader[2]);

      myBankChanged = true;
      if(success)
        myMsgCallback(std::format("Supercharger load #{} done", load));
      return;
    }
  }

  myMsgCallback(std::format("Supercharger load #{} not found in ROM image", load));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeAR::bank(uInt16 bank, uInt16)
{
  if(!hotspotsLocked())
    return bankConfiguration(static_cast<uInt8>(bank));
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeAR::getBank(uInt16) const
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeAR::romBankCount() const
{
  return 32;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeAR::patch(uInt16 address, uInt8 value)
{
  myImage[(address & 0x07FF) + myImageOffset[(address & 0x0800) ? 1 : 0]] = value;
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ByteSpan CartridgeAR::getImage() const
{
  return myLoadImages;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeAR::save(Serializer& out) const
{
  try
  {
    // Indicates the offset within the image for the corresponding bank
    out.putIntArray(myImageOffset);

    // The 6K of RAM and 2K of ROM contained in the Supercharger
    out.putByteArray(myImage);

    // The 256 byte header for the current 8448 byte load
    out.putByteArray(myHeader);

    // Indicates how many 8448 loads there are
    out.putByte(myNumberOfLoadImages);

    // All of the 8448 byte loads associated with the game
    // Note that the size of this array is myNumberOfLoadImages * 8448
    out.putByteArray(ByteSpan{myLoadImages}.first(myNumberOfLoadImages * LOAD_SIZE));

    // Indicates if the RAM is write enabled
    out.putBool(myWriteEnabled);

    // Indicates if the ROM's power is on or off
    out.putBool(myPower);

    // Data hold register used for writing
    out.putByte(myDataHoldRegister);

    // Indicates number of distinct accesses when data hold register was set
    out.putInt(myNumberOfDistinctAccesses);

    // Indicates if a write is pending or not
    out.putBool(myWritePending);

    // Indicates which bank is currently active
    out.putShort(myCurrentBank);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeAR::save\n";
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeAR::load(Serializer& in)
{
  try
  {
    // Indicates the offset within the image for the corresponding bank
    in.getIntArray(myImageOffset);

    // The 6K of RAM and 2K of ROM contained in the Supercharger
    in.getByteArray(myImage);

    // The 256 byte header for the current 8448 byte load
    in.getByteArray(myHeader);

    // Indicates how many 8448 loads there are
    myNumberOfLoadImages = in.getByte();

    // All of the 8448 byte loads associated with the game
    // Note that the size of this array is myNumberOfLoadImages * 8448
    in.getByteArray(ByteMSpan{myLoadImages}.first(myNumberOfLoadImages * LOAD_SIZE));

    // Indicates if the RAM is write enabled
    myWriteEnabled = in.getBool();

    // Indicates if the ROM's power is on or off
    myPower = in.getBool();

    // Data hold register used for writing
    myDataHoldRegister = in.getByte();

    // Indicates number of distinct accesses when data hold register was set
    myNumberOfDistinctAccesses = in.getInt();

    // Indicates if a write is pending or not
    myWritePending = in.getBool();

    // Indicates which bank is currently active
    myCurrentBank = in.getShort();
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeAR::load\n";
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::array<uInt8, 294> CartridgeAR::ourDummyROMCode = {
  0xa5, 0xfa, 0x85, 0x80, 0x4c, 0x18, 0xf8, 0xff,
  0xff, 0xff, 0x78, 0xd8, 0xa0, 0x00, 0xa2, 0x00,
  0x94, 0x00, 0xe8, 0xd0, 0xfb, 0x4c, 0x50, 0xf8,
  0xa2, 0x00, 0xbd, 0x06, 0xf0, 0xad, 0xf8, 0xff,
  0xa2, 0x00, 0xad, 0x00, 0xf0, 0xea, 0xbd, 0x00,
  0xf7, 0xca, 0xd0, 0xf6, 0x4c, 0x50, 0xf8, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xa2, 0x03, 0xbc, 0x22, 0xf9, 0x94, 0xfa, 0xca,
  0x10, 0xf8, 0xa0, 0x00, 0xa2, 0x28, 0x94, 0x04,
  0xca, 0x10, 0xfb, 0xa2, 0x1c, 0x94, 0x81, 0xca,
  0x10, 0xfb, 0xa9, 0xff, 0xc9, 0x00, 0xd0, 0x03,
  0x4c, 0x13, 0xf9, 0xa9, 0x00, 0x85, 0x1b, 0x85,
  0x1c, 0x85, 0x1d, 0x85, 0x1e, 0x85, 0x1f, 0x85,
  0x19, 0x85, 0x1a, 0x85, 0x08, 0x85, 0x01, 0xa9,
  0x10, 0x85, 0x21, 0x85, 0x02, 0xa2, 0x07, 0xca,
  0xca, 0xd0, 0xfd, 0xa9, 0x00, 0x85, 0x20, 0x85,
  0x10, 0x85, 0x11, 0x85, 0x02, 0x85, 0x2a, 0xa9,
  0x05, 0x85, 0x0a, 0xa9, 0xff, 0x85, 0x0d, 0x85,
  0x0e, 0x85, 0x0f, 0x85, 0x84, 0x85, 0x85, 0xa9,
  0xf0, 0x85, 0x83, 0xa9, 0x74, 0x85, 0x09, 0xa9,
  0x0c, 0x85, 0x15, 0xa9, 0x1f, 0x85, 0x17, 0x85,
  0x82, 0xa9, 0x07, 0x85, 0x19, 0xa2, 0x08, 0xa0,
  0x00, 0x85, 0x02, 0x88, 0xd0, 0xfb, 0x85, 0x02,
  0x85, 0x02, 0xa9, 0x02, 0x85, 0x02, 0x85, 0x00,
  0x85, 0x02, 0x85, 0x02, 0x85, 0x02, 0xa9, 0x00,
  0x85, 0x00, 0xca, 0x10, 0xe4, 0x06, 0x83, 0x66,
  0x84, 0x26, 0x85, 0xa5, 0x83, 0x85, 0x0d, 0xa5,
  0x84, 0x85, 0x0e, 0xa5, 0x85, 0x85, 0x0f, 0xa6,
  0x82, 0xca, 0x86, 0x82, 0x86, 0x17, 0xe0, 0x0a,
  0xd0, 0xc3, 0xa9, 0x02, 0x85, 0x01, 0xa2, 0x1c,
  0xa0, 0x00, 0x84, 0x19, 0x84, 0x09, 0x94, 0x81,
  0xca, 0x10, 0xfb, 0xa6, 0x80, 0xdd, 0x00, 0xf0,
  0xa9, 0x9a, 0xa2, 0xff, 0xa0, 0x00, 0x9a, 0x4c,
  0xfa, 0x00, 0xcd, 0xf8, 0xff, 0x4c
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::pair<std::vector<float>, uInt32>
CartridgeAR::loadPCM(const FSNode& file)
{
  ByteArray magic;
  if(file.read(magic, 4) < 4) return {};

  float* buf = nullptr;
  size_t frameCount = 0;
  unsigned int channels = 0, sampleRate = 0;
  bool freeAsWAV = false;

  const string& path = file.getPath();

  if(std::string_view{reinterpret_cast<const char*>(magic.data()), 4} == "RIFF")
  {
    drwav_uint64 fc{};
    buf = drwav_open_file_and_read_pcm_frames_f32(
      path.c_str(), &channels, &sampleRate, &fc, nullptr);
    frameCount = static_cast<size_t>(fc);
    freeAsWAV = true;
    if(!buf)
    {
      cerr << std::format("CartridgeAR: failed to open WAV '{}'\n", path);
      return {};
    }
  }
  else
  {
    const bool isID3  = (magic[0] == 'I' && magic[1] == 'D' && magic[2] == '3');
    const bool isSync = (magic[0] == 0xFF && (magic[1] & 0xE0) == 0xE0);
    if(!isID3 && !isSync)
    {
      cerr << std::format("CartridgeAR: unrecognised audio format in '{}'\n",
                          file.getName());
      return {};
    }
    drmp3_config mp3Cfg{0, 0};
    drmp3_uint64 fc{};
    buf = drmp3_open_file_and_read_pcm_frames_f32(
      path.c_str(), &mp3Cfg, &fc, nullptr);
    channels   = mp3Cfg.channels;
    sampleRate = mp3Cfg.sampleRate;
    frameCount = static_cast<size_t>(fc);
    if(!buf)
    {
      cerr << std::format("CartridgeAR: failed to open MP3 '{}'\n", path);
      return {};
    }
  }

  if(frameCount == 0 || channels == 0)
  {
    if(freeAsWAV) drwav_free(buf, nullptr);
    else          drmp3_free(buf, nullptr);
    return {};
  }

  if(channels > 1)
  {
    const float scale = 1.F / static_cast<float>(channels);
    for(size_t i = 0; i < frameCount; ++i)
    {
      float sum = 0.F;
      for(uInt32 ch = 0; ch < channels; ++ch)
        sum += buf[i * channels + ch];
      buf[i] = sum * scale;
    }
  }

  // DC removal only: subtract the mean so the signal is centred.
  // We do NOT normalise or clamp here; instead we compute the post-DC mean
  // (which will be 0 after subtraction, so threshold = 0 suffices).
  // For MP3 files the encoder delay creates a large initial negative burst;
  // using the play-delay in peek() means the PCM only starts streaming once
  // the BIOS is ready, so the burst is presented during a time when the BIOS
  // treats any "active" reading as part of the expected leader lead-in.
  conditionSignal({buf, frameCount});

  std::vector<float> result(buf, buf + frameCount);

  if(freeAsWAV) drwav_free(buf, nullptr);
  else          drmp3_free(buf, nullptr);

  return {std::move(result), static_cast<uInt32>(sampleRate)};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeAR::conditionSignal(FloatMSpan samples)
{
  // DC removal only: subtract the mean so the signal is centred around 0.
  // After this, threshold = 0 (sample >= 0 → tape silent/high → bit 1).
  if(samples.empty()) return;
  const float mean = std::reduce(samples.begin(), samples.end()) /
                     static_cast<float>(samples.size());
  std::ranges::for_each(samples, [mean](float& s) { s -= mean; });
}
