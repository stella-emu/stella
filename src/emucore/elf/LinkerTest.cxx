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

  class ElfFixture: public ElfFile {
    public:
      explicit ElfFixture(size_t size) : mySize(size) {
        myData = make_unique<uInt8[]>(mySize);
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

      ElfFixture& write8(size_t address, uInt8 value) {
        myData[address] = value;

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

    EXPECT_EQ(linker.getSegmentSize(ElfLinker::SegmentType::text), static_cast<uInt32>(0));
    EXPECT_EQ(linker.getSegmentSize(ElfLinker::SegmentType::data), static_cast<uInt32>(0));
    EXPECT_EQ(linker.getSegmentSize(ElfLinker::SegmentType::rodata), static_cast<uInt32>(0));
  }

  TEST(ElfLinker, TextSegmentsGoToText) {
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

    EXPECT_EQ(linker.getSegmentSize(ElfLinker::SegmentType::text), static_cast<uInt32>(78));
    EXPECT_EQ(linker.getSegmentSize(ElfLinker::SegmentType::data), static_cast<uInt32>(0));
    EXPECT_EQ(linker.getSegmentSize(ElfLinker::SegmentType::rodata), static_cast<uInt32>(0));

    EXPECT_EQ(linker.getRelocatedSections()[0]->offset, static_cast<uInt32>(0));
    EXPECT_EQ(linker.getRelocatedSections()[0]->segment, ElfLinker::SegmentType::text);

    EXPECT_EQ(linker.getRelocatedSections()[1]->offset, static_cast<uInt32>(12));
    EXPECT_EQ(linker.getRelocatedSections()[1]->segment, ElfLinker::SegmentType::text);

    EXPECT_EQ(linker.getRelocatedSections()[2]->offset, static_cast<uInt32>(34));
    EXPECT_EQ(linker.getRelocatedSections()[2]->segment, ElfLinker::SegmentType::text);

    EXPECT_EQ(linker.getRelocatedSections()[3]->offset, static_cast<uInt32>(67));
    EXPECT_EQ(linker.getRelocatedSections()[3]->segment, ElfLinker::SegmentType::text);

    EXPECT_EQ(linker.getSegmentData(ElfLinker::SegmentType::text)[0], 0x01);
    EXPECT_EQ(linker.getSegmentData(ElfLinker::SegmentType::text)[12], 0x02);
    EXPECT_EQ(linker.getSegmentData(ElfLinker::SegmentType::text)[34], 0x03);
    EXPECT_EQ(linker.getSegmentData(ElfLinker::SegmentType::text)[67], 0x04);
  }

  TEST(ElfLinker, DataSegmentsGoToData) {
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

    EXPECT_EQ(linker.getSegmentSize(ElfLinker::SegmentType::text), static_cast<uInt32>(0));
    EXPECT_EQ(linker.getSegmentSize(ElfLinker::SegmentType::data), static_cast<uInt32>(78));
    EXPECT_EQ(linker.getSegmentSize(ElfLinker::SegmentType::rodata), static_cast<uInt32>(0));

    EXPECT_EQ(linker.getRelocatedSections()[0]->offset, static_cast<uInt32>(0));
    EXPECT_EQ(linker.getRelocatedSections()[0]->segment, ElfLinker::SegmentType::data);

    EXPECT_EQ(linker.getRelocatedSections()[1]->offset, static_cast<uInt32>(12));
    EXPECT_EQ(linker.getRelocatedSections()[1]->segment, ElfLinker::SegmentType::data);

    EXPECT_EQ(linker.getRelocatedSections()[2]->offset, static_cast<uInt32>(34));
    EXPECT_EQ(linker.getRelocatedSections()[2]->segment, ElfLinker::SegmentType::data);

    EXPECT_EQ(linker.getRelocatedSections()[3]->offset, static_cast<uInt32>(67));
    EXPECT_EQ(linker.getRelocatedSections()[3]->segment, ElfLinker::SegmentType::data);

    EXPECT_EQ(linker.getSegmentData(ElfLinker::SegmentType::data)[0], 0x01);
    EXPECT_EQ(linker.getSegmentData(ElfLinker::SegmentType::data)[12], 0x02);
    EXPECT_EQ(linker.getSegmentData(ElfLinker::SegmentType::data)[34], 0x03);
    EXPECT_EQ(linker.getSegmentData(ElfLinker::SegmentType::data)[67], 0x04);
  }

TEST(ElfLinker, RodataSegmentsGoToRodata) {
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

    EXPECT_EQ(linker.getSegmentSize(ElfLinker::SegmentType::text), static_cast<uInt32>(0));
    EXPECT_EQ(linker.getSegmentSize(ElfLinker::SegmentType::data), static_cast<uInt32>(0));
    EXPECT_EQ(linker.getSegmentSize(ElfLinker::SegmentType::rodata), static_cast<uInt32>(78));

    EXPECT_EQ(linker.getRelocatedSections()[0]->offset, static_cast<uInt32>(0));
    EXPECT_EQ(linker.getRelocatedSections()[0]->segment, ElfLinker::SegmentType::rodata);

    EXPECT_EQ(linker.getRelocatedSections()[1]->offset, static_cast<uInt32>(12));
    EXPECT_EQ(linker.getRelocatedSections()[1]->segment, ElfLinker::SegmentType::rodata);

    EXPECT_EQ(linker.getRelocatedSections()[2]->offset, static_cast<uInt32>(34));
    EXPECT_EQ(linker.getRelocatedSections()[2]->segment, ElfLinker::SegmentType::rodata);

    EXPECT_EQ(linker.getRelocatedSections()[3]->offset, static_cast<uInt32>(67));
    EXPECT_EQ(linker.getRelocatedSections()[3]->segment, ElfLinker::SegmentType::rodata);

    EXPECT_EQ(linker.getSegmentData(ElfLinker::SegmentType::rodata)[0], 0x01);
    EXPECT_EQ(linker.getSegmentData(ElfLinker::SegmentType::rodata)[12], 0x02);
    EXPECT_EQ(linker.getSegmentData(ElfLinker::SegmentType::rodata)[34], 0x03);
    EXPECT_EQ(linker.getSegmentData(ElfLinker::SegmentType::rodata)[67], 0x04);
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

    EXPECT_EQ(linker.getSegmentSize(ElfLinker::SegmentType::text), static_cast<uInt32>(0));
    EXPECT_EQ(linker.getSegmentSize(ElfLinker::SegmentType::data), static_cast<uInt32>(76));
    EXPECT_EQ(linker.getSegmentSize(ElfLinker::SegmentType::rodata), static_cast<uInt32>(0));

    EXPECT_EQ(linker.getRelocatedSections()[0]->offset, static_cast<uInt32>(0));
    EXPECT_EQ(linker.getRelocatedSections()[0]->segment, ElfLinker::SegmentType::data);

    EXPECT_EQ(linker.getRelocatedSections()[1]->offset, static_cast<uInt32>(44));
    EXPECT_EQ(linker.getRelocatedSections()[1]->segment, ElfLinker::SegmentType::data);

    EXPECT_EQ(linker.getRelocatedSections()[2]->offset, static_cast<uInt32>(10));
    EXPECT_EQ(linker.getRelocatedSections()[2]->segment, ElfLinker::SegmentType::data);

    EXPECT_EQ(linker.getRelocatedSections()[3]->offset, static_cast<uInt32>(65));
    EXPECT_EQ(linker.getRelocatedSections()[3]->segment, ElfLinker::SegmentType::data);

    EXPECT_EQ(linker.getSegmentData(ElfLinker::SegmentType::data)[0], 0x01);
    EXPECT_EQ(linker.getSegmentData(ElfLinker::SegmentType::data)[10], 0x02);
    EXPECT_EQ(linker.getSegmentData(ElfLinker::SegmentType::data)[44], 0);
    EXPECT_EQ(linker.getSegmentData(ElfLinker::SegmentType::data)[65], 0);
  }
}
