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
  // Memory map. Our cortex emulator maps memory in multiples of 4kB, so use that
  // here, too.

  // runtime method stubs: 4kB - 8kB
  constexpr uInt32 ADDR_STUB_BASE = 0x1000;       // 4kB
  constexpr uInt32 STUB_SIZE = 0x1000;            // 4kB

  // stack (32kB): 16kB - 48kB
  constexpr uInt32 ADDR_STACK_BASE = 0x4000;      // 16kB
  constexpr uInt32 STACK_SIZE = 0x8000;      // 32kB

  // text (1MB): 1MB - 2MB
  constexpr uInt32 ADDR_TEXT_BASE = 0x00100000;   // 1MB
  constexpr uInt32 TEXT_SIZE =  0x00100000;       // 1MB

  // data (512kB): 2MB - 2.5MB
  constexpr uInt32 ADDR_DATA_BASE = 0x00200000;   // 2MB
  constexpr uInt32 DATA_SIZE =  0x00080000;       // 512kB

  // rodata (512kB): 3MB - 3.5MB
  constexpr uInt32 ADDR_RODATA_BASE = 0x00300000; // 3MB
  constexpr uInt32 RODATA_SIZE = 0x00080000;      // 512kB

  // lookup tables (4kB): 4MB -- 4.004MB
  constexpr uInt32 ADDR_TABLES_BASE = 0x00400000; // 4MB
  constexpr uInt32 TABLES_SIZE = 0x1000;          // 4kB

  // peripherials: 4kB @ 0xf0000000
  constexpr uInt32 ADDR_PERIPHERIALS_BASE = 0xf0000000;
  constexpr uInt32 PERIPHERIALS_SIZE = 0x1000;

  constexpr uInt32 ADDR_ADDR_IDR = ADDR_PERIPHERIALS_BASE;
  constexpr uInt32 ADDR_DATA_IDR =ADDR_PERIPHERIALS_BASE + 4;
  constexpr uInt32 ADDR_DATA_ODR = ADDR_PERIPHERIALS_BASE + 8;
  constexpr uInt32 ADDR_DATA_MODER = ADDR_PERIPHERIALS_BASE + 12;

  constexpr uInt32 ADDR_MEMSET = ADDR_STUB_BASE;
  constexpr uInt32 ADDR_MEMCPY = ADDR_STUB_BASE + 4;

  constexpr uInt32 ADDR_VCS_LDA_FOR_BUS_STUFF2 = ADDR_STUB_BASE + 8;
  constexpr uInt32 ADDR_VCS_LDX_FOR_BUS_STUFF2 = ADDR_STUB_BASE + 12;
  constexpr uInt32 ADDR_VCS_LDY_FOR_BUS_STUFF2 = ADDR_STUB_BASE + 16;
  constexpr uInt32 ADDR_VCS_WRITE3 = ADDR_STUB_BASE + 20;
  constexpr uInt32 ADDR_VCS_JMP3 = ADDR_STUB_BASE + 24;
  constexpr uInt32 ADDR_VCS_NOP2 = ADDR_STUB_BASE + 28;
  constexpr uInt32 ADDR_VCS_NOP2N = ADDR_STUB_BASE + 32;
  constexpr uInt32 ADDR_VCS_WRITE5 = ADDR_STUB_BASE + 36;
  constexpr uInt32 ADDR_VCS_WRITE6 = ADDR_STUB_BASE + 40;
  constexpr uInt32 ADDR_VCS_LDA2 = ADDR_STUB_BASE + 44;
  constexpr uInt32 ADDR_VCS_LDX2 = ADDR_STUB_BASE + 48;
  constexpr uInt32 ADDR_VCS_LDY2 = ADDR_STUB_BASE + 52;
  constexpr uInt32 ADDR_VCS_SAX3 = ADDR_STUB_BASE + 56;
  constexpr uInt32 ADDR_VCS_STA3 = ADDR_STUB_BASE + 60;
  constexpr uInt32 ADDR_VCS_STX3 = ADDR_STUB_BASE + 64;
  constexpr uInt32 ADDR_VCS_STY3 = ADDR_STUB_BASE + 68;
  constexpr uInt32 ADDR_VCS_STA4 = ADDR_STUB_BASE + 72;
  constexpr uInt32 ADDR_VCS_STX4 = ADDR_STUB_BASE + 76;
  constexpr uInt32 ADDR_VCS_STY4 = ADDR_STUB_BASE + 80;
  constexpr uInt32 ADDR_VCS_COPY_OVERBLANK_TO_RIOT_RAM = ADDR_STUB_BASE + 84;
  constexpr uInt32 ADDR_VCS_START_OVERBLANK = ADDR_STUB_BASE + 88;
  constexpr uInt32 ADDR_VCS_END_OVERBLANK = ADDR_STUB_BASE + 92;
  constexpr uInt32 ADDR_VCS_READ4 = ADDR_STUB_BASE + 96;
  constexpr uInt32 ADDR_RANDINT = ADDR_STUB_BASE + 100;
  constexpr uInt32 ADDR_VCS_TXS2 = ADDR_STUB_BASE + 104;
  constexpr uInt32 ADDR_VCS_JSR6 = ADDR_STUB_BASE + 108;
  constexpr uInt32 ADDR_VCS_PHA3 = ADDR_STUB_BASE + 112;
  constexpr uInt32 ADDR_VCS_PHP3 = ADDR_STUB_BASE + 116;
  constexpr uInt32 ADDR_VCS_PLA4 = ADDR_STUB_BASE + 120;
  constexpr uInt32 ADDR_VCS_PLP4 = ADDR_STUB_BASE + 124;
  constexpr uInt32 ADDR_VCS_PLA4_EX = ADDR_STUB_BASE + 128;
  constexpr uInt32 ADDR_VCS_PLP4_EX = ADDR_STUB_BASE + 132;
  constexpr uInt32 ADDR_VCS_JMP_TO_RAM3 = ADDR_STUB_BASE + 136;
  constexpr uInt32 ADDR_VCS_WAIT_FOR_ADDRESS = ADDR_STUB_BASE + 140;
  constexpr uInt32 ADDR_INJECT_DMA_DATA = ADDR_STUB_BASE + 144;

  constexpr uInt32 ADDR_TABLE_COLOR_LOOKUP_NTSC = ADDR_TABLES_BASE;
  constexpr uInt32 ADDR_TABLE_COLOR_LOOKUP_PAL = ADDR_TABLES_BASE + 256;
  constexpr uInt32 ADDR_TABLE_COLOR_LOOKUP_REVERSE_BYTE = ADDR_TABLES_BASE + 512;

  extern const uInt8 LOOKUP_TABLES[3 * 256];

  extern const uInt8 OVERBLANK_PROGRAM[];
  extern const uInt32 OVERBLANK_PROGRAM_SIZE;

  constexpr uInt32  ST_NTSC_2600 =	0;
  constexpr uInt32  ST_PAL_2600 = 1;
  constexpr uInt32  ST_PAL60_2600 = 2;

  constexpr uInt32 RETURN_ADDR = 0xffffdead;

  constexpr uInt32 ERR_STOP_EXECUTION = 1;
  constexpr uInt32 ERR_RETURN = 2;

  constexpr uInt32 QUEUE_SIZE_LIMIT = 10;

  enum class SystemType: uInt8 {ntsc, pal, pal60};

  vector<ElfLinker::ExternalSymbol> externalSymbols(SystemType systemType);
}  // namespace elfEnvironment

#endif // ELF_ENVIRONMENT
