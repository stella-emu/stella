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

#ifndef ELF_LINKER
#define ELF_LINKER

#include "bspf.hxx"
#include "ElfFile.hxx"

class ElfLinker {
  public:
    class ElfLinkError : public std::exception {
      friend ElfLinker;

      public:
        const char* what() const noexcept override { return myReason.c_str(); }

        [[noreturn]] static void raise(string_view message) {
          throw ElfLinkError(message);
        }

      private:
        explicit ElfLinkError(string_view reason) : myReason(reason) {}

      private:
        string myReason;
    };

    class ElfSymbolResolutionError : public std::exception {
      friend ElfLinker;

      public:
        const char* what() const noexcept override { return myReason.c_str(); }

        [[noreturn]] static void raise(string_view message) {
          throw ElfSymbolResolutionError(message);
        }

      private:
        explicit ElfSymbolResolutionError(string_view reason) : myReason(reason) {}

      private:
        string myReason;
    };

    enum class SegmentType: uInt8 { text, data, rodata };

    struct RelocatedSection {
      SegmentType segment{};
      uInt32 offset{0};
    };

    struct RelocatedSymbol {
      optional<SegmentType> segment;
      uInt32 value{0};
      bool undefined{true};
    };

    struct ExternalSymbol {
      string name;
      uInt32 value{0};
    };

  public:
    ElfLinker(uInt32 textBase, uInt32 dataBase, uInt32 rodataBase, const ElfFile& elf);
    ~ElfLinker() = default;

    ElfLinker& setUndefinedSymbolDefault(uInt32 defaultValue);
    void link(const vector<ExternalSymbol>& externalSymbols);
    void relink(const vector<ExternalSymbol>& externalSymbols);

    uInt32 getSegmentSize(SegmentType type) const;
    const uInt8* getSegmentData(SegmentType type) const;
    uInt32 getSegmentBase(SegmentType type) const;

    const vector<uInt32>& getInitArray() const;
    const vector<uInt32>& getPreinitArray() const;

    RelocatedSymbol findRelocatedSymbol(string_view name) const;

    const vector<std::optional<RelocatedSection>>& getRelocatedSections() const;
    const vector<std::optional<RelocatedSymbol>>& getRelocatedSymbols() const;

  private:
    uInt32& getSegmentSizeRef(SegmentType type);
    unique_ptr<uInt8[]>& getSegmentDataRef(SegmentType type);

    void relocateSections();
    void copySections();
    void relocateInitArrays();
    void relocateSymbols(const vector<ExternalSymbol>& externalSymbols);
    void applyRelocationsToSections();
    void copyInitArrays(vector<uInt32>& initArray, const std::unordered_map<uInt32, uInt32>& relocatedInitArrays);

    void applyRelocationToSection(const ElfFile::Relocation& relocation, size_t iSection);
    void applyRelocationsToInitArrays(uInt8 initArrayType, vector<uInt32>& initArray,
                                      const std::unordered_map<uInt32, uInt32>& relocatedInitArrays);

    static constexpr uInt32 read32(const uInt8* address) {
      uInt32 value = *(address++);
      value |= *(address++) << 8;
      value |= *(address++) << 16;
      value |= *(address++) << 24;

      return value;
    }
    static constexpr void write32(uInt8* address, uInt32 value) {
      *(address++) = value;
      *(address++) = value >> 8;
      *(address++) = value >> 16;
      *(address++) = value >> 24;
    }

  private:
    std::optional<uInt32> undefinedSymbolDefault;

    const uInt32 myTextBase{0};
    const uInt32 myDataBase{0};
    const uInt32 myRodataBase{0};
    const ElfFile& myElf;

    uInt32 myTextSize{0};
    uInt32 myDataSize{0};
    uInt32 myRodataSize{0};
    unique_ptr<uInt8[]> myTextData;
    unique_ptr<uInt8[]> myDataData;
    unique_ptr<uInt8[]> myRodataData;

    vector<optional<RelocatedSection>> myRelocatedSections;
    vector<optional<RelocatedSymbol>> myRelocatedSymbols;

    vector<uInt32> myInitArray;
    vector<uInt32> myPreinitArray;

  private:
    ElfLinker(const ElfLinker&) = delete;
    ElfLinker(ElfLinker&&) = delete;
    ElfLinker& operator=(const ElfLinker&) = delete;
    ElfLinker& operator=(ElfLinker&&) = delete;
};

#endif // ELF_LINKER
