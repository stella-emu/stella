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
// Copyright (c) 1995-2024 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef ELF_ENVIRONMENT
#define ELF_ENVIRONMENT

#include "bspf.hxx"
#include "ElfLinker.hxx"

namespace elfEnvironment {
  constexpr uInt32 ADDR_TEXT_BASE = 0x00100000;
  constexpr uInt32 ADDR_DATA_BASE = 0x00200000;
  constexpr uInt32 ADDR_RODATA_BASE = 0x00300000;
  constexpr uInt32 ADDR_TABLES_BASE = 0x00400000;

  constexpr uInt32 ADDR_ADDR_IDR = 0xf0000000;
  constexpr uInt32 ADDR_DATA_IDR = 0xf0000004;
  constexpr uInt32 ADDR_DATA_ODR = 0xf0000008;
  constexpr uInt32 ADDR_DATA_MODER = 0xf0000010;

  constexpr uInt32 STUB_BASE = 0x1001;

  constexpr uInt32 ADDR_MEMSET = STUB_BASE;
  constexpr uInt32 ADDR_MEMCPY = STUB_BASE + 4;

  constexpr uInt32 ADDR_VCS_LDA_FOR_BUS_STUFF2 = STUB_BASE + 8;
  constexpr uInt32 ADDR_VCS_LDX_FOR_BUS_STUFF2 = STUB_BASE + 12;
  constexpr uInt32 ADDR_VCS_LDY_FOR_BUS_STUFF2 = STUB_BASE + 16;
  constexpr uInt32 ADDR_VCS_WRITE3 = STUB_BASE + 20;
  constexpr uInt32 ADDR_VCS_JMP3 = STUB_BASE + 24;
  constexpr uInt32 ADDR_VCS_NOP2 = STUB_BASE + 28;
  constexpr uInt32 ADDR_VCS_NOP2N = STUB_BASE + 32;
  constexpr uInt32 ADDR_VCS_WRITE5 = STUB_BASE + 36;
  constexpr uInt32 ADDR_VCS_WRITE6 = STUB_BASE + 40;
  constexpr uInt32 ADDR_VCS_LDA2 = STUB_BASE + 44;
  constexpr uInt32 ADDR_VCS_LDX2 = STUB_BASE + 48;
  constexpr uInt32 ADDR_VCS_LDY2 = STUB_BASE + 52;
  constexpr uInt32 ADDR_VCS_SAX3 = STUB_BASE + 56;
  constexpr uInt32 ADDR_VCS_STA3 = STUB_BASE + 60;
  constexpr uInt32 ADDR_VCS_STX3 = STUB_BASE + 64;
  constexpr uInt32 ADDR_VCS_STY3 = STUB_BASE + 68;
  constexpr uInt32 ADDR_VCS_STA4 = STUB_BASE + 72;
  constexpr uInt32 ADDR_VCS_STX4 = STUB_BASE + 76;
  constexpr uInt32 ADDR_VCS_STY4 = STUB_BASE + 80;
  constexpr uInt32 ADDR_VCS_COPY_OVERBLANK_TO_RIOT_RAM = STUB_BASE + 84;
  constexpr uInt32 ADDR_VCS_START_OVERBLANK = STUB_BASE + 88;
  constexpr uInt32 ADDR_VCS_END_OVERBLANK = STUB_BASE + 92;
  constexpr uInt32 ADDR_VCS_READ4 = STUB_BASE + 96;
  constexpr uInt32 ADDR_RANDINT = STUB_BASE + 100;
  constexpr uInt32 ADDR_VCS_TXS2 = STUB_BASE + 104;
  constexpr uInt32 ADDR_VCS_JSR6 = STUB_BASE + 108;
  constexpr uInt32 ADDR_VCS_PHA3 = STUB_BASE + 112;
  constexpr uInt32 ADDR_VCS_PHP3 = STUB_BASE + 116;
  constexpr uInt32 ADDR_VCS_PLA4 = STUB_BASE + 120;
  constexpr uInt32 ADDR_VCS_PLP4 = STUB_BASE + 124;
  constexpr uInt32 ADDR_VCS_PLA4_EX = STUB_BASE + 128;
  constexpr uInt32 ADDR_VCS_PLP4_EX = STUB_BASE + 132;
  constexpr uInt32 ADDR_VCS_JMP_TO_RAM3 = STUB_BASE + 136;
  constexpr uInt32 ADDR_VCS_WAIT_FOR_ADDRESS = STUB_BASE + 140;
  constexpr uInt32 ADDR_INJECT_DMA_DATA = STUB_BASE + 144;

  constexpr uInt32 ADDR_TABLE_COLOR_LOOKUP_NTSC = ADDR_TABLES_BASE;
  constexpr uInt32 ADDR_TABLE_COLOR_LOOKUP_PAL = ADDR_TABLES_BASE + 256;
  constexpr uInt32 ADDR_TABLE_COLOR_LOOKUP_REVERSE_BYTE = ADDR_TABLES_BASE + 512;

  extern const uInt8 LOOKUP_TABLES[3 * 256];

  extern const uInt8 OVERBLANK_PROGRAM[];
  extern const uInt32 OVERBLANK_PROGRAM_SIZE;

  enum class Palette: uInt8 {pal, ntsc};

  vector<ElfLinker::ExternalSymbol> externalSymbols(Palette palette);
}

#endif // ELF_ENVIRONMENT
