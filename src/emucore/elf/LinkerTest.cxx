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

#include <gtest/gtest.h>
#include <unordered_map>

#include "bspf.hxx"
#include "ElfFile.hxx"
#include "ElfLinker.hxx"

namespace {
  using SegmentType = ElfLinker::SegmentType;

  uInt32 segmentRead32(const ElfLinker& linker, SegmentType segment, uInt32 offset) {
    const uInt8* addr = linker.getSegmentData(segment) + offset;

    uInt32 value = *(addr++);
    value |= *(addr++) << 8;
    value |= *(addr++) << 16;
    value |= *(addr++) << 24;

    return value;
  }

  class ElfFixture: public ElfFile {
    public:
      explicit ElfFixture(size_t size) : mySize(size) {
        myData = make_unique<uInt8[]>(mySize);

        addSection("", 0, 0, 0);
      }

      const uInt8 *getData() const override {
        return myData.get();
      }

      size_t getSize() const override {
        return mySize;
      }

      const vector<Section>& getSections() const override {
        return mySections;
      }

      const vector<Symbol>& getSymbols() const override {
        return mySymbols;
      }

      const optional<vector<Relocation>> getRelocations(size_t section) const override {
        return myRelocations.contains(section) ? myRelocations.at(section) : optional<vector<Relocation>>();
      }

      ElfFixture& addSection(string_view name, uInt32 type, uInt32 offset, uInt32 size, uInt32 align = 4) {
        mySections.push_back({
          .nameOffset = 0,
          .name = string(name),
          .type = type,
          .flags = 0,
          .virtualAddress = 0,
          .offset = offset,
          .size = size,
          .info = 0,
          .align = align
        });

        return *this;
      }

      ElfFixture& addSymbol(string_view name, uInt32 value, uInt16 section, uInt8 type = ElfFile::STT_NOTYPE) {
        mySymbols.push_back({
          .nameOffset = 0,
          .value = value,
          .size = 0,
          .info = 0,
          .visibility = 0,
          .section = section,
          .name = string(name),
          .bind = 0,
          .type = type
        });

        return *this;
      }

      ElfFixture& addRelocation(uInt32 section, uInt32 symbol, uInt32 offset,
                                uInt8 type, optional<uInt32> addend = std::nullopt)
      {
        if (!myRelocations.contains(section))
          myRelocations[section] = vector<ElfFile::Relocation>();

        myRelocations[section].push_back(ElfFile::Relocation{
          .offset = offset,
          .info = 0,
          .addend = addend,
          .symbol = symbol,
          .type = type,
          .symbolName = ""
        });

        return *this;
      }

      ElfFixture& write8(size_t address, uInt8 value) {
        myData[address] = value;

        return *this;
      }

      ElfFixture& write32(size_t address, uInt32 value) {
        myData[address++] = value;
        myData[address++] = value >> 8;
        myData[address++] = value >> 16;
        myData[address++] = value >> 24;

        return *this;
      }

    public:
      size_t mySize;
      unique_ptr<uInt8[]> myData;

      vector<Section> mySections;
      vector<Symbol> mySymbols;
      std::unordered_map<size_t, vector<Relocation>> myRelocations;
  };

  TEST(ElfLinker, HasEmptySegmentsForAnEmptyELF) {
    ElfFixture fixture(0);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    linker.link({});

    EXPECT_EQ(linker.getSegmentSize(SegmentType::text), static_cast<uInt32>(0));
    EXPECT_EQ(linker.getSegmentSize(SegmentType::data), static_cast<uInt32>(0));
    EXPECT_EQ(linker.getSegmentSize(SegmentType::rodata), static_cast<uInt32>(0));
  }

  TEST(ElfLinker, TextSectionsGoToText) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".text.1", ElfFile::SHT_PROGBITS, 0, 10, 4)
      .write8(0, 0x01)
      .addSection(".text.2", ElfFile::SHT_PROGBITS, 10, 21, 4)
      .write8(10, 0x02)
      .addSection(".text.3", ElfFile::SHT_PROGBITS, 31, 33, 2)
      .write8(31, 0x03)
      .addSection(".text.4", ElfFile::SHT_PROGBITS, 64, 11, 1)
      .write8(64, 0x04);

    linker.link({});

    EXPECT_EQ(linker.getSegmentSize(SegmentType::text), static_cast<uInt32>(78));
    EXPECT_EQ(linker.getSegmentSize(SegmentType::data), static_cast<uInt32>(0));
    EXPECT_EQ(linker.getSegmentSize(SegmentType::rodata), static_cast<uInt32>(0));

    EXPECT_EQ(linker.getRelocatedSections()[1]->offset, static_cast<uInt32>(0));
    EXPECT_EQ(linker.getRelocatedSections()[1]->segment, SegmentType::text);

    EXPECT_EQ(linker.getRelocatedSections()[2]->offset, static_cast<uInt32>(12));
    EXPECT_EQ(linker.getRelocatedSections()[2]->segment, SegmentType::text);

    EXPECT_EQ(linker.getRelocatedSections()[3]->offset, static_cast<uInt32>(34));
    EXPECT_EQ(linker.getRelocatedSections()[3]->segment, SegmentType::text);

    EXPECT_EQ(linker.getRelocatedSections()[4]->offset, static_cast<uInt32>(67));
    EXPECT_EQ(linker.getRelocatedSections()[4]->segment, SegmentType::text);

    EXPECT_EQ(linker.getSegmentData(SegmentType::text)[0], 0x01);
    EXPECT_EQ(linker.getSegmentData(SegmentType::text)[12], 0x02);
    EXPECT_EQ(linker.getSegmentData(SegmentType::text)[34], 0x03);
    EXPECT_EQ(linker.getSegmentData(SegmentType::text)[67], 0x04);
  }

  TEST(ElfLinker, DataSectionsGoToData) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".data.1", ElfFile::SHT_PROGBITS, 0, 10, 4)
      .write8(0, 0x01)
      .addSection(".data.2", ElfFile::SHT_PROGBITS, 10, 21, 4)
      .write8(10, 0x02)
      .addSection(".data.3", ElfFile::SHT_PROGBITS, 31, 33, 2)
      .write8(31, 0x03)
      .addSection(".data.4", ElfFile::SHT_PROGBITS, 64, 11, 1)
      .write8(64, 0x04);

    linker.link({});

    EXPECT_EQ(linker.getSegmentSize(SegmentType::text), static_cast<uInt32>(0));
    EXPECT_EQ(linker.getSegmentSize(SegmentType::data), static_cast<uInt32>(78));
    EXPECT_EQ(linker.getSegmentSize(SegmentType::rodata), static_cast<uInt32>(0));

    EXPECT_EQ(linker.getRelocatedSections()[1]->offset, static_cast<uInt32>(0));
    EXPECT_EQ(linker.getRelocatedSections()[1]->segment, SegmentType::data);

    EXPECT_EQ(linker.getRelocatedSections()[2]->offset, static_cast<uInt32>(12));
    EXPECT_EQ(linker.getRelocatedSections()[2]->segment, SegmentType::data);

    EXPECT_EQ(linker.getRelocatedSections()[3]->offset, static_cast<uInt32>(34));
    EXPECT_EQ(linker.getRelocatedSections()[3]->segment, SegmentType::data);

    EXPECT_EQ(linker.getRelocatedSections()[4]->offset, static_cast<uInt32>(67));
    EXPECT_EQ(linker.getRelocatedSections()[4]->segment, SegmentType::data);

    EXPECT_EQ(linker.getSegmentData(SegmentType::data)[0], 0x01);
    EXPECT_EQ(linker.getSegmentData(SegmentType::data)[12], 0x02);
    EXPECT_EQ(linker.getSegmentData(SegmentType::data)[34], 0x03);
    EXPECT_EQ(linker.getSegmentData(SegmentType::data)[67], 0x04);
  }

TEST(ElfLinker, RodataSectionsGoToRodata) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".rodata.1", ElfFile::SHT_PROGBITS, 0, 10, 4)
      .write8(0, 0x01)
      .addSection(".rodata.2", ElfFile::SHT_PROGBITS, 10, 21, 4)
      .write8(10, 0x02)
      .addSection(".rodata.3", ElfFile::SHT_PROGBITS, 31, 33, 2)
      .write8(31, 0x03)
      .addSection(".rodata.4", ElfFile::SHT_PROGBITS, 64, 11, 1)
      .write8(64, 0x04);

    linker.link({});

    EXPECT_EQ(linker.getSegmentSize(SegmentType::text), static_cast<uInt32>(0));
    EXPECT_EQ(linker.getSegmentSize(SegmentType::data), static_cast<uInt32>(0));
    EXPECT_EQ(linker.getSegmentSize(SegmentType::rodata), static_cast<uInt32>(78));

    EXPECT_EQ(linker.getRelocatedSections()[1]->offset, static_cast<uInt32>(0));
    EXPECT_EQ(linker.getRelocatedSections()[1]->segment, SegmentType::rodata);

    EXPECT_EQ(linker.getRelocatedSections()[2]->offset, static_cast<uInt32>(12));
    EXPECT_EQ(linker.getRelocatedSections()[2]->segment, SegmentType::rodata);

    EXPECT_EQ(linker.getRelocatedSections()[3]->offset, static_cast<uInt32>(34));
    EXPECT_EQ(linker.getRelocatedSections()[3]->segment, SegmentType::rodata);

    EXPECT_EQ(linker.getRelocatedSections()[4]->offset, static_cast<uInt32>(67));
    EXPECT_EQ(linker.getRelocatedSections()[4]->segment, SegmentType::rodata);

    EXPECT_EQ(linker.getSegmentData(SegmentType::rodata)[0], 0x01);
    EXPECT_EQ(linker.getSegmentData(SegmentType::rodata)[12], 0x02);
    EXPECT_EQ(linker.getSegmentData(SegmentType::rodata)[34], 0x03);
    EXPECT_EQ(linker.getSegmentData(SegmentType::rodata)[67], 0x04);
  }

  TEST(ElfLinker, BssSectionsGoToTheEndOfData) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".data.1", ElfFile::SHT_PROGBITS, 0, 10, 4)
      .write8(0, 0x01)
      .addSection(".bss", ElfFile::SHT_NOBITS, 0, 21, 4)
      .addSection(".data.2", ElfFile::SHT_PROGBITS, 10, 33, 2)
      .write8(10, 0x02)
      .addSection(".noinit", ElfFile::SHT_NOBITS, 0, 11, 1);

    linker.link({});

    EXPECT_EQ(linker.getSegmentSize(SegmentType::text), static_cast<uInt32>(0));
    EXPECT_EQ(linker.getSegmentSize(SegmentType::data), static_cast<uInt32>(76));
    EXPECT_EQ(linker.getSegmentSize(SegmentType::rodata), static_cast<uInt32>(0));

    EXPECT_EQ(linker.getRelocatedSections()[1]->offset, static_cast<uInt32>(0));
    EXPECT_EQ(linker.getRelocatedSections()[1]->segment, SegmentType::data);

    EXPECT_EQ(linker.getRelocatedSections()[2]->offset, static_cast<uInt32>(44));
    EXPECT_EQ(linker.getRelocatedSections()[2]->segment, SegmentType::data);

    EXPECT_EQ(linker.getRelocatedSections()[3]->offset, static_cast<uInt32>(10));
    EXPECT_EQ(linker.getRelocatedSections()[3]->segment, SegmentType::data);

    EXPECT_EQ(linker.getRelocatedSections()[4]->offset, static_cast<uInt32>(65));
    EXPECT_EQ(linker.getRelocatedSections()[4]->segment, SegmentType::data);

    EXPECT_EQ(linker.getSegmentData(SegmentType::data)[0], 0x01);
    EXPECT_EQ(linker.getSegmentData(SegmentType::data)[10], 0x02);
    EXPECT_EQ(linker.getSegmentData(SegmentType::data)[44], 0);
    EXPECT_EQ(linker.getSegmentData(SegmentType::data)[65], 0);
  }

  TEST(ElfLinker, SegmentsMayEndNextToEachOther) {
    ElfFixture fixture(1000);
    ElfLinker linker(100, 200, 300, fixture);

    fixture
      .addSection(".text", ElfFile::SHT_PROGBITS, 0, 100, 4)
      .addSection(".data", ElfFile::SHT_PROGBITS, 0, 100, 4)
      .addSection(".rodata", ElfFile::SHT_PROGBITS, 0, 100, 4);

    EXPECT_NO_THROW(linker.link({}));
  }

  TEST(ElfLinker, OverlappingTextAndDataSegmentsThrow) {
    ElfFixture fixture(1000);
    ElfLinker linker(100, 200, 300, fixture);

    fixture
      .addSection(".text", ElfFile::SHT_PROGBITS, 0, 101, 4)
      .addSection(".data", ElfFile::SHT_PROGBITS, 0, 100, 4)
      .addSection(".rodata", ElfFile::SHT_PROGBITS, 0, 100, 4);

    EXPECT_THROW(linker.link({}), ElfLinker::ElfLinkError);
  }

  TEST(ElfLinker, OverlappingTextAndRodataSegmentsThrow) {
    ElfFixture fixture(1000);
    ElfLinker linker(100, 300, 200, fixture);

    fixture
      .addSection(".text", ElfFile::SHT_PROGBITS, 0, 101, 4)
      .addSection(".data", ElfFile::SHT_PROGBITS, 0, 100, 4)
      .addSection(".rodata", ElfFile::SHT_PROGBITS, 0, 100, 4);

    EXPECT_THROW(linker.link({}), ElfLinker::ElfLinkError);
  }

  TEST(ElfLinker, OverlappingDataAndRodataSegmentsThrow) {
    ElfFixture fixture(1000);
    ElfLinker linker(100, 200, 300, fixture);

    fixture
      .addSection(".text", ElfFile::SHT_PROGBITS, 0, 100, 4)
      .addSection(".data", ElfFile::SHT_PROGBITS, 0, 101, 4)
      .addSection(".rodata", ElfFile::SHT_PROGBITS, 0, 100, 4);

    EXPECT_THROW(linker.link({}), ElfLinker::ElfLinkError);
  }

  TEST(ElfLinker, ABSSymbolsAreAcceptedUnchanged) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSymbol("foo", 0x12345678, ElfFile::SHN_ABS);

    linker.link({});

    EXPECT_EQ(linker.getRelocatedSymbols().size(), static_cast<size_t>(1));
    EXPECT_TRUE(linker.getRelocatedSymbols()[0].has_value());
    EXPECT_FALSE(linker.getRelocatedSymbols()[0]->undefined);
    EXPECT_EQ(linker.getRelocatedSymbols()[0]->value, static_cast<uInt32>(0x12345678));
  }

  TEST(ElfLinker, UNDSymbolesAreTakenFromExternals) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSymbol("foo", 0, ElfFile::SHN_UND);

    linker.link({{"foo", 0x12345678}});

    EXPECT_EQ(linker.getRelocatedSymbols().size(), static_cast<size_t>(1));
    EXPECT_TRUE(linker.getRelocatedSymbols()[0].has_value());
    EXPECT_FALSE(linker.getRelocatedSymbols()[0]->undefined);
    EXPECT_EQ(linker.getRelocatedSymbols()[0]->value, static_cast<uInt32>(0x12345678));
  }

  TEST(ElfLinker, UNDSymbolsAreResolvedWithTheDefaultIfSet) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSymbol("foo", 0, ElfFile::SHN_UND);

    linker
      .setUndefinedSymbolDefault(0x12345678)
      .link({});

    EXPECT_EQ(linker.getRelocatedSymbols().size(), static_cast<size_t>(1));
    EXPECT_TRUE(linker.getRelocatedSymbols()[0].has_value());
    EXPECT_TRUE(linker.getRelocatedSymbols()[0]->undefined);
    EXPECT_EQ(linker.getRelocatedSymbols()[0]->value, static_cast<uInt32>(0x12345678));
  }

  TEST(ElfLinker, UNDSymbolsAreIgnoredIfTheyCannotBeResolved) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSymbol("foo", 0, ElfFile::SHN_UND);

    linker.link({});

    EXPECT_EQ(linker.getRelocatedSymbols().size(), static_cast<size_t>(1));
    EXPECT_FALSE(linker.getRelocatedSymbols()[0].has_value());
  }

  TEST(ElfLinker, SymbolsThatReferToTextAreResolvedRelativeToText) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".text.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".text.2", ElfFile::SHT_PROGBITS, 0, 0xef)
      .addSection(".data", ElfFile::SHT_PROGBITS, 0, 0xff)
      .addSection(".rodata", ElfFile::SHT_PROGBITS, 0, 0xff)
      .addSymbol("foo", 0x42, 2);

    linker.link({});

    EXPECT_EQ(linker.getRelocatedSymbols().size(), static_cast<size_t>(1));
    EXPECT_TRUE(linker.getRelocatedSymbols()[0].has_value());
    EXPECT_FALSE(linker.getRelocatedSymbols()[0]->undefined);
    EXPECT_EQ(linker.getRelocatedSymbols()[0]->value, static_cast<uInt32>(0x00100052));
  }

  TEST(ElfLinker, SymbolsThatReferToDataAreResolvedRelativeToData) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".text", ElfFile::SHT_PROGBITS, 0, 0xff)
      .addSection(".data.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".data.2", ElfFile::SHT_PROGBITS, 0, 0xef)
      .addSection(".rodata", ElfFile::SHT_PROGBITS, 0, 0xff)
      .addSymbol("foo", 0x42, 3);

    linker.link({});

    EXPECT_EQ(linker.getRelocatedSymbols().size(), static_cast<size_t>(1));
    EXPECT_TRUE(linker.getRelocatedSymbols()[0].has_value());
    EXPECT_FALSE(linker.getRelocatedSymbols()[0]->undefined);
    EXPECT_EQ(linker.getRelocatedSymbols()[0]->value, static_cast<uInt32>(0x00200052));
  }

  TEST(ElfLinker, SymbolsThatReferToRodataAreResolvedRelativeToRodata) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".text", ElfFile::SHT_PROGBITS, 0, 0xff)
      .addSection(".data", ElfFile::SHT_PROGBITS, 0, 0xff)
      .addSection(".rodata.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".rodata.2", ElfFile::SHT_PROGBITS, 0, 0xef)
      .addSymbol("foo", 0x42, 4);

    linker.link({});

    EXPECT_EQ(linker.getRelocatedSymbols().size(), static_cast<size_t>(1));
    EXPECT_TRUE(linker.getRelocatedSymbols()[0].has_value());
    EXPECT_FALSE(linker.getRelocatedSymbols()[0]->undefined);
    EXPECT_EQ(linker.getRelocatedSymbols()[0]->value, static_cast<uInt32>(0x00300052));
  }

  TEST(ElfLinker, SymbolsThatReferToBssAreResolvedRelativeToBss) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".text", ElfFile::SHT_PROGBITS, 0, 0xff)
      .addSection(".data", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".bss", ElfFile::SHT_PROGBITS, 0, 0xef)
      .addSection(".rodata", ElfFile::SHT_PROGBITS, 0, 0xff)
      .addSymbol("foo", 0x42, 3);

    linker.link({});

    EXPECT_EQ(linker.getRelocatedSymbols().size(), static_cast<size_t>(1));
    EXPECT_TRUE(linker.getRelocatedSymbols()[0].has_value());
    EXPECT_FALSE(linker.getRelocatedSymbols()[0]->undefined);
    EXPECT_EQ(linker.getRelocatedSymbols()[0]->value, static_cast<uInt32>(0x00200052));
  }

  TEST(ElfLinker, SymbolsThatReferToSectionsThatAreNotLoadedAreIgnored) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".init_array", ElfFile::SHT_INIT_ARRAY, 0, 0x10)
      .addSymbol("foo", 0, 1);

    linker.link({});

    EXPECT_EQ(linker.getRelocatedSymbols().size(), static_cast<size_t>(1));
    EXPECT_FALSE(linker.getRelocatedSymbols()[0].has_value());
  }

  TEST(ElfLinker, R_ARM_ABS32_InsertsTheValueAtTheTargetPosition_text) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".text.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".text.2", ElfFile::SHT_PROGBITS, 0x10, 0x10)
      .addSymbol("foo", 0x12345678, ElfFile::SHN_ABS)
      .addRelocation(2, 0, 0x04, ElfFile::R_ARM_ABS32);

    linker.link({});

    EXPECT_EQ(segmentRead32(linker, SegmentType::text, 0x14), static_cast<uInt32>(0x12345678));
  }

  TEST(ElfLinker, R_ARM_ABS32_InsertsTheValueAtTheTargetPosition_data) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".data.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".data.2", ElfFile::SHT_PROGBITS, 0x10, 0x10)
      .addSymbol("foo", 0x12345678, ElfFile::SHN_ABS)
      .addRelocation(2, 0, 0x04, ElfFile::R_ARM_ABS32);

    linker.link({});

    EXPECT_EQ(segmentRead32(linker, SegmentType::data, 0x14), static_cast<uInt32>(0x12345678));
  }

  TEST(ElfLinker, R_ARM_ABS32_InsertsTheValueAtTheTargetPosition_rodata) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".rodata.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".rodata.2", ElfFile::SHT_PROGBITS, 0x10, 0x10)
      .addSymbol("foo", 0x12345678, ElfFile::SHN_ABS)
      .addRelocation(2, 0, 0x04, ElfFile::R_ARM_ABS32);

    linker.link({});

    EXPECT_EQ(segmentRead32(linker, SegmentType::rodata, 0x14), static_cast<uInt32>(0x12345678));
  }

  TEST(ElfLinker, R_ARM_ABS32_AddsAddendToTarget) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".rodata.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".rodata.2", ElfFile::SHT_PROGBITS, 0x10, 0x10)
      .addSymbol("foo", 0x12345678, ElfFile::SHN_ABS)
      .addRelocation(2, 0, 0x04, ElfFile::R_ARM_ABS32, -4);


    linker.link({});

    EXPECT_EQ(segmentRead32(linker, SegmentType::rodata, 0x14), static_cast<uInt32>(0x12345674));
  }

  TEST(ElfLinker, R_ARM_ABS32_UsesExistingValueAsAddend) {
    ElfFixture fixture(1000);

    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".rodata.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".rodata.2", ElfFile::SHT_PROGBITS, 0x10, 0x10)
      .addSymbol("foo", 0x12345678, ElfFile::SHN_ABS)
      .addRelocation(2, 0, 0x04, ElfFile::R_ARM_ABS32)
      .write32(0x14, -4);


    linker.link({});

    EXPECT_EQ(segmentRead32(linker, SegmentType::rodata, 0x14), static_cast<uInt32>(0x12345674));
  }

  TEST(ElfLinker, R_ARM_ABS32_SetsBit0IfTargetIsFunction) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".rodata.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".rodata.2", ElfFile::SHT_PROGBITS, 0x10, 0x10)
      .addSymbol("foo", 0x12345678, ElfFile::SHN_ABS, ElfFile::STT_FUNC)
      .addRelocation(2, 0, 0x04, ElfFile::R_ARM_ABS32);


    linker.link({});

    EXPECT_EQ(segmentRead32(linker, SegmentType::rodata, 0x14), static_cast<uInt32>(0x12345679));
  }


  TEST(ElfLinker, R_ARM_TARGET1_InsertsTheValueAtTheTargetPosition_text) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".text.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".text.2", ElfFile::SHT_PROGBITS, 0x10, 0x10)
      .addSymbol("foo", 0x12345678, ElfFile::SHN_ABS)
      .addRelocation(2, 0, 0x04, ElfFile::R_ARM_TARGET1);

    linker.link({});

    EXPECT_EQ(segmentRead32(linker, SegmentType::text, 0x14), static_cast<uInt32>(0x12345678));
  }

  TEST(ElfLinker, R_ARM_TARGET1_InsertsTheValueAtTheTargetPosition_data) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".data.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".data.2", ElfFile::SHT_PROGBITS, 0x10, 0x10)
      .addSymbol("foo", 0x12345678, ElfFile::SHN_ABS)
      .addRelocation(2, 0, 0x04, ElfFile::R_ARM_TARGET1);

    linker.link({});

    EXPECT_EQ(segmentRead32(linker, SegmentType::data, 0x14), static_cast<uInt32>(0x12345678));
  }

  TEST(ElfLinker, R_ARM_TARGET1_InsertsTheValueAtTheTargetPosition_rodata) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".rodata.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".rodata.2", ElfFile::SHT_PROGBITS, 0x10, 0x10)
      .addSymbol("foo", 0x12345678, ElfFile::SHN_ABS)
      .addRelocation(2, 0, 0x04, ElfFile::R_ARM_TARGET1);

    linker.link({});

    EXPECT_EQ(segmentRead32(linker, SegmentType::rodata, 0x14), static_cast<uInt32>(0x12345678));
  }

  TEST(ElfLinker, R_ARM_TARGET1_AddsAddendToTarget) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".rodata.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".rodata.2", ElfFile::SHT_PROGBITS, 0x10, 0x10)
      .addSymbol("foo", 0x12345678, ElfFile::SHN_ABS)
      .addRelocation(2, 0, 0x04, ElfFile::R_ARM_TARGET1, -4);


    linker.link({});

    EXPECT_EQ(segmentRead32(linker, SegmentType::rodata, 0x14), static_cast<uInt32>(0x12345674));
  }

  TEST(ElfLinker, R_ARM_TARGET1_UsesExistingValueAsAddend) {
    ElfFixture fixture(1000);

    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".rodata.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".rodata.2", ElfFile::SHT_PROGBITS, 0x10, 0x10)
      .addSymbol("foo", 0x12345678, ElfFile::SHN_ABS)
      .addRelocation(2, 0, 0x04, ElfFile::R_ARM_TARGET1)
      .write32(0x14, -4);


    linker.link({});

    EXPECT_EQ(segmentRead32(linker, SegmentType::rodata, 0x14), static_cast<uInt32>(0x12345674));
  }

  TEST(ElfLinker, R_ARM_TARGET1_SetsBit0IfTargetIsFunction) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".rodata.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".rodata.2", ElfFile::SHT_PROGBITS, 0x10, 0x10)
      .addSymbol("foo", 0x12345678, ElfFile::SHN_ABS, ElfFile::STT_FUNC)
      .addRelocation(2, 0, 0x04, ElfFile::R_ARM_TARGET1);


    linker.link({});

    EXPECT_EQ(segmentRead32(linker, SegmentType::rodata, 0x14), static_cast<uInt32>(0x12345679));
  }

  TEST(ElfLinker, R_ARM_THM_CALL_PatchesOffset) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".text.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".text.2", ElfFile::SHT_PROGBITS, 0x10, 0x20)
      .addSymbol("foo", 0x04, 2)
      .addRelocation(1, 0, 0x08, ElfFile::R_ARM_THM_CALL)
      .write32(0x08, 0xf800f000);

    linker.link({});

    EXPECT_EQ(segmentRead32(linker, SegmentType::text, 0x08), 0xf804f000);
  }

  TEST(ElfLinker, R_ARM_THM_CALL_AddsAddendToTarget) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".text.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".text.2", ElfFile::SHT_PROGBITS, 0x10, 0x20)
      .addSymbol("foo", 0x04, 2)
      .addRelocation(1, 0, 0x08, ElfFile::R_ARM_THM_CALL, -2)
      .write32(0x08, 0xf800f000);

    linker.link({});

    EXPECT_EQ(segmentRead32(linker, SegmentType::text, 0x08), 0xf803f000);
  }

  TEST(ElfLinker, R_ARM_THM_CALL_UsesExistingValueAsAddend) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".text.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".text.2", ElfFile::SHT_PROGBITS, 0x10, 0x20)
      .addSymbol("foo", 0x04, 2)
      .addRelocation(1, 0, 0x08, ElfFile::R_ARM_THM_CALL)
      .write32(0x08, 0xfffff7ff);

    linker.link({});

    EXPECT_EQ(segmentRead32(linker, SegmentType::text, 0x08), 0xf803f000);
  }

  TEST(ElfLinker, R_ARM_THM_CALL_ThrowsIfTargetIsOutOfBounds) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".text.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".text.2", ElfFile::SHT_PROGBITS, 0x10, 0x20)
      .addSymbol("foo", 0x04, 2)
      .addRelocation(1, 0, 0x08, ElfFile::R_ARM_THM_CALL, 0x7fffff00)
      .write32(0x08, 0xfffff7ff);

    EXPECT_THROW(linker.link({}), ElfLinker::ElfLinkError);
  }

  TEST(ElfLinker, R_ARM_THM_JUMP24L_PatchesOffset) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".text.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".text.2", ElfFile::SHT_PROGBITS, 0x10, 0x20)
      .addSymbol("foo", 0x04, 2)
      .addRelocation(1, 0, 0x08, ElfFile::R_ARM_THM_JUMP24)
      .write32(0x08, 0xb800f000);

    linker.link({});

    EXPECT_EQ(segmentRead32(linker, SegmentType::text, 0x08), 0xb804f000);
  }

  TEST(ElfLinker, R_ARM_THM_JUMP24_AddsAddendToTarget) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".text.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".text.2", ElfFile::SHT_PROGBITS, 0x10, 0x20)
      .addSymbol("foo", 0x04, 2)
      .addRelocation(1, 0, 0x08, ElfFile::R_ARM_THM_JUMP24, -2)
      .write32(0x08, 0xb800f000);

    linker.link({});

    EXPECT_EQ(segmentRead32(linker, SegmentType::text, 0x08), 0xb803f000);
  }

  TEST(ElfLinker, R_ARM_THM_JUMP24_UsesExistingValueAsAddend) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".text.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".text.2", ElfFile::SHT_PROGBITS, 0x10, 0x20)
      .addSymbol("foo", 0x04, 2)
      .addRelocation(1, 0, 0x08, ElfFile::R_ARM_THM_JUMP24)
      .write32(0x08, 0xbffff7ff);

    linker.link({});

    EXPECT_EQ(segmentRead32(linker, SegmentType::text, 0x08), 0xb803f000);
  }

  TEST(ElfLinker, R_ARM_THM_JUMP24_ThrowsIfTargetIsOutOfBounds) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".text.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".text.2", ElfFile::SHT_PROGBITS, 0x10, 0x20)
      .addSymbol("foo", 0x04, 2)
      .addRelocation(1, 0, 0x08, ElfFile::R_ARM_THM_JUMP24, 0x7fffff00)
      .write32(0x08, 0xbffff7ff);

    EXPECT_THROW(linker.link({}), ElfLinker::ElfLinkError);
  }

  class RelocationExceptionTest: public testing::TestWithParam<uInt32> {};

  TEST_P(RelocationExceptionTest, RelocationThrowsIfRelocatedSymbolIsNotResolved) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".text.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".text.2", ElfFile::SHT_PROGBITS, 0x10, 0x10)
      .addSymbol("foo", 0x1234, ElfFile::SHN_UND)
      .addRelocation(2, 0, 0x04, GetParam());

    EXPECT_THROW(linker.link({}), ElfLinker::ElfLinkError);
  }

  TEST_P(RelocationExceptionTest, RelocationThrowsIfTargetIsBeyondSectionLimits) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".text.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".text.2", ElfFile::SHT_PROGBITS, 0x10, 0x10)
      .addSymbol("foo", 0x1234, ElfFile::SHN_ABS)
      .addRelocation(2, 0, 0xd, GetParam());

    EXPECT_THROW(linker.link({}), ElfLinker::ElfLinkError);
  }

  TEST_P(RelocationExceptionTest, RelocationDoesNotThrowIfTargetIsExactlyOnSectionLimit) {
    ElfFixture fixture(1000);
    ElfLinker linker(0x00100000, 0x00200000, 0x00300000, fixture);

    fixture
      .addSection(".text.1", ElfFile::SHT_PROGBITS, 0, 0x10)
      .addSection(".text.2", ElfFile::SHT_PROGBITS, 0x10, 0x10)
      .addSymbol("foo", 0x1234, ElfFile::SHN_ABS)
      .addRelocation(2, 0, 0xc, GetParam());

    EXPECT_NO_THROW(linker.link({}));
  }

  INSTANTIATE_TEST_SUITE_P(RelocationExceptionSuite, RelocationExceptionTest, testing::Values(
    ElfFile::R_ARM_ABS32, ElfFile::R_ARM_TARGET1,
    ElfFile::R_ARM_THM_CALL, ElfFile::R_ARM_THM_JUMP24
  ));

  class InitArrayTest: public testing::TestWithParam<uInt32> {
    public:
      InitArrayTest()
        : fixture(1000), linker(0x00100000, 0x00200000, 0x00300000, fixture)
      {}

    protected:
      uInt32 sectionType() const {
        return GetParam();
      }

      string sectionName() const {
        return sectionType() == ElfFile::SHT_INIT_ARRAY ? ".init_array" : ".preinit_array";
      }

      const vector<uInt32>& initArray() const {
        return sectionType() == ElfFile::SHT_INIT_ARRAY ? linker.getInitArray() : linker.getPreinitArray();
      }

      ElfFixture fixture;
      ElfLinker linker;
  };

  TEST_P(InitArrayTest, InitArrayWithBadSizeThrowsAnException) {
    fixture
      .addSection(sectionName(), sectionType(), 0, 3);

    EXPECT_THROW(linker.link({}), ElfLinker::ElfLinkError);
  }

  TEST_P(InitArrayTest, InitArrayIsReadIntoVectorLittleEndian) {
    fixture
      .addSection(sectionName(), sectionType(), 0, 0x08)
      .write32(0x00, 0x12345678)
      .write32(0x04, 0xabcdef01);

    linker.link({});

    EXPECT_EQ(initArray().size(), static_cast<size_t>(2));
    EXPECT_EQ(initArray()[0], static_cast<uInt32>(0x12345678));
    EXPECT_EQ(initArray()[1], static_cast<uInt32>(0xabcdef01));
  }

  TEST_P(InitArrayTest, R_ARM_ABS32_RelocationsApplyToInitArray) {
    fixture
      .addSection(sectionName(), sectionType(), 0, 0x08)
      .addSymbol("foo", 0xabcdef01, ElfFile::SHN_ABS)
      .addRelocation(1, 0, 0x04, ElfFile::R_ARM_ABS32);

    linker.link({});

    EXPECT_EQ(initArray()[1], static_cast<uInt32>(0xabcdef01));
  }

  TEST_P(InitArrayTest, R_ARM_ABS32_RelocationsToInitArrayApplyAddend) {
    fixture
      .addSection(sectionName(), sectionType(), 0, 0x08)
      .addSymbol("foo", 0xabcdef01, ElfFile::SHN_ABS)
      .addRelocation(1, 0, 0x04, ElfFile::R_ARM_ABS32, 0xa0);

    linker.link({});

    EXPECT_EQ(initArray()[1], static_cast<uInt32>(0xabcdefa1));
  }

  TEST_P(InitArrayTest, R_ARM_ABS32_RelocationsToInitArrayUseCurrentValueAsAddend) {
    fixture
      .addSection(sectionName(), sectionType(), 0, 0x08)
      .addSymbol("foo", 0xabcdef01, ElfFile::SHN_ABS)
      .addRelocation(1, 0, 0x04, ElfFile::R_ARM_ABS32)
      .write32(0x04, -0x0300);

    linker.link({});

    EXPECT_EQ(initArray()[1], static_cast<uInt32>(0xabcdec01));
  }

  TEST_P(InitArrayTest, R_ARM_ABS32_RelocationsToInitArrayThrowIfLocationLaysBeyondBoundary) {
    fixture
      .addSection(sectionName(), sectionType(), 0, 0x08)
      .addSymbol("foo", 0xabcdef01, ElfFile::SHN_ABS)
      .addRelocation(1, 0, 0x05, ElfFile::R_ARM_ABS32);

    EXPECT_THROW(linker.link({}), ElfLinker::ElfLinkError);
  }

  TEST_P(InitArrayTest, R_ARM_ABS32_RelocationsToInitArrayThrowIfSymbolCannotBeResolved) {
    fixture
      .addSection(sectionName(), sectionType(), 0, 0x08)
      .addSymbol("foo", 0xabcdef01, ElfFile::SHN_UND)
      .addRelocation(1, 0, 0x04, ElfFile::R_ARM_ABS32);

    EXPECT_THROW(linker.link({}), ElfLinker::ElfLinkError);
  }

  TEST_P(InitArrayTest, R_ARM_TARGET1_RelocationsApplyToInitArray) {
    fixture
      .addSection(sectionName(), sectionType(), 0, 0x08)
      .addSymbol("foo", 0xabcdef01, ElfFile::SHN_ABS)
      .addRelocation(1, 0, 0x04, ElfFile::R_ARM_TARGET1);

    linker.link({});

    EXPECT_EQ(initArray()[1], static_cast<uInt32>(0xabcdef01));
  }

  TEST_P(InitArrayTest, R_ARM_TARGET1_RelocationsToInitArrayApplyAddend) {
    fixture
      .addSection(sectionName(), sectionType(), 0, 0x08)
      .addSymbol("foo", 0xabcdef01, ElfFile::SHN_ABS)
      .addRelocation(1, 0, 0x04, ElfFile::R_ARM_TARGET1, 0xa0);

    linker.link({});

    EXPECT_EQ(initArray()[1], static_cast<uInt32>(0xabcdefa1));
  }

  TEST_P(InitArrayTest, R_ARM_TARGET1_RelocationsToInitArrayUseCurrentValueAsAddend) {
    fixture
      .addSection(sectionName(), sectionType(), 0, 0x08)
      .addSymbol("foo", 0xabcdef01, ElfFile::SHN_ABS)
      .addRelocation(1, 0, 0x04, ElfFile::R_ARM_TARGET1)
      .write32(0x04, -0x0300);

    linker.link({});

    EXPECT_EQ(initArray()[1], static_cast<uInt32>(0xabcdec01));
  }

  TEST_P(InitArrayTest, R_ARM_TARGET1_RelocationsToInitArrayThrowIfLocationLaysBeyondBoundary) {
    fixture
      .addSection(sectionName(), sectionType(), 0, 0x08)
      .addSymbol("foo", 0xabcdef01, ElfFile::SHN_ABS)
      .addRelocation(1, 0, 0x05, ElfFile::R_ARM_TARGET1);

    EXPECT_THROW(linker.link({}), ElfLinker::ElfLinkError);
  }

  TEST_P(InitArrayTest, R_ARM_TARGET1_RelocationsToInitArrayThrowIfSymbolCannotBeResolved) {
    fixture
      .addSection(sectionName(), sectionType(), 0, 0x08)
      .addSymbol("foo", 0xabcdef01, ElfFile::SHN_UND)
      .addRelocation(1, 0, 0x04, ElfFile::R_ARM_TARGET1);

    EXPECT_THROW(linker.link({}), ElfLinker::ElfLinkError);
  }

  TEST_P(InitArrayTest, R_ARM_THM_CALL_Throws) {
    fixture
      .addSection(sectionName(), sectionType(), 0, 0x08)
      .addSymbol("foo", 0x1000, ElfFile::SHN_ABS)
      .addRelocation(1, 0, 0x04, ElfFile::R_ARM_THM_CALL)
      .write32(0x04, 0xf800f000);

    EXPECT_THROW(linker.link({}), ElfLinker::ElfLinkError);
  }

  TEST_P(InitArrayTest, R_ARM_THM_JUMP24_Throws) {
    fixture
      .addSection(sectionName(), sectionType(), 0, 0x08)
      .addSymbol("foo", 0x1000, ElfFile::SHN_ABS)
      .addRelocation(1, 0, 0x04, ElfFile::R_ARM_THM_JUMP24)
      .write32(0x04, 0xf800f000);

    EXPECT_THROW(linker.link({}), ElfLinker::ElfLinkError);
  }

  INSTANTIATE_TEST_SUITE_P(InitArraySuite, InitArrayTest, testing::Values(
    ElfFile::SHT_INIT_ARRAY, ElfFile::SHT_PREINIT_ARRAY
  ));
}
