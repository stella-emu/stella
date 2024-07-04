#include "ElfParser.hxx"

#include <cstring>

namespace {
  constexpr uInt32 ELF_MAGIC = 0x7f454c46;
  constexpr uInt8 ELF_CLASS_32 = 1;
  constexpr uInt8 ELF_VERSION = 1;
  constexpr uInt32 SYMBOL_ENTRY_SIZE = 16;
} // namespace

void ElfParser::parse(const uInt8 *elfData, size_t size)
{
  data = elfData;
  this->size = size;

  sections.resize(0);

  try {
    if (read32(0x00) != ELF_MAGIC) EInvalidElf::raise("bad magic");
    if (read8(0x04) != ELF_CLASS_32) EInvalidElf::raise("not 32bit ELF");
    if (read8(0x06) != ELF_VERSION) EInvalidElf::raise("invalid ELF version");

    header.endianess = read8(0x05);
    bigEndian = header.endianess == ENDIAN_BIG_ENDIAN;

    if (read32(0x14) != ELF_VERSION) EInvalidElf::raise("inconsistent ELF version");

    header.type = read16(0x10);
    header.arch = read16(0x12);

    header.shOffset = read32(0x20);
    header.shSize = read16(0x2e);
    header.shNum = read16(0x30);
    header.shstrIndex = read16(0x32);

    if (header.shstrIndex >= header.shNum) EInvalidElf::raise(".shstrtab out of range");

    sections.reserve(header.shNum);

    for (size_t i = 0; i < header.shNum; i++)
      sections.push_back(
          readSection(header.shOffset + i * header.shSize));

    const Section &shrstrtab(sections[header.shstrIndex]);

    if (shrstrtab.type != SHT_STRTAB) EInvalidElf::raise(".shstrtab has wrong type");

    for (Section &section : sections)
      section.name = getName(shrstrtab, section.nameOffset);

    const Section* symtab = getSymtab();
    if (symtab) {
      const Section* strtab = getStrtab();
      if (!strtab) EInvalidElf::raise("no string table to resolve symbol names");

      symbols.reserve(symtab->size / SYMBOL_ENTRY_SIZE);

      for (size_t i = 0; i < symtab->size / SYMBOL_ENTRY_SIZE; i++)
        symbols.push_back(readSymbol(i, *symtab, *strtab));
    }
  } catch (const EInvalidElf &e) {
    EInvalidElf::raise("failed to parse ELF: " + string(e.what()));
  }
}

const uInt8 *ElfParser::getData() const { return data; }

size_t ElfParser::getSize() const { return size; }

const vector<ElfParser::Section> &ElfParser::getSections() const
{
  return sections;
}

const vector<ElfParser::Symbol>& ElfParser::getSymbols() const
{
  return symbols;
}

const optional<ElfParser::Section>
ElfParser::getSection(const string &name) const {
  for (const Section &section : sections)
    if (section.name == name)
      return section;

  return optional<Section>();
}

uInt8 ElfParser::read8(uInt32 offset) const
{
  if (offset >= size)
    EInvalidElf::raise("reference beyond bounds");

  return data[offset];
}

uInt16 ElfParser::read16(uInt32 offset)  const
{
  return bigEndian ? ((read8(offset) << 8) | read8(offset + 1))
                   : ((read8(offset + 1) << 8) | read8(offset));
}

uInt32 ElfParser::read32(uInt32 offset) const
{
  return bigEndian ? ((read8(offset) << 24) | (read8(offset + 1) << 16) |
                      (read8(offset + 2) << 8) | read8(offset + 3))
                   : ((read8(offset + 3) << 24) | (read8(offset + 2) << 16) |
                      (read8(offset + 1) << 8) | read8(offset));
}

ElfParser::Section ElfParser::readSection(uInt32 offset) const {
  Section section;

  try {
    section.nameOffset = read32(offset);
    section.type = read32(offset + 0x04);
    section.flags = read32(offset + 0x08);
    section.virtualAddress = read32(offset + 0x0c);
    section.offset = read32(offset + 0x10);
    section.size = read32(offset + 0x14);
    section.align = read32(offset + 0x20);

    if (section.offset + section.size >= size)
      EInvalidElf::raise("section exceeds bounds");
  } catch (const EInvalidElf &e) {
    EInvalidElf::raise("failed to parse section: " + string(e.what()));
  }

  return section;
}

ElfParser::Symbol ElfParser::readSymbol(uInt32 index, const Section& symSec, const Section& strSec) const
{
  Symbol sym;

  uInt32 offset = index * SYMBOL_ENTRY_SIZE;
  if (offset + SYMBOL_ENTRY_SIZE > symSec.size) EInvalidElf::raise("symbol is beyond section");
  offset += symSec.offset;

  sym.nameOffset = read32(offset);
  sym.value = read32(offset + 0x04);
  sym.size = read32(offset + 0x08);
  sym.info = read8(offset + 0x0c);
  sym.visibility = read8(offset + 0x0d);
  sym.section = read16(offset + 0x0e);

  sym.name = getName(strSec, sym.nameOffset);
  sym.bind = sym.info >> 4;
  sym.type = sym.info & 0x0f;

  return sym;
}

const char* ElfParser::getName(const Section& section, uInt32 offset) const
{
  if (offset >= section.size) EInvalidElf::raise("name out of bounds");
  const uInt32 imageOffset = offset + section.offset;

  const char *name = reinterpret_cast<const char *>(data + imageOffset);

  if (data[imageOffset + strnlen(name, section.size - offset)] != '\0')
    EInvalidElf::raise("unterminated section name");

  return name;
}

const ElfParser::Section* ElfParser::getSymtab() const
{
  for (auto& section: sections)
    if (section.type == SHT_SYMTAB) return &section;

  return nullptr;
}

const ElfParser::Section* ElfParser::getStrtab() const
{
  for (size_t i = 0; i < sections.size(); i++)
      if (sections[i].type == SHT_STRTAB && i != header.shstrIndex) return &sections[i];

   return nullptr;
}

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
    << " align=" << section.align;

  return os;
}

ostream& operator<<(ostream& os, const ElfParser::Symbol symbol)
{
  std::ios reset(nullptr);
  reset.copyfmt(os);

  os
    << symbol.name
    << std::hex << std::setw(4) << std::setfill('0')
    << " value=0x" << symbol.value
    << " size=0x" << symbol.size
    << std::setw(1)
    << " bind=0x" << (int)symbol.bind
    << " type=0x" << (int)symbol.type;

  os.copyfmt(reset);

  os << " section=" << symbol.section;

  return os;
}
