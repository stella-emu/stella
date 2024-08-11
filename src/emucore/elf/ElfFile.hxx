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
      uInt32 nameOffset{0};
      string name;

      uInt32 type{0};
      uInt32 flags{0};

      uInt32 virtualAddress{0};
      uInt32 offset{0};
      uInt32 size{0};

      uInt32 info{0};
      uInt32 align{0};
    };

    struct Symbol {
      uInt32 nameOffset{0};
      uInt32 value{0};
      uInt32 size{0};
      uInt8 info{0};
      uInt8 visibility{0};
      uInt16 section{0};

      string name;
      uInt8 bind{0};
      uInt8 type{0};
    };

    struct Relocation {
      uInt32 offset{0};
      uInt32 info{0};
      optional<uInt32> addend;

      uInt32 symbol{0};
      uInt8 type{0};
      string symbolName;
    };

  public:
    ElfFile() = default;
    virtual ~ElfFile() = default;

    virtual const uInt8* getData() const = 0;
    virtual size_t getSize() const = 0;

    virtual const vector<Section>& getSections() const = 0;
    virtual const vector<Symbol>& getSymbols() const = 0;
    virtual optional<vector<Relocation>> getRelocations(size_t section) const = 0;

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
    static constexpr uInt32 R_ARM_REL32 = 0x03;
    static constexpr uInt32 R_ARM_THM_CALL = 0x0a;
    static constexpr uInt32 R_ARM_THM_JUMP24 = 0x1e;
    static constexpr uInt32 R_ARM_TARGET1 = 0x26;

    static constexpr uInt32 STT_FUNC = 0x02;

  private:
    // Following constructors and assignment operators not supported
    ElfFile(const ElfFile&) = delete;
    ElfFile(ElfFile&&) = delete;
    ElfFile& operator=(const ElfFile&) = delete;
    ElfFile& operator=(ElfFile&&) = delete;
};

#endif // ELF_FILE
