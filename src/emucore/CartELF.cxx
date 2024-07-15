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

#include "System.hxx"
#include "ElfParser.hxx"
#include "ElfLinker.hxx"
#include "ElfEnvironment.hxx"
#include "exception/FatalEmulationError.hxx"

#include "CartELF.hxx"

#define DUMP_ELF

using namespace elfEnvironment;

namespace {
  constexpr size_t TRANSACTION_QUEUE_CAPACITY = 16384;

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
#endif
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeELF::CartridgeELF(const ByteBuffer& image, size_t size, string_view md5,
                           const Settings& settings)
  : Cartridge(settings, md5), myImageSize(size)
{
  ElfParser elfParser;

  try {
    elfParser.parse(image.get(), size);
  } catch (ElfParser::ElfParseError& e) {
    throw runtime_error("failed to initialize ELF: " + string(e.what()));
  }

  myImage = make_unique<uInt8[]>(size);
  std::memcpy(myImage.get(), image.get(), size);

  myLastPeekResult = make_unique<uInt8[]>(0x1000);
  std::fill_n(myLastPeekResult.get(), 0x1000, 0);

  createRomAccessArrays(0x1000);

#ifdef DUMP_ELF
  dumpElf(elfParser);
#endif

  ElfLinker elfLinker(ADDR_TEXT_BASE, ADDR_DATA_BASE, ADDR_RODATA_BASE, elfParser);
  try {
    elfLinker.link(externalSymbols(Palette::ntsc));
  } catch (const ElfLinker::ElfLinkError& e) {
    throw runtime_error("failed to link ELF: " + string(e.what()));
  }

  try {
    myArmEntrypoint = elfLinker.findRelocatedSymbol("elf_main").value;
  } catch (const ElfLinker::ElfSymbolResolutionError& e) {
    throw runtime_error("failed to resolve ARM entrypoint" + string(e.what()));
  }

  #ifdef DUMP_ELF
    dumpLinkage(elfParser, elfLinker);

    cout
      << "\nARM entrypoint: 0x"
      << std::hex << std::setw(8) << std::setfill('0') << myArmEntrypoint << std::dec
      << '\n';
  #endif
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeELF::~CartridgeELF() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::reset()
{
  std::fill_n(myLastPeekResult.get(), 0x1000, 0);
  myIsBusDriven = false;
  myDriveBusValue = 0;

  myTransactionQueue.reset();
	myTransactionQueue.injectROM(0x00, 0x1ffc);
	myTransactionQueue.injectROM(0x10);
  myTransactionQueue.setNextInjectAddress(0x1000);

  vcsCopyOverblankToRiotRam();
  vcsStartOverblank();
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteBuffer& CartridgeELF::getImage(size_t& size) const
{
  size = myImageSize;
  return myImage;
}

uInt8 CartridgeELF::overdrivePeek(uInt16 address, uInt8 value)
{
  value = driveBus(address, value);

  if (address & 0x1000) {
    if (!myIsBusDriven) value = mySystem->getDataBusState();
    myLastPeekResult[address & 0xfff] = value;
  }

  return value;
}

uInt8 CartridgeELF::overdrivePoke(uInt16 address, uInt8 value)
{
  return driveBus(address, value);
}

inline uInt8 CartridgeELF::driveBus(uInt16 address, uInt8 value)
{
  BusTransaction* nextTransaction = myTransactionQueue.getNextTransaction(address);
  if (nextTransaction) nextTransaction->setBusState(myIsBusDriven, myDriveBusValue);

  if (myIsBusDriven) value |= myDriveBusValue;

  return value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::vcsWrite5(uInt8 zpAddress, uInt8 value)
{
	myTransactionQueue.injectROM(0xa9);
	myTransactionQueue.injectROM(value);
	myTransactionQueue.injectROM(0x85);
	myTransactionQueue.injectROM(zpAddress);
  myTransactionQueue.yield(zpAddress);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::vcsCopyOverblankToRiotRam()
{
  for (uInt8 i = 0; i < OVERBLANK_PROGRAM_SIZE; i++)
    vcsWrite5(0x80 + i, OVERBLANK_PROGRAM[i]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::vcsStartOverblank()
{
	myTransactionQueue.injectROM(0x4c);
	myTransactionQueue.injectROM(0x80);
	myTransactionQueue.injectROM(0x00);
  myTransactionQueue.yield(0x0080);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeELF::BusTransaction CartridgeELF::BusTransaction::transactionYield(uInt16 address)
{
  address &= 0x1fff;
  return {.address = address, .value = 0, .yield = true};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeELF::BusTransaction CartridgeELF::BusTransaction::transactionDrive(uInt16 address, uInt8 value)
{
  address &= 0x1fff;
  return {.address = address, .value = value, .yield = false};
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::BusTransaction::setBusState(bool& bs_drive, uInt8& bs_value) const
{
  if (yield) {
    bs_drive = false;
  } else {
    bs_drive = true;
    bs_value = this->value;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeELF::BusTransactionQueue::BusTransactionQueue()
{
  myQueue = make_unique<BusTransaction[]>(TRANSACTION_QUEUE_CAPACITY);
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::BusTransactionQueue::reset()
{
  myQueueNext = myQueueSize = 0;
  myNextInjectAddress = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::BusTransactionQueue::setNextInjectAddress(uInt16 address)
{
  myNextInjectAddress = address;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::BusTransactionQueue::injectROM(uInt8 value)
{
  injectROM(value, myNextInjectAddress);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::BusTransactionQueue::injectROM(uInt8 value, uInt16 address)
{
  push(BusTransaction::transactionDrive(address, value));
  myNextInjectAddress = address + 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::BusTransactionQueue::yield(uInt16 address)
{
  push(BusTransaction::transactionYield(address));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeELF::BusTransactionQueue::hasPendingTransaction() const
{
  return myQueueSize > 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeELF::BusTransaction* CartridgeELF::BusTransactionQueue::getNextTransaction(uInt16 address)
{
  if (myQueueSize == 0) return nullptr;

  BusTransaction* nextTransaction = &myQueue[myQueueNext];
  if (nextTransaction->address != (address & 0x1fff)) return nullptr;

  myQueueNext = (myQueueNext + 1) % TRANSACTION_QUEUE_CAPACITY;
  myQueueSize--;

  return nextTransaction;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::BusTransactionQueue::push(const BusTransaction& transaction)
{
  if (myQueueSize > 0) {
    BusTransaction& lastTransaction = myQueue[(myQueueNext + myQueueSize - 1) % TRANSACTION_QUEUE_CAPACITY];

    if (lastTransaction.address == transaction.address) {
      lastTransaction = transaction;
      return;
    }
  }

  if (myQueueSize == TRANSACTION_QUEUE_CAPACITY)
    throw FatalEmulationError("read stream overflow");

  myQueue[(myQueueNext + myQueueSize++) % TRANSACTION_QUEUE_CAPACITY] = transaction;
}


