#include "ElfParser.hxx"

#include <cstring>

namespace {
  constexpr uInt32 ELF_MAGIC = 0x7f454c46;
  constexpr uInt8 ELF_CLASS_32 = 1;
  constexpr uInt8 ELF_VERSION = 1;
} // namespace

ElfParser::EInvalidElf::EInvalidElf(const string &reason) : reason(reason) {}

const string &ElfParser::EInvalidElf::getReason() const { return reason; }

void ElfParser::parse(const uInt8 *elfData, size_t size) {
  data = elfData;
  this->size = size;

  sections.resize(0);

  try {
    if (read32(0x00) != ELF_MAGIC) throw EInvalidElf("bad magic");
    if (read8(0x04) != ELF_CLASS_32) throw EInvalidElf("not 32bit ELF");
    if (read8(0x06) != ELF_VERSION) throw EInvalidElf("invalid ELF version");

    header.endianess = read8(0x05);
    bigEndian = header.endianess == ENDIAN_BIG_ENDIAN;

    if (read32(0x14) != ELF_VERSION) throw EInvalidElf("inconsistent ELF version");

    header.type = read16(0x10);
    header.arch = read16(0x12);

    header.shOffset = read32(0x20);
    header.shSize = read16(0x2e);
    header.shNum = read16(0x30);
    header.shstrIndex = read16(0x32);

    if (header.shstrIndex >= header.shNum) throw EInvalidElf(".shstrtab out of range");

    sections.reserve(header.shNum);

    for (uInt32 i = 0; i < header.shNum; i++)
      sections.push_back(
          readSection(header.shOffset + i * header.shSize));

    const Section &shrstrtab(sections[header.shstrIndex]);

    if (shrstrtab.type != SHT_STRTAB) throw new EInvalidElf(".shstrtab has wrong type");


    for (Section &section : sections)
      section.name = getName(shrstrtab, section.nameOffset);

  } catch (const EInvalidElf &e) {
    throw EInvalidElf("failed to parse ELF: " + e.getReason());
  }
}

const uInt8 *ElfParser::getData() const { return data; }

size_t ElfParser::getSize() const { return size; }

const vector<ElfParser::Section> &ElfParser::getSections() const {
  return sections;
}

const optional<ElfParser::Section>
ElfParser::getSection(const string &name) const {
  for (const Section &section : sections)
    if (section.name == name)
      return section;

  return optional<Section>();
}

uInt8 ElfParser::read8(uInt32 offset) {
  if (offset >= size)
    throw EInvalidElf("reference beyond bounds");

  return data[offset];
}

uInt16 ElfParser::read16(uInt32 offset) {
  return bigEndian ? ((read8(offset) << 8) | read8(offset + 1))
                   : ((read8(offset + 1) << 8) | read8(offset));
}

uInt32 ElfParser::read32(uInt32 offset) {
  return bigEndian ? ((read8(offset) << 24) | (read8(offset + 1) << 16) |
                      (read8(offset + 2) << 8) | read8(offset + 3))
                   : ((read8(offset + 3) << 24) | (read8(offset + 2) << 16) |
                      (read8(offset + 1) << 8) | read8(offset));
}

ElfParser::Section ElfParser::readSection(uInt32 offset) {
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
      throw EInvalidElf("section exceeds bounds");
  } catch (const EInvalidElf &e) {
    throw EInvalidElf("failed to parse section: " + e.getReason());
  }

  return section;
}

const char* ElfParser::getName(const Section& section, uInt32 offset)
{
  if (offset >= section.size) throw EInvalidElf("name out of bounds");
  const uInt32 imageOffset = offset + section.offset;

  const char *name = reinterpret_cast<const char *>(data + imageOffset);

  if (data[imageOffset + strnlen(name, section.size - offset)] != '\0')
    throw new EInvalidElf("unterminated section name");

  return name;
}
