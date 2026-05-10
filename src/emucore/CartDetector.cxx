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

#include "bspf.hxx"
#include "Logger.hxx"

#include "ElfParser.hxx"
#include "CartDetector.hxx"
#include "CartMVC.hxx"

using BSPF::searchForBytes;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Bankswitch::Type CartDetector::autodetectType(ByteSpan image)
{
  // Guess type based on size
  Bankswitch::Type type = Bankswitch::Type::AUTO;

  if(isProbablyELF(image))
  {
    type = Bankswitch::Type::ELF;
  }
  else if((image.size() % 8448) == 0 || image.size() == 6_KB)
  {
    if(image.size() == 6_KB && isProbablyGL(image))
      type = Bankswitch::Type::GL;
    else
      type = Bankswitch::Type::AR;
  }
  else if((image.size() <= 2_KB) ||
          (image.size() == 4_KB && std::memcmp(image.data(), image.data() + 2_KB, 2_KB) == 0))
  {
    type = isProbablyCV(image) ? Bankswitch::Type::CV : Bankswitch::Type::_2K;
  }
  else if(image.size() == 4_KB)
  {
    if(     isProbablyCV(image))    type = Bankswitch::Type::CV;
    else if(isProbably4KSC(image))  type = Bankswitch::Type::_4KSC;
    else if(isProbablyFC(image))    type = Bankswitch::Type::FC;
    else if(isProbablyGL(image))    type = Bankswitch::Type::GL;
    else                            type = Bankswitch::Type::_4K;
  }
  else if(image.size() == 8_KB)
  {
    // First check for *potential* F8
    static constexpr BSPF::array2D<uInt8, 2, 3> f8sig = {{
      { 0x8D, 0xF9, 0x1F },  // STA $1FF9
      { 0x8D, 0xF9, 0xFF }   // STA $FFF9
    }};
    const bool f8 = std::ranges::any_of(f8sig, [&](const auto& sig) {
      return searchForBytes(image, sig, 2);
    });

    if(     isProbablySC(image))          type = Bankswitch::Type::F8SC;
    else if(std::memcmp(image.data(),
        image.data() + 4_KB, 4_KB) == 0)  type = Bankswitch::Type::_4K;
    else if(isProbablyE0(image))          type = Bankswitch::Type::E0;
    else if(isProbably3EX(image))         type = Bankswitch::Type::_3EX;
    else if(isProbably3E(image))          type = Bankswitch::Type::_3E;
    else if(isProbably3F(image))          type = Bankswitch::Type::_3F;
    else if(isProbablyUA(image))          type = Bankswitch::Type::UA;
    else if(isProbably0FA0(image))        type = Bankswitch::Type::_0FA0;
    else if(isProbablyFE(image) && !f8)   type = Bankswitch::Type::FE;
    else if(isProbably0840(image))        type = Bankswitch::Type::_0840;
    else if(isProbablyE78K(image))        type = Bankswitch::Type::E7;
    else if(isProbablyWD(image))          type = Bankswitch::Type::WD;
    else if(isProbablyFC(image))          type = Bankswitch::Type::FC;
    else if(isProbably03E0(image))        type = Bankswitch::Type::_03E0;
    else                                  type = Bankswitch::Type::F8;
  }
  else if(image.size() == 8_KB + 3)  // 8195 bytes (Experimental)
  {
    type = Bankswitch::Type::WDSW;
  }
  else if(image.size() >= 10_KB && image.size() <= 10_KB + 256)  // ~10K - Pitfall2
  {
    type = Bankswitch::Type::DPC;
  }
  else if(image.size() == 12_KB)
  {
    if(isProbablyE7(image))  type = Bankswitch::Type::E7;
    else                         type = Bankswitch::Type::FA;
  }
  else if(image.size() == 16_KB)
  {
    if(     isProbablySC(image))    type = Bankswitch::Type::F6SC;
    else if(isProbablyE7(image))    type = Bankswitch::Type::E7;
    else if(isProbablyFC(image))    type = Bankswitch::Type::FC;
    else if(isProbably3EX(image))   type = Bankswitch::Type::_3EX;
    else if(isProbably3E(image))    type = Bankswitch::Type::_3E;
  /* no known 16K 3F ROMS
    else if(isProbably3F(image))    type = Bankswitch::Type::3F;
  */
    else if(isProbablyJANE(image))  type = Bankswitch::Type::JANE;
    else                            type = Bankswitch::Type::F6;
  }
  else if(image.size() == 24_KB || image.size() == 28_KB)
  {
    type = Bankswitch::Type::FA2;
  }
  else if(image.size() == 29_KB)
  {
    if(isProbablyARM(image))              type = Bankswitch::Type::FA2;
    else /*if(isProbablyDPCplus(image))*/ type = Bankswitch::Type::DPCP;
  }
  else if(image.size() == 32_KB)
  {
    if(     isProbablyCTY(image))      type = Bankswitch::Type::CTY;
    else if(isProbablyCDF(image))      type = Bankswitch::Type::CDF;
    else if(isProbablyDPCplus(image))  type = Bankswitch::Type::DPCP;
    else if(isProbablySC(image))       type = Bankswitch::Type::F4SC;
    else if(isProbably3EX(image))      type = Bankswitch::Type::_3EX;
    else if(isProbably3E(image))       type = Bankswitch::Type::_3E;
    else if(isProbably3F(image))       type = Bankswitch::Type::_3F;
    else if(isProbablyBUS(image))      type = Bankswitch::Type::BUS;
    else if(isProbablyFA2(image))      type = Bankswitch::Type::FA2;
    else if(isProbablyFC(image))       type = Bankswitch::Type::FC;
    else                               type = Bankswitch::Type::F4;
  }
  else if(image.size() == 60_KB)
  {
    if(isProbablyCTY(image))  type = Bankswitch::Type::CTY;
    else                      type = Bankswitch::Type::F4;
  }
  else if(image.size() == 64_KB)
  {
    if(     isProbablyEFF(image))       type = Bankswitch::Type::EFF;
    else if(isProbablyCDF(image))       type = Bankswitch::Type::CDF;
    else if(isProbably3EX(image))       type = Bankswitch::Type::_3EX;
    else if(isProbably3E(image))        type = Bankswitch::Type::_3E;
    else if(isProbably3F(image))        type = Bankswitch::Type::_3F;
    else if(isProbably4A50(image))      type = Bankswitch::Type::_4A50;
    else if(isProbablyEF(image, type))  ; // type has been set directly in the function
    else if(isProbablyX07(image))       type = Bankswitch::Type::X07;
    else                                type = Bankswitch::Type::F0;
  }
  else if(image.size() == 128_KB)
  {
    if(     isProbablyCDF(image))       type = Bankswitch::Type::CDF;
    else if(isProbably3EX(image))       type = Bankswitch::Type::_3EX;
    else if(isProbably3E(image))        type = Bankswitch::Type::_3E;
    else if(isProbablyDF(image, type))  ; // type has been set directly in the function
    else if(isProbably3F(image))        type = Bankswitch::Type::_3F;
    else if(isProbably4A50(image))      type = Bankswitch::Type::_4A50;
    else /*if(isProbablySB(image))*/    type = Bankswitch::Type::SB;
  }
  else if(image.size() == 256_KB)
  {
    if(     isProbablyCDF(image))       type = Bankswitch::Type::CDF;
    else if(isProbably3EX(image))       type = Bankswitch::Type::_3EX;
    else if(isProbably3E(image))        type = Bankswitch::Type::_3E;
    else if(isProbablyBF(image, type))  ; // type has been set directly in the function
    else if(isProbably3F(image))        type = Bankswitch::Type::_3F;
    else /*if(isProbablySB(image))*/    type = Bankswitch::Type::SB;
  }
  else if(image.size() == 512_KB)
  {
    if(     isProbablyTVBoy(image))  type = Bankswitch::Type::TVBOY;
    else if(isProbablyCDF(image))    type = Bankswitch::Type::CDF;
    else if(isProbably3EX(image))    type = Bankswitch::Type::_3EX;
    else if(isProbably3E(image))     type = Bankswitch::Type::_3E;
    else if(isProbably3F(image))     type = Bankswitch::Type::_3F;
  }
  else  // what else can we do?
  {
    if(     isProbably3EX(image))  type = Bankswitch::Type::_3EX;
    else if(isProbably3E(image))   type = Bankswitch::Type::_3E;
    else if(isProbably3F(image))   type = Bankswitch::Type::_3F;
  }

  // Variable image.size()d ROM formats are independent of image image.size() and come last
  if(     isProbably3EPlus(image))  type = Bankswitch::Type::_3EP;
  else if(isProbablyMDM(image))     type = Bankswitch::Type::MDM;
  else if(isProbablyMVC(image))     type = Bankswitch::Type::MVC;

  // If we get here and autodetection failed, then we force '4K'
  if(type == Bankswitch::Type::AUTO)
    type = Bankswitch::Type::_4K;  // Most common bankswitching type

  Logger::debug(std::format("Bankswitching type '{}' detected",
                            Bankswitch::typeToDesc(type)));

  return type;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablySC(ByteSpan image)
{
  // Superchip cart repeats the first 128 bytes for the second 128 bytes
  // in the RAM area, which is the first 256 bytes of each 4K bank
  const uInt8* ptr = image.data();
  size_t size = image.size();
  while(size)
  {
    if(std::memcmp(ptr, ptr + 128, 128) != 0)
      return false;

    ptr  += 4_KB;
    size -= 4_KB;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyARM(ByteSpan image)
{
  // ARM code contains the following 'loader' patterns in the first 1K
  // Thanks to Thomas Jentzsch of AtariAge for this advice
  static constexpr BSPF::array2D<uInt8, 2, 4> signature = {{
    { 0xA0, 0xC1, 0x1F, 0xE0 },
    { 0x00, 0x80, 0x02, 0xE0 }
  }};
  const auto region = image.first(std::min<size_t>(image.size(), 1_KB));
  return std::ranges::any_of(signature, [&](const auto& sig) {
    return searchForBytes(region, sig);
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbably03E0(ByteSpan image)
{
  // 03E0 cart bankswitching for Brazilian Parker Bros ROMs, switches segment
  // 0 into bank 0 by accessing address 0x3E0 using 'LDA $3E0' or 'ORA $3E0'.
  static constexpr BSPF::array2D<uInt8, 2, 4> signature = {{
    { 0x0D, 0xE0, 0x03, 0x0D },  // ORA $3E0, ORA (Popeye)
    { 0xAD, 0xE0, 0x03, 0xAD }   // LDA $3E0, ORA (Montezuma's Revenge)
  }};
  return std::ranges::any_of(signature, [&](const auto& sig) {
    return searchForBytes(image, sig);
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbably0840(ByteSpan image)
{
  // 0840 cart bankswitching is triggered by accessing addresses 0x0800
  // or 0x0840 at least twice
  static constexpr BSPF::array2D<uInt8, 3, 3> signature1 = {{
    { 0xAD, 0x00, 0x08 },  // LDA $0800
    { 0xAD, 0x40, 0x08 },  // LDA $0840
    { 0x2C, 0x00, 0x08 }   // BIT $0800
  }};
  if(std::ranges::any_of(signature1, [&](const auto& sig) {
    return searchForBytes(image, sig, 2);
  }))
    return true;

  static constexpr BSPF::array2D<uInt8, 2, 4> signature2 = {{
    { 0x0C, 0x00, 0x08, 0x4C },  // NOP $0800; JMP ...
    { 0x0C, 0xFF, 0x0F, 0x4C }   // NOP $0FFF; JMP ...
  }};
  return std::ranges::any_of(signature2, [&](const auto& sig) {
    return searchForBytes(image, sig, 2);
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbably0FA0(ByteSpan image)
{
  // Other Brazilian (Fotomania) ROM's bankswitching switches to bank 1 by
  // accessing address 0xFC0 using 'BIT $FC0', 'BIT $FC0' or 'STA $FC0'
  // Also a game (Motocross) using 'BIT $EFC0' has been found
  static constexpr BSPF::array2D<uInt8, 4, 3> signature = {{
    { 0x2C, 0xC0, 0x0F },  // BIT $FC0  (H.E.R.O., Kung-Fu Master)
    { 0x8D, 0xC0, 0x0F },  // STA $FC0  (Pole Position, Subterranea)
    { 0xAD, 0xC0, 0x0F },  // LDA $FC0  (Front Line, Zaxxon)
    { 0x2C, 0xC0, 0xEF }   // BIT $EFC0 (Motocross)
  }};
  return std::ranges::any_of(signature, [&](const auto& sig) {
    return searchForBytes(image, sig);
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbably3E(ByteSpan image)
{
  // 3E cart RAM bankswitching is triggered by storing the bank number
  // in address 3E using 'STA $3E', ROM bankswitching is triggered by
  // storing the bank number in address 3F using 'STA $3F'.
  // We expect the latter will be present at least 2 times, since there
  // are at least two banks
  static constexpr std::array<uInt8, 2> signature1 = { 0x85, 0x3E };  // STA $3E
  static constexpr std::array<uInt8, 2> signature2 = { 0x85, 0x3F };  // STA $3F
  return searchForBytes(image, signature1) &&
         searchForBytes(image, signature2, 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbably3EX(ByteSpan image)
{
  // 3EX cart have at least 2 occurrences of the string "3EX"
  static constexpr std::array<uInt8, 3> sig = { '3', 'E', 'X' };
  return searchForBytes(image, sig, 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbably3EPlus(ByteSpan image)
{
  // 3E+ cart is identified key 'TJ3E' in the ROM
  static constexpr std::array<uInt8, 4> sig = { 'T', 'J', '3', 'E' };
  return searchForBytes(image, sig);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbably3F(ByteSpan image)
{
  // 3F cart bankswitching is triggered by storing the bank number
  // in address 3F using 'STA $3F'
  // We expect it will be present at least 2 times, since there are
  // at least two banks
  static constexpr std::array<uInt8, 2> signature = { 0x85, 0x3F };  // STA $3F
  return searchForBytes(image, signature, 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbably4A50(ByteSpan image)
{
  // 4A50 carts store address $4A50 at the NMI vector, which
  // in this scheme is always in the last page of ROM at
  // $1FFA - $1FFB (at least this is true in rev 1 of the format)
  if(image[image.size()-6] == 0x50 && image[image.size()-5] == 0x4A)
    return true;

  // Program starts at $1Fxx with NOP $6Exx or NOP $6Fxx?
  if(((image[0xfffd] & 0x1f) == 0x1f) &&
      (image[image[0xfffd] * 256 + image[0xfffc]] == 0x0c) &&
      ((image[image[0xfffd] * 256 + image[0xfffc] + 2] & 0xfe) == 0x6e))
    return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbably4KSC(ByteSpan image)
{
  // We check if the first 256 bytes are identical *and* if there's
  // an "SC" signature for one of our larger SC types at 1FFA.
  const uInt8 first = image[0];
  for(uInt32 i = 1; i < 256; ++i)
    if(image[i] != first)
      return false;

  return (image[image.size()-6] == 'S') && (image[image.size()-5] == 'C');
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyBF(ByteSpan image, Bankswitch::Type& type)
{
  // BF carts store strings 'BFBF' and 'BFSC' starting at address $FFF8
  // This signature is attributed to "RevEng" of AtariAge
  static constexpr std::array<uInt8, 4> bf   = { 'B', 'F', 'B', 'F' };
  static constexpr std::array<uInt8, 4> bfsc = { 'B', 'F', 'S', 'C' };
  const auto tail = image.last(8);
  if(searchForBytes(tail, bf))
  {
    type = Bankswitch::Type::BF;
    return true;
  }
  else if(searchForBytes(tail, bfsc))
  {
    type = Bankswitch::Type::BFSC;
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyBUS(ByteSpan image)
{
  // BUS ARM code has 2 occurrences of the string BUS
  // Note: all Harmony/Melody custom drivers also contain the value
  // 0x10adab1e (LOADABLE) if needed for future improvement
  static constexpr std::array<uInt8, 3> bus = { 'B', 'U', 'S' };
  return searchForBytes(image, bus, 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyCDF(ByteSpan image)
{
  // CDF ARM code has 3 occurrences of the string CDF
  // Note: all Harmony/Melody custom drivers also contain the value
  // 0x10adab1e (LOADABLE) if needed for future improvement
  static constexpr std::array<uInt8, 3> cdf      = { 'C', 'D', 'F' };
  static constexpr std::array<uInt8, 8> cdfjplus = { 'P', 'L', 'U', 'S', 'C', 'D', 'F', 'J' };
  return searchForBytes(image, cdf, 3) || searchForBytes(image, cdfjplus);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyCTY(ByteSpan image)
{
  static constexpr std::array<uInt8, 5> lenin = { 'L', 'E', 'N', 'I', 'N' };
  return searchForBytes(image, lenin);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyCV(ByteSpan image)
{
  // CV RAM access occurs at addresses $f3ff and $f400
  // These signatures are attributed to the MESS project
  static constexpr BSPF::array2D<uInt8, 2, 3> signature = {{
    { 0x9D, 0xFF, 0xF3 },  // STA $F3FF,X  MagiCard
    { 0x99, 0x00, 0xF4 }   // STA $F400,Y  Video Life
  }};
  return std::ranges::any_of(signature, [&](const auto& sig) {
    return searchForBytes(image, sig);
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyDF(ByteSpan image, Bankswitch::Type& type)
{
  // DF carts store strings 'DFDF' and 'DFSC' starting at address $FFF8
  // This signature is attributed to "RevEng" of AtariAge
  static constexpr std::array<uInt8, 4> df   = { 'D', 'F', 'D', 'F' };
  static constexpr std::array<uInt8, 4> dfsc = { 'D', 'F', 'S', 'C' };
  const auto tail = image.last(8);
  if(searchForBytes(tail, df))
  {
    type = Bankswitch::Type::DF;
    return true;
  }
  else if(searchForBytes(tail, dfsc))
  {
    type = Bankswitch::Type::DFSC;
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyDPCplus(ByteSpan image)
{
  // DPC+ ARM code has 2 occurrences of the string DPC+
  // Note: all Harmony/Melody custom drivers also contain the value
  // 0x10adab1e (LOADABLE) if needed for future improvement
  static constexpr std::array<uInt8, 4> dpcp = { 'D', 'P', 'C', '+' };
  return searchForBytes(image, dpcp, 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyE0(ByteSpan image)
{
  // E0 cart bankswitching is triggered by accessing addresses
  // $FE0 to $FF9 using absolute non-indexed addressing
  // To eliminate false positives (and speed up processing), we
  // search for only certain known signatures
  // Thanks to "stella@casperkitty.com" for this advice
  // These signatures are attributed to the MESS project
  static constexpr BSPF::array2D<uInt8, 8, 3> signature = {{
    { 0x8D, 0xE0, 0x1F },  // STA $1FE0
    { 0x8D, 0xE0, 0x5F },  // STA $5FE0
    { 0x8D, 0xE9, 0xFF },  // STA $FFE9
    { 0x0C, 0xE0, 0x1F },  // NOP $1FE0
    { 0xAD, 0xE0, 0x1F },  // LDA $1FE0
    { 0xAD, 0xE9, 0xFF },  // LDA $FFE9
    { 0xAD, 0xED, 0xFF },  // LDA $FFED
    { 0xAD, 0xF3, 0xBF }   // LDA $BFF3
  }};
  return std::ranges::any_of(signature, [&](const auto& sig) {
    return searchForBytes(image, sig);
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyE7(ByteSpan image)
{
  // E7 cart bankswitching is triggered by accessing addresses
  // $FE0 to $FE6 using absolute non-indexed addressing
  // To eliminate false positives (and speed up processing), we
  // search for only certain known signatures
  // Thanks to "stella@casperkitty.com" for this advice
  // These signatures are attributed to the MESS project
  static constexpr BSPF::array2D<uInt8, 7, 3> signature = {{
    { 0xAD, 0xE2, 0xFF },  // LDA $FFE2
    { 0xAD, 0xE5, 0xFF },  // LDA $FFE5
    { 0xAD, 0xE5, 0x1F },  // LDA $1FE5
    { 0xAD, 0xE7, 0x1F },  // LDA $1FE7
    { 0x0C, 0xE7, 0x1F },  // NOP $1FE7
    { 0x8D, 0xE7, 0xFF },  // STA $FFE7
    { 0x8D, 0xE7, 0x1F }   // STA $1FE7
  }};
  return std::ranges::any_of(signature, [&](const auto& sig) {
    return searchForBytes(image, sig);
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyE78K(ByteSpan image)
{
  // E78K cart bankswitching is triggered by accessing addresses
  // $FE4 to $FE6 using absolute non-indexed addressing
  // To eliminate false positives (and speed up processing), we
  // search for only certain known signatures
  static constexpr BSPF::array2D<uInt8, 3, 3> signature = {{
    { 0xAD, 0xE4, 0xFF },  // LDA $FFE4
    { 0xAD, 0xE5, 0xFF },  // LDA $FFE5
    { 0xAD, 0xE6, 0xFF },  // LDA $FFE6
  }};
  return std::ranges::any_of(signature, [&](const auto& sig) {
    return searchForBytes(image, sig);
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyEF(ByteSpan image, Bankswitch::Type& type)
{
  // Newer EF carts store strings 'EFEF' and 'EFSC' starting at address $FFF8
  // This signature is attributed to "RevEng" of AtariAge
  static constexpr std::array<uInt8, 4> efef = { 'E', 'F', 'E', 'F' };
  static constexpr std::array<uInt8, 4> efsc = { 'E', 'F', 'S', 'C' };
  const auto tail = image.last(8);
  if(searchForBytes(tail, efef))
  {
    type = Bankswitch::Type::EF;
    return true;
  }
  else if(searchForBytes(tail, efsc))
  {
    type = Bankswitch::Type::EFSC;
    return true;
  }

  // Otherwise, EF cart bankswitching switches banks by accessing addresses
  // 0xFE0 to 0xFEF, usually with either a NOP or LDA
  // It's likely that the code will switch to bank 0, so that's what is tested
  static constexpr BSPF::array2D<uInt8, 4, 3> signature = {{
    { 0x0C, 0xE0, 0xFF },  // NOP $FFE0
    { 0xAD, 0xE0, 0xFF },  // LDA $FFE0
    { 0x0C, 0xE0, 0x1F },  // NOP $1FE0
    { 0xAD, 0xE0, 0x1F }   // LDA $1FE0
  }};
  const bool isEF = std::ranges::any_of(signature, [&](const auto& sig) {
    return searchForBytes(image, sig);
  });

  // Now that we know that the ROM is EF, we need to check if it's
  // the SC variant
  if(isEF)
  {
    type = isProbablySC(image) ? Bankswitch::Type::EFSC : Bankswitch::Type::EF;
    return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyEFF(ByteSpan image)
{
  static constexpr std::array<uInt8, 4> effb = { 'E', 'F', 'F', 'B' };
  if(searchForBytes(image.last(8), effb))
    return true;

  // Otherwise, EFF cart bankswitching switches banks by accessing
  // addresses 0xFE0 to 0xFEF, usually with a NOP, as well as the 0xFF0
  // to 0xFF3 for save-to-cart and read from 0xFF4 (for the EEPROM).
  // Unlike EF, we require to find all three.
  static constexpr BSPF::array2D<uInt8, 3, 3> signature = {{
    { 0x0C, 0xE0, 0xFF },  // NOP $FFE0
    { 0x0C, 0xF0, 0x1F },  // NOP $1FF0
    { 0xAD, 0xF4, 0x1F }   // LDA $1FF4
  }};
  return std::ranges::all_of(signature, [&](const auto& sig) {
    return searchForBytes(image, sig);
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyFA2(ByteSpan image)
{
  // This currently tests only the 32K version of FA2; the 24 and 28K
  // versions are easy, in that they're the only possibility with those
  // file sizes

  // 32K version has all zeros in 29K-32K area
  for(size_t i = 29_KB; i < 32_KB; ++i)
    if(image[i] != 0)
      return false;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyFC(ByteSpan image)
{
  // FC bankswitching uses consecutive writes to 3 hotspots
  static constexpr BSPF::array2D<uInt8, 3, 6> signature = {{
    { 0x8d, 0xf8, 0x1f, 0x4a, 0x4a, 0x8d },  // STA $1FF8, LSR, LSR, STA... Power Play Arcade Menus, 3-D Ghost Attack
    { 0x8d, 0xf8, 0xff, 0x8d, 0xfc, 0xff },  // STA $FFF8, STA $FFFC        Surf's Up (4K)
    { 0x8c, 0xf9, 0xff, 0xad, 0xfc, 0xff }   // STY $FFF9, LDA $FFFC        3-D Havoc
  }};
  return std::ranges::any_of(signature, [&](const auto& sig) {
    return searchForBytes(image, sig);
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyFE(ByteSpan image)
{
  // FE bankswitching is very weird, but always seems to include a
  // 'JSR $xxxx'
  // These signatures are (mostly) attributed to the MESS project
  static constexpr BSPF::array2D<uInt8, 5, 5> signature = {{
    { 0x20, 0x00, 0xD0, 0xC6, 0xC5 },  // JSR $D000; DEC $C5  Decathlon
    { 0x20, 0xC3, 0xF8, 0xA5, 0x82 },  // JSR $F8C3; LDA $82  Robot Tank
    { 0xD0, 0xFB, 0x20, 0x73, 0xFE },  // BNE $FB; JSR $FE73  Space Shuttle (NTSC/PAL)
    { 0xD0, 0xFB, 0x20, 0x68, 0xFE },  // BNE $FB; JSR $FE68  Space Shuttle (SECAM)
    { 0x20, 0x00, 0xF0, 0x84, 0xD6 }   // JSR $F000; STY $D6  Thwocker
  }};
  return std::ranges::any_of(signature, [&](const auto& sig) {
    return searchForBytes(image, sig);
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyJANE(ByteSpan image)
{
  static constexpr std::array<uInt8, 4> signature = { 0xad, 0xf1, 0xff, 0x60 };  // LDA $0CB8
  return searchForBytes(image, signature);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyGL(ByteSpan image)
{
  static constexpr std::array<uInt8, 3> signature = { 0xad, 0xb8, 0x0c };  // LDA $0CB8
  return searchForBytes(image, signature);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyMDM(ByteSpan image)
{
  // MDM cart is identified key 'MDMC' in the first 8K of ROM
  static constexpr std::array<uInt8, 4> mdmc = { 'M', 'D', 'M', 'C' };
  return searchForBytes(image.first(std::min<size_t>(image.size(), 8_KB)), mdmc);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyMVC(ByteSpan image)
{
  // MVC version 0
  static constexpr std::array<uInt8, 4> sig = { 'M', 'V', 'C', 0 };
  return searchForBytes(image.first(std::min<size_t>(image.size(), sig.size() + 1)), sig);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t CartDetector::isProbablyMVC(const FSNode& rom)
{
  constexpr size_t frameSize = 2 * CartridgeMVC::MVC_FIELD_SIZE;

  if(Bankswitch::typeFromExtension(rom) == Bankswitch::Type::MVC)
    return frameSize;

  // TODO: Maybe we can determine whether a ROM is MVC before opening it
  //       Perhaps based on size??  This call is made for every attempt
  //       to open _any_ ROM, and most of the time they won't be MVC
  Serializer s(rom.getPath(), Serializer::FileMode::ReadOnly);
  if(s)
  {
    if(s.size() < frameSize)
      return 0;

    uInt8 image[frameSize];
    s.getByteArray(image);

    static constexpr std::array<uInt8, 4> sig = { 'M', 'V', 'C', 0 };  // MVC version 0
    return searchForBytes({image, frameSize}, sig) ? frameSize : 0;
  }
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablySB(ByteSpan image)
{
  // SB cart bankswitching switches banks by accessing address 0x0800
  static constexpr BSPF::array2D<uInt8, 2, 3> signature = {{
    { 0xBD, 0x00, 0x08 },  // LDA $0800,x
    { 0xAD, 0x00, 0x08 }   // LDA $0800
  }};
  return std::ranges::any_of(signature, [&](const auto& sig) {
    return searchForBytes(image, sig);
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyTVBoy(ByteSpan image)
{
  // TV Boy cart bankswitching switches banks by accessing addresses 0x1800..$187F
  static constexpr std::array<uInt8, 5> sig = { 0x91, 0x82, 0x6c, 0xfc, 0xff };  // STA ($82),Y; JMP ($FFFC)
  return searchForBytes(image, sig);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyUA(ByteSpan image)
{
  // UA cart bankswitching switches to bank 1 by accessing address 0x240
  // using 'STA $240' or 'LDA $240'.
  // Brazilian (Digivison) cart bankswitching switches to bank 1 by accessing address 0x2C0
  // using 'BIT $2C0', 'STA $2C0' or 'LDA $2C0'
  static constexpr BSPF::array2D<uInt8, 6, 3> signature = {{
    { 0x8D, 0x40, 0x02 },  // STA $240 (Funky Fish, Pleiades)
    { 0xAD, 0x40, 0x02 },  // LDA $240 (???)
    { 0xBD, 0x1F, 0x02 },  // LDA $21F,X (Gingerbread Man)
    { 0x2C, 0xC0, 0x02 },  // BIT $2C0 (Time Pilot)
    { 0x8D, 0xC0, 0x02 },  // STA $2C0 (Fathom, Vanguard)
    { 0xAD, 0xC0, 0x02 },  // LDA $2C0 (Mickey)
  }};
  return std::ranges::any_of(signature, [&](const auto& sig) {
    return searchForBytes(image, sig);
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyWD(ByteSpan image)
{
  // WD cart bankswitching switches banks by accessing address 0x30..0x3f
  static constexpr std::array<uInt8, 3> signature = { 0xA5, 0x39, 0x4C };  // LDA $39, JMP
  return searchForBytes(image, signature);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyX07(ByteSpan image)
{
  // X07 bankswitching switches to bank 0, 1, 2, etc by accessing address 0x08xd
  static constexpr BSPF::array2D<uInt8, 6, 3> signature = {{
    { 0xAD, 0x0D, 0x08 },  // LDA $080D
    { 0xAD, 0x1D, 0x08 },  // LDA $081D
    { 0xAD, 0x2D, 0x08 },  // LDA $082D
    { 0x0C, 0x0D, 0x08 },  // NOP $080D
    { 0x0C, 0x1D, 0x08 },  // NOP $081D
    { 0x0C, 0x2D, 0x08 }   // NOP $082D
  }};
  return std::ranges::any_of(signature, [&](const auto& sig) {
    return searchForBytes(image, sig);
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyELF(ByteSpan image)
{
  // Min ELF header size
  if(image.size() < 52) return false;

  // Must start with ELF magic
  static constexpr std::array<uInt8, 4> signature = { 0x7f, 'E', 'L', 'F' };
  if(!searchForBytes(image.first(2 * signature.size()), signature))
    return false;

  // We require little endian
  if(image[0x05] != ElfParser::ENDIAN_LITTLE_ENDIAN) return false;

  // Type must be ET_REL (relocatable ELF)
  if(image[0x10] != ElfParser::ET_REL)               return false;

  // Arch must be ARM
  if(image[0x12] != ElfParser::ARCH_ARM32)           return false;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyPlusROM(ByteSpan image)
{
  // PlusCart uses this pattern to detect a PlusROM
  static constexpr std::array<uInt8, 3> signature = { 0x8d, 0xf1, 0x1f };  // STA $1FF1
  return searchForBytes(image, signature);
}
