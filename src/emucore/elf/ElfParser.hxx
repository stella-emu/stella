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

#ifndef ELF_PARSER
#define ELF_PARSER

#include "bspf.hxx"

class ElfParser {
  public:
    class EInvalidElf : public std::exception {
      friend ElfParser;

      public:
        const char* what() const noexcept override { return myReason.c_str(); }

        [[noreturn]] static void raise(string_view message) {
          throw EInvalidElf(message);
        }

      private:
        explicit EInvalidElf(string_view reason) : myReason(reason) {}

      private:
        const string myReason;
    };

    struct Header {
      uInt16 type;
      uInt16 arch;
      uInt8 endianess;
      uInt32 shOffset;

      uInt16 shNum;
      uInt16 shSize;
      uInt16 shstrIndex;
    };

    struct Section {
      uInt32 nameOffset;
      string name;

      uInt32 type;
      uInt32 flags;

      uInt32 virtualAddress;
      uInt32 offset;
      uInt32 size;

      uInt32 align;
    };

  public:
    static constexpr uInt8 ENDIAN_LITTLE_ENDIAN = 0x01;
    static constexpr uInt8 ENDIAN_BIG_ENDIAN = 0x02;

    static constexpr uInt16 ET_REL = 0x01;

    static constexpr uInt16 ARCH_ARM32 = 0x28;

    static constexpr uInt32 SHT_PROGBITS = 0x01;
    static constexpr uInt32 SHT_SYMTAB = 0x02;
    static constexpr uInt32 SHT_STRTAB = 0x03;
    static constexpr uInt32 SHT_RELA = 0x04;
    static constexpr uInt32 SHT_NOBITS = 0x08;
    static constexpr uInt32 SHT_REL = 0x09;
    static constexpr uInt32 SHT_INIT_ARRAY = 0x0e;
    static constexpr uInt32 SHT_PREINIT_ARRAY = 0x10;

  public:
    ElfParser() = default;

    void parse(const uInt8 *elfData, size_t size);

    const uInt8 *getData() const;
    size_t getSize() const;

    const Header& getHeader() const;
    const vector<Section>& getSections() const;
    const optional<Section> getSection(const std::string &name) const;

  private:
    uInt8 read8(uInt32 offset);
    uInt16 read16(uInt32 offset);
    uInt32 read32(uInt32 offset);

    Section readSection(uInt32 offset);
    const char* getName(const Section& section, uInt32 offset);

  private:
    const uInt8 *data{nullptr};
    size_t size;

    bool bigEndian{true};

    Header header;
    vector<Section> sections;
};

#endif // ELF_PARSER
