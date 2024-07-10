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

#include <unordered_map>

#include "ElfUtil.hxx"

#include "ElfLinker.hxx"

ElfLinker::ElfLinker(uInt32 textBase, uInt32 dataBase, const ElfParser& parser)
  : myTextBase(textBase), myDataBase(dataBase), myParser(parser)
{}

void ElfLinker::link(const vector<ExternalSymbol>& externalSymbols)
{
  myTextSize = myDataSize = 0;
  myTextData.reset();
  myDataData.reset();
  myRelocatedSections.resize(0);
  myRelocatedSymbols.resize(0);

  auto& sections = myParser.getSections();
  myRelocatedSections.resize(sections.size(), std::nullopt);

  // relocate all .text and .data sections
  for (size_t i = 0; i < sections.size(); i++) {
    const auto& section = sections[i];

    if (section.type == ElfParser::SHT_PROGBITS) {
      const bool isText = section.name.starts_with(".text");
      uInt32& segmentSize = isText ? myTextSize : myDataSize;

      if (segmentSize % section.align)
        segmentSize = (segmentSize / section.align + 1) * section.align;

      myRelocatedSections[i] = {isText ? SegmentType::text : SegmentType::data, segmentSize};
      segmentSize += section.size;
    }
  }

  // relocate all .bss sections
  for (size_t i = 0; i < sections.size(); i++) {
    const auto& section = sections[i];

    if (section.type == ElfParser::SHT_NOBITS) {
      if (myDataSize % section.align)
        myDataSize = (myDataSize / section.align + 1) * section.align;

      myRelocatedSections[i] = {SegmentType::data, myDataSize};
      myDataSize += section.size;
    }
  }

  // ensure that the segments don't overlap
  if (!(myTextBase + myTextSize <= myDataBase || myDataBase + myDataSize <= myTextBase))
    ElfLinkError::raise("segments overlap");

  // allocate and copy section data
  myTextData = make_unique<uInt8[]>(myTextSize);
  myDataData = make_unique<uInt8[]>(myDataSize);

  std::memset(myTextData.get(), 0, myTextSize);
  std::memset(myDataData.get(), 0, myDataSize);

  for (size_t i = 0; i < sections.size(); i++) {
    const auto& relocatedSection = myRelocatedSections[i];
    if (!relocatedSection) continue;

    const auto& section = sections[i];
    if (section.type != ElfParser::SHT_PROGBITS) continue;

    const bool isText = section.name.starts_with(".text");

    std::memcpy(
      (isText ? myTextData : myDataData).get() + relocatedSection->offset,
      myParser.getData() + section.offset,
      section.size
    );
  }

  std::unordered_map<string, const ExternalSymbol*> externalSymbolLookup;
  for (const auto& externalSymbol: externalSymbols)
    externalSymbolLookup[externalSymbol.name] = &externalSymbol;

  // relocate symbols
  const auto& symbols = myParser.getSymbols();
  myRelocatedSymbols.resize(symbols.size(), std::nullopt);

  for (size_t i = 0; i < symbols.size(); i++) {
    const auto& symbol = symbols[i];

    if (symbol.section == ElfParser::SHN_ABS) {
      myRelocatedSymbols[i] = {std::nullopt, symbol.value};

      continue;
    }

    if (symbol.section == ElfParser::SHN_UND) {
      if (externalSymbolLookup.contains(symbol.name))
        myRelocatedSymbols[i] = {std::nullopt, externalSymbolLookup[symbol.name]->value};

      continue;
    }

    const auto& relocatedSection = myRelocatedSections[symbol.section];
    if (!relocatedSection) continue;

    uInt32 value = relocatedSection->segment == SegmentType::text ? myTextBase : myDataBase;
    value += relocatedSection->offset;
    if (symbol.type != ElfParser::STT_SECTION) value += symbol.value;

    myRelocatedSymbols[i] = {relocatedSection->segment, value};
  }

  // apply relocations
  for (size_t iSection = 0; iSection < sections.size(); iSection++) {
    const auto& relocations = myParser.getRelocations(iSection);
    if (!relocations) continue;
    if (!myRelocatedSections[iSection]) continue;

    for (const auto& relocation: *relocations)
      applyRelocation(relocation, iSection);
  }
}

uInt32 ElfLinker::getTextBase() const
{
  return myTextBase;
}

uInt32 ElfLinker::getTextSize() const
{
  return myTextSize;
}

const uInt8* ElfLinker::getTextData() const
{
  return myTextData ? myTextData.get() : nullptr;
}

uInt32 ElfLinker::getDataBase() const
{
  return myDataBase;
}

uInt32 ElfLinker::getDataSize() const
{
  return myDataSize;
}

const uInt8* ElfLinker::getDataData() const
{
  return myDataData ? myDataData.get() : nullptr;
}

const vector<uInt32> ElfLinker::getInitArray() const
{
  throw runtime_error("not implemented");
}

const vector<uInt32> ElfLinker::getPreinitArray() const
{
  throw runtime_error("not implemented");
}

ElfLinker::RelocatedSymbol ElfLinker::findRelocatedSymbol(string_view name) const
{
  const auto& symbols = myParser.getSymbols();
  for (size_t i = 0; i < symbols.size(); i++) {
    if (symbols[i].name != name) continue;

    if (!myRelocatedSymbols[i])
      ElfSymbolResolutionError::raise("symbol could not be relocated");

    return *myRelocatedSymbols[i];
  }

  ElfSymbolResolutionError::raise("symbol not found");
}

const vector<std::optional<ElfLinker::RelocatedSection>>& ElfLinker::getRelocatedSections() const
{
  return myRelocatedSections;
}

const vector<std::optional<ElfLinker::RelocatedSymbol>>& ElfLinker::getRelocatedSymbols() const
{
  return myRelocatedSymbols;
}

void ElfLinker::applyRelocation(const ElfParser::Relocation& relocation, size_t iSection)
{
  const auto& targetSection = myParser.getSections()[iSection];
  const auto& targetSectionRelocated = *myRelocatedSections[iSection];
  const auto& symbol = myParser.getSymbols()[relocation.symbol];
  const auto& relocatedSymbol = myRelocatedSymbols[relocation.symbol];

  if (!relocatedSymbol)
    ElfLinkError::raise(
      "unable to relocate " + symbol.name + " in " + targetSection.name + ": symbol could not be relocated"
    );

  if (relocation.offset + 4 > targetSection.size)
    ElfLinkError::raise(
      "unable to relocate " + symbol.name + " in " + targetSection.name + ": target out of range"
    );

  uInt8* target =
    (targetSectionRelocated.segment == SegmentType::text ? myTextData : myDataData).get() +
    targetSectionRelocated.offset + relocation.offset;

  switch (relocation.type) {
    case ElfParser::R_ARM_ABS32:
    case ElfParser::R_ARM_TARGET1:
      {
        const uInt32 value = relocatedSymbol->value + relocation.addend.value_or(read32(target));
        write32(target, value | (symbol.type == ElfParser::STT_FUNC ? 0x01 : 0));

        break;
      }

    case ElfParser::R_ARM_THM_CALL:
    case ElfParser::R_ARM_THM_JUMP24:
      {
        const uInt32 op = read32(target);

        Int32 offset = relocatedSymbol->value + relocation.addend.value_or(elfUtil::decode_B_BL(op)) -
          (targetSectionRelocated.segment == SegmentType::text ? myTextBase : myDataBase) -
          targetSectionRelocated.offset - relocation.offset - 4;

        if ((offset >> 24) != -1 && (offset >> 24) != 0)
          ElfLinkError::raise("unable to relocate jump: offset out of bounds");

        write32(target, elfUtil::encode_B_BL(offset, relocation.type == ElfParser::R_ARM_THM_CALL));
      }
  }
}

uInt32 ElfLinker::read32(const uInt8* address)
{
  uInt32 value = *(address++);
  value |= *(address++) << 8;
  value |= *(address++) << 16;
  value |= *(address++) << 24;

  return value;
}

void ElfLinker::write32(uInt8* address, uInt32 value)
{
  *(address++) = value;
  *(address++) = value >> 8;
  *(address++) = value >> 16;
  *(address++) = value >> 24;
}
