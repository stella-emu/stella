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
#include "Logger.hxx"

#include "ElfLinker.hxx"

namespace {
  optional<ElfLinker::SegmentType> determineSegmentType(const ElfFile::Section& section)
  {
    switch (section.type) {
      case ElfFile::SHT_PROGBITS:
        if (section.name.starts_with(".text")) return ElfLinker::SegmentType::text;

        if (section.name.starts_with(".rodata")) return ElfLinker::SegmentType::rodata;

        return ElfLinker::SegmentType::data;

      case ElfFile::SHT_NOBITS:
        return ElfLinker::SegmentType::data;

      default:
        return std::nullopt;
    }
  }

  constexpr bool checkSegmentOverlap(uInt32 segmentBase1, uInt32 segmentSize1, uInt32 segmentBase2, uInt32 segmentSize2) {
    return !(segmentBase1 + segmentSize1 <= segmentBase2 || segmentBase2 + segmentSize2 <= segmentBase1);
  }
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ElfLinker::ElfLinker(uInt32 textBase, uInt32 dataBase, uInt32 rodataBase,
                     const ElfFile& elf)
  : myTextBase{textBase},
    myDataBase{dataBase},
    myRodataBase{rodataBase},
    myElf{elf}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ElfLinker& ElfLinker::setUndefinedSymbolDefault(uInt32 defaultValue)
{
  undefinedSymbolDefault = defaultValue;

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ElfLinker::link(const vector<ExternalSymbol>& externalSymbols)
{
  myTextSize = myDataSize = myRodataSize = 0;
  myTextData.reset();
  myDataData.reset();
  myRodataData.reset();
  myRelocatedSections.resize(0);

  relocateSections();

  relink(externalSymbols);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ElfLinker::relink(const vector<ExternalSymbol>& externalSymbols)
{
  myRelocatedSymbols.resize(0);
  myInitArray.resize(0);
  myPreinitArray.resize(0);

  copySections();
  relocateSymbols(externalSymbols);
  relocateInitArrays();
  applyRelocationsToSections();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 ElfLinker::getSegmentSize(SegmentType type) const
{
  switch (type) {
    case SegmentType::text:
      return myTextSize;

    case SegmentType::data:
      return myDataSize;

    case SegmentType::rodata:
      return myRodataSize;

    default:
      throw runtime_error("unreachable");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* ElfLinker::getSegmentData(SegmentType type) const
{
  switch (type) {
    case SegmentType::text:
      return myTextData.get();

    case SegmentType::data:
      return myDataData.get();

    case SegmentType::rodata:
      return myRodataData.get();

    default:
      throw runtime_error("unreachable");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 ElfLinker::getSegmentBase(SegmentType type) const
{
  switch (type) {
    case SegmentType::text:
      return myTextBase;

    case SegmentType::data:
      return myDataBase;

    case SegmentType::rodata:
      return myRodataBase;

    default:
      throw runtime_error("unreachable");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const vector<uInt32>& ElfLinker::getInitArray() const
{
  return myInitArray;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const vector<uInt32>& ElfLinker::getPreinitArray() const
{
  return myPreinitArray;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ElfLinker::RelocatedSymbol ElfLinker::findRelocatedSymbol(string_view name) const
{
  const auto& symbols = myElf.getSymbols();
  for (size_t i = 0; i < symbols.size(); i++) {
    if (symbols[i].name != name) continue;

    if (!myRelocatedSymbols[i])
      ElfSymbolResolutionError::raise("symbol could not be relocated");

    return myRelocatedSymbols[i].value();  // NOLINT: we know the value is valid
  }

  ElfSymbolResolutionError::raise("symbol not found");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const vector<optional<ElfLinker::RelocatedSection>>&
ElfLinker::getRelocatedSections() const
{
  return myRelocatedSections;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const vector<optional<ElfLinker::RelocatedSymbol>>&
ElfLinker::getRelocatedSymbols() const
{
  return myRelocatedSymbols;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32& ElfLinker::getSegmentSizeRef(SegmentType type)
{
  switch (type) {
    case SegmentType::text:
      return myTextSize;

    case SegmentType::data:
      return myDataSize;

    case SegmentType::rodata:
      return myRodataSize;

    default:
      throw runtime_error("unreachable");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<uInt8[]>& ElfLinker::getSegmentDataRef(SegmentType type)
{
  switch (type) {
    case SegmentType::text:
      return myTextData;

    case SegmentType::data:
      return myDataData;

    case SegmentType::rodata:
      return myRodataData;

    default:
      throw runtime_error("unreachable");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ElfLinker::relocateSections()
{
  const auto& sections = myElf.getSections();
  myRelocatedSections.resize(sections.size(), std::nullopt);

  // relocate everything that is not .bss
  for (size_t i = 0; i < sections.size(); i++) {
    const auto& section = sections[i];

    const auto segmentType = determineSegmentType(section);
    if (!segmentType || section.type == ElfFile::SHT_NOBITS) continue;

    uInt32& segmentSize = getSegmentSizeRef(*segmentType);

    if (segmentSize % section.align)
      segmentSize = (segmentSize / section.align + 1) * section.align;

    myRelocatedSections[i] = {*segmentType, segmentSize};
    segmentSize += section.size;
  }

  // relocate all .bss sections
  for (size_t i = 0; i < sections.size(); i++) {
    const auto& section = sections[i];

    if (section.type == ElfFile::SHT_NOBITS) {
      if (myDataSize % section.align)
        myDataSize = (myDataSize / section.align + 1) * section.align;

      myRelocatedSections[i] = {SegmentType::data, myDataSize};
      myDataSize += section.size;
    }
  }

  // ensure that the segments don't overlap
  if (
    checkSegmentOverlap(myTextBase, myTextSize, myDataBase, myDataSize) ||
    checkSegmentOverlap(myTextBase, myTextSize, myRodataBase, myRodataSize) ||
    checkSegmentOverlap(myDataBase, myDataSize, myRodataBase, myRodataSize)
  )
    ElfLinkError::raise("segments overlap");

  // allocate segment data
  for (const SegmentType segmentType: {SegmentType::text, SegmentType::data, SegmentType::rodata}) {
    const uInt32 segmentSize = getSegmentSize(segmentType);
    if (segmentSize == 0) continue;

    auto& segmentData = getSegmentDataRef(segmentType);

    segmentData = make_unique<uInt8[]>(segmentSize);
    std::memset(segmentData.get(), 0, segmentSize);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ElfLinker::copySections()
{
  const auto& sections = myElf.getSections();

  // copy segment data
  for (size_t i = 0; i < sections.size(); i++) {
    const auto& relocatedSection = myRelocatedSections[i];
    if (!relocatedSection) continue;

    const auto& section = sections[i];
    if (section.type == ElfFile::SHT_NOBITS) continue;

    const auto segmentType = determineSegmentType(section);
    if (!segmentType) continue;

    std::memcpy(
      getSegmentDataRef(*segmentType).get() + relocatedSection->offset,
      myElf.getData() + section.offset,
      section.size
    );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ElfLinker::relocateInitArrays()
{
  const auto& sections = myElf.getSections();
  uInt32 initArraySize = 0;
  uInt32 preinitArraySize = 0;

  std::unordered_map<uInt32, uInt32> relocatedInitArrays;
  std::unordered_map<uInt32, uInt32> relocatedPreinitArrays;

  // Relocate init arrays
  for (uInt32 i = 0; i < sections.size(); i++) {
    const auto& section = sections[i];

    switch (section.type) {
      case ElfFile::SHT_INIT_ARRAY:
        if (section.size % 4) ElfLinkError::raise("invalid init array");

        relocatedInitArrays[i] = initArraySize;
        initArraySize += section.size;

        break;

      case ElfFile::SHT_PREINIT_ARRAY:
        if (section.size % 4) ElfLinkError::raise("invalid preinit array");

        relocatedPreinitArrays[i] = preinitArraySize;
        preinitArraySize += section.size;

        break;

      default:
        break;
    }
  }

  myInitArray.resize(initArraySize >> 2);
  myPreinitArray.resize(preinitArraySize >> 2);

  copyInitArrays(myInitArray, relocatedInitArrays);
  copyInitArrays(myPreinitArray, relocatedPreinitArrays);

  applyRelocationsToInitArrays(ElfFile::SHT_INIT_ARRAY, myInitArray, relocatedInitArrays);
  applyRelocationsToInitArrays(ElfFile::SHT_PREINIT_ARRAY, myPreinitArray, relocatedPreinitArrays);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ElfLinker::relocateSymbols(const vector<ExternalSymbol>& externalSymbols)
{
  std::unordered_map<string, const ExternalSymbol*> externalSymbolLookup;

  for (const auto& externalSymbol: externalSymbols)
    externalSymbolLookup[externalSymbol.name] = &externalSymbol;

  // relocate symbols
  const auto& symbols = myElf.getSymbols();
  myRelocatedSymbols.resize(symbols.size(), std::nullopt);

  for (size_t i = 0; i < symbols.size(); i++) {
    const auto& symbol = symbols[i];

    if (symbol.section == ElfFile::SHN_ABS) {
      myRelocatedSymbols[i] = {std::nullopt, symbol.value, false};

      continue;
    }

    if (symbol.section == ElfFile::SHN_UND) {
      if (externalSymbolLookup.contains(symbol.name))
        myRelocatedSymbols[i] = {std::nullopt, externalSymbolLookup[symbol.name]->value, false};

      else if (undefinedSymbolDefault)
        myRelocatedSymbols[i] = {std::nullopt, *undefinedSymbolDefault, true};

      continue;
    }

    const auto& relocatedSection = myRelocatedSections[symbol.section];
    if (!relocatedSection) continue;

    uInt32 value = getSegmentBase(relocatedSection->segment) + relocatedSection->offset;
    if (symbol.type != ElfFile::STT_SECTION) value += symbol.value;

    myRelocatedSymbols[i] = {relocatedSection->segment, value, false};
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ElfLinker::applyRelocationsToSections()
{
  const auto& sections = myElf.getSections();

  // apply relocations
  for (size_t iSection = 0; iSection < sections.size(); iSection++) {
    const auto& relocations = myElf.getRelocations(iSection);
    if (!relocations) continue;
    if (!myRelocatedSections[iSection]) continue;

    for (const auto& relocation: *relocations)
      applyRelocationToSection(relocation, iSection);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ElfLinker::copyInitArrays(vector<uInt32>& initArray, const std::unordered_map<uInt32, uInt32>& relocatedInitArrays)
{
  const uInt8* elfData = myElf.getData();
  const auto& sections = myElf.getSections();

  // Copy init arrays
  for (const auto [iSection, offset]: relocatedInitArrays) {
    const auto& section = sections[iSection];

    for (size_t i = 0; i < section.size; i += 4)
      initArray[(offset + i) >> 2] = read32(elfData + section.offset + i);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ElfLinker::applyRelocationToSection(const ElfFile::Relocation& relocation, size_t iSection)
{
  const auto& targetSection = myElf.getSections()[iSection];
  const auto& targetSectionRelocated = myRelocatedSections[iSection].value(); // NOLINT
  const auto& symbol = myElf.getSymbols()[relocation.symbol];
  const auto& relocatedSymbol = myRelocatedSymbols[relocation.symbol];

  if (!relocatedSymbol) ElfLinkError::raise(
    "unable to relocate " + symbol.name + " in " + targetSection.name + ": symbol could not be relocated"
  );

  if (relocatedSymbol->undefined) Logger::error("unable to resolve symbol " + relocation.symbolName);

  if (relocation.offset + 4 > targetSection.size)
    ElfLinkError::raise(
      "unable to relocate " + symbol.name + " in " + targetSection.name + ": target out of range"
    );

  const uInt32 targetAddress =
    getSegmentBase(targetSectionRelocated.segment) +
    targetSectionRelocated.offset +
    relocation.offset;

  uInt8* const target =
    getSegmentDataRef(targetSectionRelocated.segment).get() +
    targetSectionRelocated.offset +
    relocation.offset;

  switch (relocation.type) {
    case ElfFile::R_ARM_ABS32:
    case ElfFile::R_ARM_TARGET1:
      {
        const uInt32 value = relocatedSymbol->value + relocation.addend.value_or(read32(target));
        write32(target, value | (symbol.type == ElfFile::STT_FUNC ? 0x01 : 0));

        break;
      }

    case ElfFile::R_ARM_REL32:
      {
        uInt32 value = relocatedSymbol->value + relocation.addend.value_or(read32(target));
        value |= (symbol.type == ElfFile::STT_FUNC ? 0x01 : 0);

        write32(target, value - targetAddress);

        break;
      }

    case ElfFile::R_ARM_THM_CALL:
    case ElfFile::R_ARM_THM_JUMP24:
      {
        const uInt32 op = read32(target);

        const Int32 offset = relocatedSymbol->value + relocation.addend.value_or(elfUtil::decode_B_BL(op) + 4) -
          getSegmentBase(targetSectionRelocated.segment) -
          targetSectionRelocated.offset -
          relocation.offset - 4;

        if ((offset >> 24) != -1 && (offset >> 24) != 0)
          ElfLinkError::raise("unable to relocate jump: offset out of bounds");

        write32(target, elfUtil::encode_B_BL(offset, relocation.type == ElfFile::R_ARM_THM_CALL));

        break;
      }

    default:
      ElfLinkError::raise("unknown relocation type");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ElfLinker::applyRelocationsToInitArrays(uInt8 initArrayType, vector<uInt32>& initArray,
                                             const std::unordered_map<uInt32, uInt32>& relocatedInitArrays)
{
  const auto& symbols = myElf.getSymbols();
  const auto& sections = myElf.getSections();

  for (uInt32 iSection = 0; iSection < sections.size(); iSection++) {
    const auto& section = sections[iSection];
    if (section.type != initArrayType) continue;

    const auto& relocations = myElf.getRelocations(iSection);
    if (!relocations) continue;

    for (const auto& relocation: *relocations) {
      if (relocation.type != ElfFile::R_ARM_ABS32 && relocation.type != ElfFile::R_ARM_TARGET1)
        ElfLinkError::raise("unsupported relocation for init array");

      const auto& relocatedSymbol = myRelocatedSymbols[relocation.symbol];
      if (!relocatedSymbol)
        ElfLinkError::raise(
          "unable to relocate init array: symbol " + relocation.symbolName + " could not be relocated"
        );

      if (relocatedSymbol->undefined)
        Logger::error("unable to relocate symbol " + relocation.symbolName);

      if (relocation.offset + 4 > section.size)
        ElfLinkError::raise("unable relocate init array: symbol " + relocation.symbolName + " out of range");

      const uInt32 index = (relocatedInitArrays.at(iSection) + relocation.offset) >> 2;
      const uInt32 value = relocatedSymbol->value + relocation.addend.value_or(initArray[index]);
      initArray[index] = value | (symbols[relocation.symbol].type == ElfFile::STT_FUNC ? 1 : 0);
    }
  }
}
