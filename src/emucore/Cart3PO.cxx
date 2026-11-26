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

#include <cstring>

#include "System.hxx"
#include "Cart3PO.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3PO::Cartridge3PO(ByteSpan image, string_view md5,
                           const Settings& settings)
  : Cartridge3EPlus(image, md5, settings)
{
  // The cart RAM the game asks for in its header in the first 1K: "C3PO",
  // "NB", the size, a build ID (12 bytes), then "RM" and the RAM in 512-byte
  // banks, low byte first.  Without it, 3E+'s 64 banks (32K).  Rounded up to
  // a power of two, as a board's RAM is, and at most 16384 banks (8MB).
  const size_t scan = std::min<size_t>(image.size(), 1_KB);
  for(size_t i = 0; i + 16 <= scan; ++i)
  {
    if(std::memcmp(image.data() + i, "C3PONB", 6) != 0)
      continue;
    if(image[i + 12] == 'R' && image[i + 13] == 'M')
    {
      const uInt32 want = U32(image[i + 14]) | (U32(image[i + 15]) << 8U);
      uInt32 banks = MIN_RAM_BANKS;
      while(banks < std::min(want, MAX_RAM_BANKS))
        banks <<= 1U;
      myRamBankCount = static_cast<uInt16>(banks);
      myRamSize = SZT(banks) << RAM_BANK_SHIFT;
    }
    break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3PO::reset()
{
  // The board must power up with both outer registers at 0, so a 3E+ ROM
  // finds its banks where 3E+ puts them
  myOuter = 0;
  myRamOuter = 0;

  Cartridge3EPlus::reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3PO::checkSwitchBank(uInt16 address, uInt8 value)
{
  // Callers pass the low 13 bits: only exactly $002E, $002F, $003E and $003F
  // switch
  switch(address)
  {
    case 0x002E:
      // The RAM outer bank number, for the next writes to $3E; no slot changes
      if(!hotspotsLocked())
        myRamOuter = value;
      return true;

    case 0x002F:
      // The outer bank number, for the next writes to $3F; no slot changes
      if(!hotspotsLocked())
        myOuter = value;
      return true;

    case 0x003F:
      // ROM bank outer * 64 + D5-D0 into slot D7-D6; past the end wraps
      bank(((U32(myOuter) << 6U) | (value & 0b111111U)) % romBankCount(), value >> 6U);
      return true;

    case 0x003E:
      // RAM bank RAM outer * 64 + D5-D0 into slot D7-D6; past the cart's RAM
      // wraps (the ROM outer isn't used)
      bank(((U32(myRamOuter) << 6U) | (value & 0b111111U)) % ramBankCount()
           + romBankCount(), value >> 6U);
      return true;

    default:
      return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3PO::save(Serializer& out) const
{
  if(!Cartridge3EPlus::save(out))
    return false;

  try
  {
    out.putByte(myOuter);
    out.putByte(myRamOuter);
  }
  catch(...)
  {
    cerr << "ERROR: " << name() << "::save\n";
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3PO::load(Serializer& in)
{
  if(!Cartridge3EPlus::load(in))
    return false;

  try
  {
    myOuter = in.getByte();
    myRamOuter = in.getByte();
  }
  catch(...)
  {
    cerr << "ERROR: " << name() << "::load\n";
    return false;
  }

  return true;
}
