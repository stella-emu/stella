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

#ifndef ELF_FILE
#define ELF_FILE

#include "bspf.hxx"

class ElfFile {
  public:
    struct Section {
      uInt32 nameOffset;
      string name;

      uInt32 type;
      uInt32 flags;

      uInt32 virtualAddress;
      uInt32 offset;
      uInt32 size;

      uInt32 info;
      uInt32 align;
    };

    struct Symbol {
      uInt32 nameOffset;
      uInt32 value;
      uInt32 size;
      uInt8 info;
      uInt8 visibility;
      uInt16 section;

      string name;
      uInt8 bind;
      uInt8 type;
    };

    struct Relocation {
      uInt32 offset;
      uInt32 info;
      optional<uInt32> addend;

      uInt32 symbol;
      uInt8 type;
      string symbolName;
    };

  public:
    virtual ~ElfFile() = default;

    virtual const uInt8 *getData() const = 0;
    virtual size_t getSize() const = 0;

    virtual const vector<Section>& getSections() const = 0;
    virtual const vector<Symbol>& getSymbols() const = 0;
    virtual const optional<vector<Relocation>> getRelocations(size_t section) const = 0;

  public:
    static constexpr uInt8 ENDIAN_LITTLE_ENDIAN = 0x01;
    static constexpr uInt8 ENDIAN_BIG_ENDIAN = 0x02;

    static constexpr uInt16 ET_REL = 0x01;

    static constexpr uInt16 ARCH_ARM32 = 0x28;

    static constexpr uInt32 SHT_NULL = 0x00;
    static constexpr uInt32 SHT_PROGBITS = 0x01;
    static constexpr uInt32 SHT_SYMTAB = 0x02;
    static constexpr uInt32 SHT_STRTAB = 0x03;
    static constexpr uInt32 SHT_RELA = 0x04;
    static constexpr uInt32 SHT_NOBITS = 0x08;
    static constexpr uInt32 SHT_REL = 0x09;
    static constexpr uInt32 SHT_INIT_ARRAY = 0x0e;
    static constexpr uInt32 SHT_PREINIT_ARRAY = 0x10;

    static constexpr uInt32 SHN_ABS = 0xfff1;
    static constexpr uInt32 SHN_UND = 0x00;

    static constexpr uInt32 STT_SECTION = 0x03;
    static constexpr uInt32 STT_NOTYPE = 0x00;

    static constexpr uInt32 R_ARM_ABS32 = 0x02;
    static constexpr uInt32 R_ARM_THM_CALL = 0x0a;
    static constexpr uInt32 R_ARM_THM_JUMP24 = 0x1e;
    static constexpr uInt32 R_ARM_TARGET1 = 0x26;

    static constexpr uInt32 STT_FUNC = 0x02;
};

#endif // ELF_FILE
