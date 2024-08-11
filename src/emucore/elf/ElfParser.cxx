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

#include "ElfParser.hxx"

#include <cstring>

namespace {
  constexpr uInt32 ELF_MAGIC = 0x7f454c46;
  constexpr uInt8 ELF_CLASS_32 = 1;
  constexpr uInt8 ELF_VERSION = 1;
  constexpr uInt32 SYMBOL_ENTRY_SIZE = 16;
  constexpr uInt32 REL_ENTRY_SIZE = 8;
  constexpr uInt32 RELA_ENTRY_SIZE = 12;
} // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ElfParser::parse(const uInt8 *elfData, size_t size)
{
  myData = elfData;
  mySize = size;

  mySections.resize(0);
  mySymbols.resize(0);
  myRelocations.clear();
  myBigEndian = true;

  try {
    if (read32(0x00) != ELF_MAGIC) ElfParseError::raise("bad magic");
    if (read8(0x04) != ELF_CLASS_32) ElfParseError::raise("not 32bit ELF");
    if (read8(0x06) != ELF_VERSION) ElfParseError::raise("invalid ELF version");

    myHeader.endianess = read8(0x05);
    myBigEndian = myHeader.endianess == ENDIAN_BIG_ENDIAN;

    if (read32(0x14) != ELF_VERSION) ElfParseError::raise("inconsistent ELF version");

    myHeader.type = read16(0x10);
    myHeader.arch = read16(0x12);

    myHeader.shOffset = read32(0x20);
    myHeader.shSize = read16(0x2e);
    myHeader.shNum = read16(0x30);
    myHeader.shstrIndex = read16(0x32);

    if (myHeader.shstrIndex >= myHeader.shNum) ElfParseError::raise(".shstrtab out of range");

    mySections.reserve(myHeader.shNum);

    for (uInt32 i = 0; i < myHeader.shNum; i++)
      mySections.push_back(
          readSection(myHeader.shOffset + i * myHeader.shSize));

    const Section &shrstrtab(mySections[myHeader.shstrIndex]);

    if (shrstrtab.type != SHT_STRTAB) ElfParseError::raise(".shstrtab has wrong type");

    for (Section &section : mySections)
      section.name = getName(shrstrtab, section.nameOffset);

    const Section* symtab = getSymtab();

    if (symtab) {
      const Section* strtab = getStrtab();
      if (!strtab) ElfParseError::raise("no string table to resolve symbol names");

      mySymbols.reserve(symtab->size / SYMBOL_ENTRY_SIZE);

      for (uInt32 i = 0; i < symtab->size / SYMBOL_ENTRY_SIZE; i++)
        mySymbols.push_back(readSymbol(i, *symtab, *strtab));
    }

    for (const auto& section: mySections) {
      if (section.type != SHT_REL && section.type != SHT_RELA) continue;
      if (section.info >= mySections.size()) ElfParseError::raise("relocation table for invalid section");

      vector<Relocation> rels;
      const size_t relocationCount = section.size / (section.type == SHT_REL ? REL_ENTRY_SIZE : RELA_ENTRY_SIZE);
      rels.reserve(section.size / relocationCount);

      for (uInt32 i = 0; i < relocationCount; i++) {
        Relocation rel = readRelocation(i, section);

        if (rel.symbol >= mySymbols.size()) ElfParseError::raise("invalid relocation symbol");
        rel.symbolName = mySymbols[rel.symbol].name;

        rels.push_back(rel);
      }

      myRelocations[section.info] = rels;
    }
  } catch (const ElfParseError &e) {
    ElfParseError::raise("failed to parse ELF: " + string(e.what()));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* ElfParser::getData() const { return myData; }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t ElfParser::getSize() const { return mySize; }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const vector<ElfParser::Section> &ElfParser::getSections() const
{
  return mySections;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const vector<ElfParser::Symbol>& ElfParser::getSymbols() const
{
  return mySymbols;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
optional<vector<ElfParser::Relocation>> ElfParser::getRelocations(size_t section) const
{
  return myRelocations.contains(section) ? myRelocations.at(section) : optional<vector<Relocation>>();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 ElfParser::read8(uInt32 offset) const
{
  if (offset >= mySize)
    ElfParseError::raise("reference beyond bounds");

  return myData[offset];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 ElfParser::read16(uInt32 offset) const
{
  return myBigEndian
    ? ((read8(offset) << 8) | read8(offset + 1))
    : ((read8(offset + 1) << 8) | read8(offset));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 ElfParser::read32(uInt32 offset) const
{
  return myBigEndian
    ? ((read8(offset) << 24) | (read8(offset + 1) << 16) |
      (read8(offset + 2) << 8) | read8(offset + 3))
    : ((read8(offset + 3) << 24) | (read8(offset + 2) << 16) |
      (read8(offset + 1) << 8) | read8(offset));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ElfParser::Section ElfParser::readSection(uInt32 offset) const {
  Section section;

  try {
    section.nameOffset = read32(offset);
    section.type = read32(offset + 0x04);
    section.flags = read32(offset + 0x08);
    section.virtualAddress = read32(offset + 0x0c);
    section.offset = read32(offset + 0x10);
    section.size = read32(offset + 0x14);
    section.info = read32(offset + 0x1c);
    section.align = read32(offset + 0x20);

    if (section.offset + section.size >= mySize && section.type != SHT_NOBITS)
      ElfParseError::raise("section  exceeds bounds");

  } catch (const ElfParseError &e) {
    ElfParseError::raise("failed to read section: " + string(e.what()));
  }

  return section;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ElfParser::Symbol ElfParser::readSymbol(uInt32 index, const Section& symSec, const Section& strSec) const
{
  uInt32 offset = index * SYMBOL_ENTRY_SIZE;
  if (offset + SYMBOL_ENTRY_SIZE > symSec.size) ElfParseError::raise("symbol is beyond section");
  offset += symSec.offset;

  Symbol sym;

  try {
    sym.nameOffset = read32(offset);
    sym.value = read32(offset + 0x04);
    sym.size = read32(offset + 0x08);
    sym.info = read8(offset + 0x0c);
    sym.visibility = read8(offset + 0x0d);
    sym.section = read16(offset + 0x0e);
  } catch (const ElfParseError& e) {
    ElfParseError::raise("failed to read section: " + string(e.what()));
  }

  if (
      ((sym.section != SHN_ABS && sym.section != SHN_UND) || sym.type == STT_SECTION) &&
      sym.section >= mySections.size()
  )
    ElfParseError::raise("symbol: section index out of range");

  sym.bind = sym.info >> 4;
  sym.type = sym.info & 0x0f;

  sym.name = sym.type == STT_SECTION ? mySections[sym.section].name : getName(strSec, sym.nameOffset);

  return sym;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ElfParser::Relocation ElfParser::readRelocation(uInt32 index, const Section& sec) const
{
  if (sec.type != SHT_REL && sec.type != SHT_RELA)
    throw runtime_error("section is not RELA or REL");

  const uInt32 size = sec.type == SHT_REL ? REL_ENTRY_SIZE : RELA_ENTRY_SIZE;
  uInt32 offset = index * size;

  if (offset + size > sec.size) ElfParseError::raise("relocation is beyond bounds");

  offset += sec.offset;
  Relocation rel;

  try {
    rel.offset = read32(offset);
    rel.info = read32(offset + 0x04);
    rel.addend = sec.type == SHT_RELA ? read32(offset + 0x08) : optional<uInt32>();
  } catch (const ElfParseError& e) {
    ElfParseError::raise("failed to read relocation: " + string(e.what()));
  }

  rel.symbol = rel.info >> 8;
  rel.type = rel.info & 0xff;

  if (rel.symbol >=mySymbols.size())
    ElfParseError::raise("bad relocation: symbol out of bounds");

  return rel;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* ElfParser::getName(const Section& section, uInt32 offset) const
{
  if (offset >= section.size) ElfParseError::raise("name out of bounds");
  const uInt32 imageOffset = offset + section.offset;

  const char *name = reinterpret_cast<const char *>(myData + imageOffset);

  if (myData[imageOffset + strnlen(name, section.size - offset)] != '\0')
    ElfParseError::raise("unterminated section name");

  return name;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ElfParser::Section* ElfParser::getSymtab() const
{
  for (const auto& section: mySections)
    if (section.type == SHT_SYMTAB) return &section;

  return nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ElfParser::Section* ElfParser::getStrtab() const
{
  const Section* strtab = nullptr;
  size_t count = 0;

  for (size_t i = 0; i < mySections.size(); i++) {
    if (mySections[i].type != SHT_STRTAB || i == myHeader.shstrIndex) continue;

    strtab = &mySections[i];
    count++;
  }

  if (count > 1) ElfParseError::raise("more than one symbol table");

  return strtab;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream& operator<<(ostream& os, const ElfParser::Section& section)
{
  std::ios reset(nullptr);
  reset.copyfmt(os);

  os
    << std::hex << std::setw(4) << std::setfill('0')
    << section.name
    << " type=0x" << section.type
    << " flags=0x" << section.flags
    << " vaddr=0x" << section.virtualAddress
    << " offset=0x" << section.offset;

  os.copyfmt(reset);

  os
    << " size=" << section.size
    << " info=" << section.info
    << " align=" << section.align;

  return os;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream& operator<<(ostream& os, const ElfParser::Symbol& symbol)
{
  std::ios reset(nullptr);
  reset.copyfmt(os);

  os
    << symbol.nameOffset << " "
    << symbol.name
    << std::hex << std::setfill('0')
    << " value=0x" << std::setw(8) << symbol.value
    << " size=0x" << std::setw(8) << symbol.size
    << std::setw(1)
    << " bind=0x" << std::setw(2) << static_cast<int>(symbol.bind)
    << " type=0x" << std::setw(2) << static_cast<int>(symbol.type);

  os.copyfmt(reset);

  os << " section=" << symbol.section;

  return os;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream& operator<<(ostream& os, const ElfParser::Relocation& rel)
{
  std::ios reset(nullptr);
  reset.copyfmt(os);

  os
    << rel.symbolName << " :"
    << std::hex << std::setfill('0')
    << " offset=0x" << std::setw(8) << rel.offset
    << " info=0x" << std::setw(8) << rel.info
    << " type=0x" << std::setw(2) << static_cast<int>(rel.type);

  if (rel.addend.has_value())
    os << " addend=0x" << std::setw(8) << *rel.addend;

  os.copyfmt(reset);

  os << " symbol=" << static_cast<int>(rel.symbol);

  return os;
}
