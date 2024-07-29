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

#include <sstream>

#ifdef DUMP_ELF
#include <ofstream>
#endif

#include "System.hxx"
#include "ElfParser.hxx"
#include "ElfLinker.hxx"
#include "ElfEnvironment.hxx"
#include "Logger.hxx"
#include "exception/FatalEmulationError.hxx"

#include "CartELF.hxx"

#define DUMP_ELF

using namespace elfEnvironment;

namespace {
  constexpr size_t TRANSACTION_QUEUE_CAPACITY = 16384;
  constexpr uInt32 ARM_RUNAHED_MIN = 3;
  constexpr uInt32 ARM_RUNAHED_MAX = 6;

#ifdef DUMP_ELF
  void dumpElf(const ElfFile& elf)
  {
    cout << "\nELF sections:\n\n";

    size_t i = 0;
    for (const auto& section: elf.getSections()) {
      if (section.type != 0x00) cout << i << " " << section << '\n';
      i++;
    }

    auto symbols = elf.getSymbols();
    cout << "\nELF symbols:\n\n";
    if (!symbols.empty()) {
      i = 0;
      for (auto& symbol: symbols)
        cout << (i++) << " " << symbol << '\n';
    }

    i = 0;
    for (const auto& section: elf.getSections()) {
      auto rels = elf.getRelocations(i++);
      if (!rels) continue;

      cout
        << "\nELF relocations for section "
        << section.name << ":\n\n";

      for (auto& rel: *rels) cout << rel << '\n';
    }
  }

  void dumpLinkage(const ElfParser& parser, const ElfLinker& linker)
  {
    cout << std::hex << std::setfill('0');

    cout
      << "\ntext segment size: 0x" << std::setw(8) << linker.getSegmentSize(ElfLinker::SegmentType::text)
      << "\ndata segment size: 0x" << std::setw(8) << linker.getSegmentSize(ElfLinker::SegmentType::data)
      << "\nrodata segment size: 0x" << std::setw(8) << linker.getSegmentSize(ElfLinker::SegmentType::rodata)
      << '\n';

    cout << "\nrelocated sections:\n\n";

    const auto& sections = parser.getSections();
    const auto& relocatedSections = linker.getRelocatedSections();

    for (size_t i = 0; i < sections.size(); i++) {
      if (!relocatedSections[i]) continue;

      cout
        << sections[i].name
        << " @ 0x"<< std::setw(8)
        << (relocatedSections[i]->offset + linker.getSegmentBase(relocatedSections[i]->segment))
        << " size 0x" << std::setw(8) << sections[i].size << '\n';
    }

    cout << "\nrelocated symbols:\n\n";

    const auto& symbols = parser.getSymbols();
    const auto& relocatedSymbols = linker.getRelocatedSymbols();

    for (size_t i = 0; i < symbols.size(); i++) {
      if (!relocatedSymbols[i]) continue;

      cout
        << symbols[i].name
        << " = 0x" << std::setw(8) << relocatedSymbols[i]->value;

      if (relocatedSymbols[i]->segment) {
        switch (*relocatedSymbols[i]->segment) {
          case ElfLinker::SegmentType::text:
            cout << " (text)";
            break;

          case ElfLinker::SegmentType::data:
            cout << " (data)";
            break;

          case ElfLinker::SegmentType::rodata:
            cout << " (rodata)";
            break;
        }
      } else {
        cout << " (abs)";
      }

      cout << '\n';
    }

    const auto& initArray = linker.getInitArray();
    const auto& preinitArray = linker.getPreinitArray();

    if (!initArray.empty()) {
      cout << "\ninit array:\n\n";

      for (const uInt32 address: initArray)
        cout << " * 0x" << std::setw(8) <<  address << '\n';
    }

    if (!preinitArray.empty()) {
      cout << "\npreinit array:\n\n";

      for (const uInt32 address: preinitArray)
        cout << " * 0x" << std::setw(8) <<  address << '\n';
    }

    cout << std::dec;
  }

  void writeDebugBinary(const ElfLinker& linker)
  {
    constexpr size_t IMAGE_SIZE = 4 * 0x00100000;
    static const char* IMAGE_FILE_NAME = "elf_executable_image.bin";

    auto binary = make_unique<uInt8[]>(IMAGE_SIZE);
    std::memset(binary.get(), 0, IMAGE_SIZE);

    for (auto segment: {ElfLinker::SegmentType::text, ElfLinker::SegmentType::data, ElfLinker::SegmentType::rodata})
      std::memcpy(
        binary.get() + linker.getSegmentBase(segment),
        linker.getSegmentData(segment),
        linker.getSegmentSize(segment)
      );

    {
      std::ofstream binaryFile;

      binaryFile.open(IMAGE_FILE_NAME);
      binaryFile.write(reinterpret_cast<const char*>(binary.get()), 4 * 0x00100000);
    }

    cout << "wrote executable image to " << IMAGE_FILE_NAME << std::endl;
  }
#endif
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeELF::CartridgeELF(const ByteBuffer& image, size_t size, string_view md5,
                           const Settings& settings)
  : Cartridge(settings, md5), myImageSize(size), myTransactionQueue(TRANSACTION_QUEUE_CAPACITY),
    myVcsLib(myTransactionQueue)
{
  myImage = make_unique<uInt8[]>(size);
  std::memcpy(myImage.get(), image.get(), size);

  myLastPeekResult = make_unique<uInt8[]>(0x1000);
  std::fill_n(myLastPeekResult.get(), 0x1000, 0);

  createRomAccessArrays(0x1000);

  parseAndLinkElf();
  setupMemoryMap();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeELF::~CartridgeELF() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::reset()
{
  std::fill_n(myLastPeekResult.get(), 0x1000, 0);
  myIsBusDriven = false;
  myDriveBusValue = 0;
  myArmCyclesOffset = 0;

  std::memset(mySectionStack.get(), 0, STACK_SIZE);
  std::memset(mySectionText.get(), 0, TEXT_SIZE);
  std::memset(mySectionData.get(), 0, DATA_SIZE);
  std::memset(mySectionRodata.get(), 0, RODATA_SIZE);
  std::memset(mySectionTables.get(), 0, TABLES_SIZE);

  std::memcpy(mySectionText.get(), myLinker->getSegmentData(ElfLinker::SegmentType::text),
                                   myLinker->getSegmentSize(ElfLinker::SegmentType::text));
  std::memcpy(mySectionData.get(), myLinker->getSegmentData(ElfLinker::SegmentType::data),
                                   myLinker->getSegmentSize(ElfLinker::SegmentType::data));
  std::memcpy(mySectionRodata.get(), myLinker->getSegmentData(ElfLinker::SegmentType::rodata),
                                     myLinker->getSegmentSize(ElfLinker::SegmentType::rodata));
  std::memcpy(mySectionTables.get(), LOOKUP_TABLES, sizeof(LOOKUP_TABLES));

  myCortexEmu.reset();

  myTransactionQueue
    .reset()
	  .injectROMAt(0x00, 0x1ffc)
	  .injectROM(0x10)
    .setNextInjectAddress(0x1000);

  myVcsLib.reset();

  myVcsLib.vcsCopyOverblankToRiotRam();
  myVcsLib.vcsStartOverblank();
  myVcsLib.vcsEndOverblank();
  myVcsLib.vcsNop2n(1024);

  jumpToMain();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::install(System& system)
{
  mySystem = &system;

  for (uInt16 addr = 0; addr < 0x1000; addr += System::PAGE_SIZE) {
    System::PageAccess access(this, System::PageAccessType::READ);
    access.romPeekCounter = &myRomAccessCounter[addr];
    access.romPokeCounter = &myRomAccessCounter[addr];

    mySystem->setPageAccess(0x1000 + addr, access);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeELF::save(Serializer& out) const
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeELF::load(Serializer& in)
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeELF::peek(uInt16 address)
{
  // The actual handling happens in overdrivePeek
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeELF::peekOob(uInt16 address)
{
  return myLastPeekResult[address & 0xfff];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeELF::poke(uInt16 address, uInt8 value)
{
  return false;
}

void CartridgeELF::consoleChanged(ConsoleTiming timing)
{
  myConsoleTiming = timing;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteBuffer& CartridgeELF::getImage(size_t& size) const
{
  size = myImageSize;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeELF::overdrivePeek(uInt16 address, uInt8 value)
{
  value = driveBus(address, value);

  if (address & 0x1000) {
    if (!myIsBusDriven) value = mySystem->getDataBusState();
    myLastPeekResult[address & 0xfff] = value;
  }

  return value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeELF::overdrivePoke(uInt16 address, uInt8 value)
{
  return driveBus(address, value);
}

inline uInt64 CartridgeELF::getArmCycles() const
{
  return myCortexEmu.getCycles() + myArmCyclesOffset;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt8 CartridgeELF::driveBus(uInt16 address, uInt8 value)
{
  auto* nextTransaction = myTransactionQueue.getNextTransaction(address);
  if (nextTransaction) {
    nextTransaction->setBusState(myIsBusDriven, myDriveBusValue);
    syncClock(*nextTransaction);
  }

  if (myIsBusDriven) value |= myDriveBusValue;

  myVcsLib.updateBus(address, value);

  if (!myVcsLib.isSuspended()) runArm();

  return value;
}

inline void CartridgeELF::syncClock(const BusTransactionQueue::Transaction& transaction)
{
  const Int64 currentSystemArmCycles = mySystem->cycles() * myArmCyclesPer6502Cycle;
  const Int64 transactionArmCycles = transaction.timestamp + myArmCyclesOffset;

  myArmCyclesOffset += currentSystemArmCycles - transactionArmCycles;

  if (transactionArmCycles > currentSystemArmCycles)
    Logger::error("ARM took too many cycles between bus transitions");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::parseAndLinkElf()
{
  ElfParser elfParser;

  try {
    elfParser.parse(myImage.get(), myImageSize);
  } catch (ElfParser::ElfParseError& e) {
    throw runtime_error("failed to initialize ELF: " + string(e.what()));
  }

#ifdef DUMP_ELF
  dumpElf(elfParser);
#endif

  myLinker = make_unique<ElfLinker>(ADDR_TEXT_BASE, ADDR_DATA_BASE, ADDR_RODATA_BASE, elfParser);
  try {
    myLinker->link(externalSymbols(Palette::ntsc));
  } catch (const ElfLinker::ElfLinkError& e) {
    throw runtime_error("failed to link ELF: " + string(e.what()));
  }

  try {
    myArmEntrypoint = myLinker->findRelocatedSymbol("elf_main").value;
  } catch (const ElfLinker::ElfSymbolResolutionError& e) {
    throw runtime_error("failed to resolve ARM entrypoint" + string(e.what()));
  }

  if (myLinker->getSegmentSize(ElfLinker::SegmentType::text) > TEXT_SIZE)
    throw runtime_error("text segment too large");

  if (myLinker->getSegmentSize(ElfLinker::SegmentType::data) > DATA_SIZE)
    throw runtime_error("data segment too large");

  if (myLinker->getSegmentSize(ElfLinker::SegmentType::rodata) > RODATA_SIZE)
    throw runtime_error("rodata segment too large");

  #ifdef DUMP_ELF
    dumpLinkage(elfParser, *myLinker);

    cout
      << "\nARM entrypoint: 0x"
      << std::hex << std::setw(8) << std::setfill('0') << myArmEntrypoint << std::dec
      << '\n';

    writeDebugBinary(*myLinker);
  #endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::setupMemoryMap()
{
  mySectionStack = make_unique<uInt8[]>(STACK_SIZE);
  mySectionText = make_unique<uInt8[]>(TEXT_SIZE);
  mySectionData = make_unique<uInt8[]>(DATA_SIZE);
  mySectionRodata = make_unique<uInt8[]>(RODATA_SIZE);
  mySectionTables = make_unique<uInt8[]>(TABLES_SIZE);

  myCortexEmu
    .mapRegionData(ADDR_STACK_BASE / CortexM0::PAGE_SIZE,
                   STACK_SIZE / CortexM0::PAGE_SIZE, false, mySectionStack.get())
    .mapRegionCode(ADDR_TEXT_BASE / CortexM0::PAGE_SIZE,
                   TEXT_SIZE / CortexM0::PAGE_SIZE, true, mySectionText.get())
    .mapRegionData(ADDR_DATA_BASE / CortexM0::PAGE_SIZE,
                   DATA_SIZE / CortexM0::PAGE_SIZE, false, mySectionData.get())
    .mapRegionData(ADDR_RODATA_BASE / CortexM0::PAGE_SIZE,
                   RODATA_SIZE / CortexM0::PAGE_SIZE, true, mySectionRodata.get())
    .mapRegionData(ADDR_TABLES_BASE / CortexM0::PAGE_SIZE,
                   TABLES_SIZE / CortexM0::PAGE_SIZE, true, mySectionTables.get())
    .mapRegionDelegate(ADDR_STUB_BASE / CortexM0::PAGE_SIZE,
                       STUB_SIZE / CortexM0::PAGE_SIZE, true, &myVcsLib);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeELF::getCoreClock() const
{
  switch (myConsoleTiming) {
    case ConsoleTiming::ntsc:
      return myArmCyclesPer6502Cycle * 262 * 76 * 60;

    case ConsoleTiming::pal:
    case ConsoleTiming::secam:
      return myArmCyclesPer6502Cycle * 312 * 76 * 50;

    default:
      throw runtime_error("invalid console timing");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeELF::getSystemType() const
{
  switch (myConsoleTiming) {
    case ConsoleTiming::ntsc:
      return ST_NTSC_2600;

    // Use frame layout here instead
    case ConsoleTiming::pal:
    case ConsoleTiming::secam:
      return ST_PAL_2600;

    default:
      throw runtime_error("invalid console timing");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::jumpToMain()
{
  if (!mySystem) throw runtime_error("cartridge not installed");

  uInt32 sp = ADDR_STACK_BASE + STACK_SIZE;
  CortexM0::err_t err = 0;

  // Feature flags
  sp -= 4;
  err |= myCortexEmu.write32(sp, 0);

  sp -= 4;
  err |= myCortexEmu.write32(sp, getCoreClock());

  sp -= 4;
  err |= myCortexEmu.write32(sp, getSystemType());

  if (err) throw runtime_error("unable to setup main args");

  myCortexEmu
    .setRegister(0, sp )
    .setRegister(13, sp)
    .setRegister(14, RETURN_ADDR_MAIN)
    .setPc(myArmEntrypoint);
}

void CartridgeELF::runArm()
{
  if (
    (getArmCycles() >= (mySystem->cycles() + ARM_RUNAHED_MIN) * myArmCyclesPer6502Cycle) ||
    myTransactionQueue.size() >= QUEUE_SIZE_LIMIT
  )
    return;

  const uInt32 cyclesGoal =
    (mySystem->cycles() + ARM_RUNAHED_MAX) * myArmCyclesPer6502Cycle - getArmCycles();
  uInt32 cycles;

  const CortexM0::err_t err = myCortexEmu.run(cyclesGoal, cycles);

  if (err && (CortexM0::getErrCustom(err) != ERR_STOP_EXECUTION))
    FatalEmulationError::raise("error executing ARM code: " + CortexM0::describeError(err));
}


