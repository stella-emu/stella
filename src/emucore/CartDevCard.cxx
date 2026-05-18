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

#include "System.hxx"
#include "CartDevCard.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDevCard::CartridgeDevCard(ByteSpan image, string_view md5,
                                   const Settings& settings)
  : Cartridge(settings, md5)
{
  const size_t len = std::min(image.size(), RAM_SIZE);
  myImage.assign(image.data() + RAM_SIZE - len, image.data() + RAM_SIZE);
  createRomAccessArrays(RAM_SIZE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDevCard::reset()
{
  myRAM.fill(0);
  std::copy(myImage.begin(), myImage.end(), myRAM.begin());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDevCard::install(System& system)
{
  mySystem = &system;
  mySystem->setAddressBits(16);

  System::PageAccess access(this, System::PageAccessType::READWRITE);

  for(size_t win = 0; win < NUM_WINDOWS; ++win)
  {
    const uInt32 winBase = static_cast<uInt32>(win * WINDOW_SIZE);
    for(uInt32 addr = WINDOWS[win]; addr < WINDOWS[win] + WINDOW_SIZE;
        addr += System::PAGE_SIZE)
    {
      const uInt32 offset = winBase + (addr - WINDOWS[win]);
      access.directPeekBase = myRAM.data() + offset;
      access.directPokeBase = myRAM.data() + offset;
      access.romAccessBase  = myRomAccessBase.get() + offset;
      access.romPeekCounter = myRomAccessCounter.get() + offset;
      access.romPokeCounter = myRomAccessCounter.get() + offset + RAM_SIZE;
      mySystem->setPageAccess(static_cast<uInt16>(addr), access);
    }
  }
}

//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//uInt16 CartridgeDevCard::bankSize(uInt16 bank) const
//{
//  return static_cast<uInt16>(
//    std::min(getImage().size(), RAM_SIZE)); // assuming that each bank has the same size
//}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDevCard::patch(uInt16 address, uInt8 value)
{
  myRAM[ramOffset(address)] = value;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ByteSpan CartridgeDevCard::getImage() const
{
  return { myImage.data(), myImage.size() };
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDevCard::save(Serializer& out) const
{
  try
  {
    out.putByteArray(myRAM);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeDevCard::save\n";
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDevCard::load(Serializer& in)
{
  try
  {
    in.getByteArray(myRAM);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeDevCard::load\n";
    return false;
  }
  return true;
}

#ifdef DEBUGGER_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeDevCard::bankOrigin(uInt16 /*bank*/, uInt16 PC) const
{
  int win = ((PC >> 12) - 5U) / 2U;
  return WINDOWS[win];  // single bank; origin is the first window
}
#endif
