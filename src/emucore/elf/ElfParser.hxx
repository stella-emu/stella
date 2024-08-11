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

#include <unordered_map>

#include "ElfFile.hxx"
#include "bspf.hxx"

class ElfParser : public ElfFile {
  public:
    class ElfParseError : public std::exception {
      friend ElfParser;

      public:
        const char* what() const noexcept override { return myReason.c_str(); }

        [[noreturn]] static void raise(string_view message) {
          throw ElfParseError(message);
        }

      private:
        explicit ElfParseError(string_view reason) : myReason(reason) {}

      private:
        string myReason;
    };

  public:
    ElfParser() = default;
    ~ElfParser() override = default;

    void parse(const uInt8 *elfData, size_t size);

    const uInt8* getData() const override;
    size_t getSize() const override;

    const vector<Section>& getSections() const override;
    const vector<Symbol>& getSymbols() const override;
    optional<vector<Relocation>> getRelocations(size_t section) const override;

  private:
    struct Header {
      uInt16 type{0};
      uInt16 arch{0};
      uInt8 endianess{0};
      uInt32 shOffset{0};

      uInt16 shNum{0};
      uInt16 shSize{0};
      uInt16 shstrIndex{0};
    };

  private:
    uInt8 read8(uInt32 offset) const;
    uInt16 read16(uInt32 offset) const;
    uInt32 read32(uInt32 offset) const;

    Section readSection(uInt32 offset) const;
    Symbol readSymbol(uInt32 index, const Section& symSec, const Section& strSec) const;
    Relocation readRelocation(uInt32 index, const Section& sec) const;
    const char* getName(const Section& section, uInt32 offset) const;

    const Section* getSymtab() const;
    const Section* getStrtab() const;

  private:
    const uInt8 *myData{nullptr};
    size_t mySize{0};

    bool myBigEndian{false};

    Header myHeader;
    vector<Section> mySections;
    vector<Symbol> mySymbols;
    std::unordered_map<size_t, vector<Relocation>> myRelocations;

  private:
    ElfParser(const ElfParser&) = delete;
    ElfParser(ElfParser&&) = delete;
    ElfParser& operator=(const ElfParser&) = delete;
    ElfParser& operator=(ElfParser&&) = delete;
};

ostream& operator<<(ostream& os, const ElfParser::Section& section);
ostream& operator<<(ostream& os, const ElfParser::Symbol& symbol);
ostream& operator<<(ostream& os, const ElfParser::Relocation& relocation);

#endif // ELF_PARSER
