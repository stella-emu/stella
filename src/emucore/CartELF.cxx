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
#include "exception/FatalEmulationError.hxx"

#include "CartELF.hxx"

#define DUMP_ELF

namespace {
  constexpr size_t TRANSACTION_QUEUE_CAPACITY = 16384;

  constexpr uInt8 OVERBLANK_PROGRAM[] = {
	  0xa0,0x00,			  // ldy #0
	  0xa5,0xe0,			  // lda $e0
	        					  // OverblankLoop:
	  0x85,0x02,			  // sta WSYNC
	  0x85,0x2d,			  // sta AUDV0 (currently using $2d instead to disable audio until fully implemented
	  0x98,				      // tya
	  0x18,				      // clc
	  0x6a,				      // ror
	  0xaa,				      // tax
	  0xb5,0xe0,			  // lda $e0,x
	  0x90,0x04,			  // bcc
	  0x4a,				      // lsr
	  0x4a,				      // lsr
	  0x4a,				      // lsr
	  0x4a,				      // lsr
	  0xc8,				      // iny
	  0xc0, 0x1d,			  // cpy #$1d
	  0xd0, 0x04,			  // bne
	  0xa2, 0x02,			  // ldx #2
	  0x86, 0x00,			  // stx VSYNC
	  0xc0, 0x20,			  // cpy #$20
	  0xd0, 0x04,			  // bne SkipClearVSync
	  0xa2, 0x00,			  // ldx #0
	  0x86, 0x00,			  // stx VSYNC
	  					        // SkipClearVSync:
	  0xc0, 0x3f,			  // cpy #$3f
	  0xd0, 0xdb,			  // bne OverblankLoop
	  					        // WaitForCart:
	  0xae, 0xff, 0xff,	// ldx $ffff
	  0xd0, 0xfb,			  // bne WaitForCart
	  0x4c, 0x00, 0x10	// jmp $1000
  };

  constexpr uInt32 ADDR_TEXT_BASE = 0x00100000;
  constexpr uInt32 ADDR_DATA_BASE = 0x00200000;

  constexpr uInt32 ADDR_ADDR_IDR = 0xf0000000;
  constexpr uInt32 ADDR_DATA_IDR = 0xf0000004;
  constexpr uInt32 ADDR_DATA_ODR = 0xf0000008;
  constexpr uInt32 ADDR_DATA_MODER = 0xf0000010;

  constexpr uInt32 VCSLIB_BASE = 0x1001;

  constexpr uInt32 ADDR_MEMSET = VCSLIB_BASE;
  constexpr uInt32 ADDR_MEMCPY = VCSLIB_BASE + 4;

  constexpr uInt32 ADDR_VCS_LDA_FOR_BUS_STUFF2 = VCSLIB_BASE + 8;
  constexpr uInt32 ADDR_VCS_LDX_FOR_BUS_STUFF2 = VCSLIB_BASE + 12;
  constexpr uInt32 ADDR_VCS_LDY_FOR_BUS_STUFF2 = VCSLIB_BASE + 16;
  constexpr uInt32 ADDR_VCS_WRITE3 = VCSLIB_BASE + 20;
  constexpr uInt32 ADDR_VCS_JMP3 = VCSLIB_BASE + 24;
  constexpr uInt32 ADDR_VCS_NOP2 = VCSLIB_BASE + 28;
  constexpr uInt32 ADDR_VCS_NOP2N = VCSLIB_BASE + 32;
  constexpr uInt32 ADDR_VCS_WRITE5 = VCSLIB_BASE + 36;
  constexpr uInt32 ADDR_VCS_WRITE6 = VCSLIB_BASE + 40;
  constexpr uInt32 ADDR_VCS_LDA2 = VCSLIB_BASE + 44;
  constexpr uInt32 ADDR_VCS_LDX2 = VCSLIB_BASE + 48;
  constexpr uInt32 ADDR_VCS_LDY2 = VCSLIB_BASE + 52;
  constexpr uInt32 ADDR_VCS_SAX3 = VCSLIB_BASE + 56;
  constexpr uInt32 ADDR_VCS_STA3 = VCSLIB_BASE + 60;
  constexpr uInt32 ADDR_VCS_STX3 = VCSLIB_BASE + 64;
  constexpr uInt32 ADDR_VCS_STY3 = VCSLIB_BASE + 68;
  constexpr uInt32 ADDR_VCS_STA4 = VCSLIB_BASE + 72;
  constexpr uInt32 ADDR_VCS_STX4 = VCSLIB_BASE + 76;
  constexpr uInt32 ADDR_VCS_STY4 = VCSLIB_BASE + 80;
  constexpr uInt32 ADDR_VCS_COPY_OVERBLANK_TO_RIOT_RAM = VCSLIB_BASE + 84;
  constexpr uInt32 ADDR_VCS_START_OVERBLANK = VCSLIB_BASE + 88;
  constexpr uInt32 ADDR_VCS_END_OVERBLANK = VCSLIB_BASE + 92;
  constexpr uInt32 ADDR_VCS_READ4 = VCSLIB_BASE + 96;
  constexpr uInt32 ADDR_RANDINT = VCSLIB_BASE + 100;
  constexpr uInt32 ADDR_VCS_TXS2 = VCSLIB_BASE + 104;
  constexpr uInt32 ADDR_VCS_JSR6 = VCSLIB_BASE + 108;
  constexpr uInt32 ADDR_VCS_PHA3 = VCSLIB_BASE + 112;
  constexpr uInt32 ADDR_VCS_PHP3 = VCSLIB_BASE + 116;
  constexpr uInt32 ADDR_VCS_PLA4 = VCSLIB_BASE + 120;
  constexpr uInt32 ADDR_VCS_PLP4 = VCSLIB_BASE + 124;
  constexpr uInt32 ADDR_VCS_PLA4_EX = VCSLIB_BASE + 128;
  constexpr uInt32 ADDR_VCS_PLP4_EX = VCSLIB_BASE + 132;
  constexpr uInt32 ADDR_VCS_JMP_TO_RAM3 = VCSLIB_BASE + 136;
  constexpr uInt32 ADDR_VCS_WAIT_FOR_ADDRESS = VCSLIB_BASE + 140;
  constexpr uInt32 ADDR_INJECT_DMA_DATA = VCSLIB_BASE + 144;

  const vector<ElfLinker::ExternalSymbol> EXTERNAL_SYMBOLS = {
    {"ADDR_IDR", ADDR_ADDR_IDR},
    {"DATA_IDR", ADDR_DATA_IDR},
    {"DATA_ODR", ADDR_DATA_ODR},
    {"DATA_MODER", ADDR_DATA_MODER},
    {"memset", ADDR_MEMSET},
    {"memcpy", ADDR_MEMCPY},
    {"vcsLdaForBusStuff2", ADDR_VCS_LDA_FOR_BUS_STUFF2},
    {"vcsLdxForBusStuff2", ADDR_VCS_LDX_FOR_BUS_STUFF2},
    {"vcsLdyForBusStuff2", ADDR_VCS_LDY_FOR_BUS_STUFF2},
    {"vcsWrite3", ADDR_VCS_WRITE3},
    {"vcsJmp3", ADDR_VCS_JMP3},
    {"vcsNop2", ADDR_VCS_NOP2},
    {"vcsNop2n", ADDR_VCS_NOP2N},
    {"vcsWrite5", ADDR_VCS_WRITE5},
    {"vcsWrite6", ADDR_VCS_WRITE6},
    {"vcsLda2", ADDR_VCS_LDA2},
    {"vcsLdx2", ADDR_VCS_LDX2},
    {"vcsLdy2", ADDR_VCS_LDY2},
    {"vcsSax3", ADDR_VCS_SAX3},
    {"vcsSta3", ADDR_VCS_STA3},
    {"vcsStx3", ADDR_VCS_STX3},
    {"vcsSty3", ADDR_VCS_STY3},
    {"vcsSta4", ADDR_VCS_STA4},
    {"vcsStx4", ADDR_VCS_STX4},
    {"vcsSty4", ADDR_VCS_STY4},
    {"vcsCopyOverblankToRiotRam", ADDR_VCS_COPY_OVERBLANK_TO_RIOT_RAM},
    {"vcsStartOverblank", ADDR_VCS_START_OVERBLANK},
    {"vcsEndOverblank", ADDR_VCS_END_OVERBLANK},
    {"vcsRead4", ADDR_VCS_READ4},
    {"randint", ADDR_RANDINT},
    {"vcsTxs2", ADDR_VCS_TXS2},
    {"vcsJsr6", ADDR_VCS_JSR6},
    {"vcsPha3", ADDR_VCS_PHA3},
    {"vcsPhp3", ADDR_VCS_PHP3},
    {"vcsPla4", ADDR_VCS_PLA4},
    {"vcsPlp4", ADDR_VCS_PLP4},
    {"vcsPla4Ex", ADDR_VCS_PLA4_EX},
    {"vcsPlp4Ex", ADDR_VCS_PLP4_EX},
    {"vcsJmpToRam3", ADDR_VCS_JMP_TO_RAM3},
    {"vcsWaitForAddress", ADDR_VCS_WAIT_FOR_ADDRESS},
    {"injectDmaData", ADDR_INJECT_DMA_DATA},
    {"ReverseByte", 0} // FIXME
  };

#ifdef DUMP_ELF
  void dumpElf(const ElfParser& parser)
  {
    cout << std::endl << "ELF sections:" << std::endl << std::endl;

    size_t i = 0;
    for (auto& section: parser.getSections()) {
      if (section.type != 0x00) cout << i << " " << section << std::endl;
      i++;
    }

    auto symbols = parser.getSymbols();
    cout << std::endl << "ELF symbols:" << std::endl << std::endl;
    if (symbols.size() > 0) {
      i = 0;
      for (auto& symbol: symbols)
        cout << (i++) << " " << symbol << std::endl;
    }

    i = 0;
    for (auto& section: parser.getSections()) {
      auto rels = parser.getRelocations(i++);
      if (!rels) continue;

      cout
        << std::endl << "ELF relocations for section "
        << section.name << ":" << std::endl << std::endl;

      for (auto& rel: *rels) cout << rel << std::endl;
    }
  }

  void dumpLinkage(const ElfParser& parser, const ElfLinker& linker)
  {
    cout << std::hex << std::setfill('0');

    cout
      << std::endl
      << "text segment size: 0x" << std::setw(8) << linker.getTextSize()
      << std::endl
      << "data segment size: 0x" << std::setw(8) << linker.getDataSize()
      << std::endl;

    cout << std::endl << "relocated sections:" << std::endl << std::endl;

    const auto& sections = parser.getSections();
    const auto& relocatedSections = linker.getRelocatedSections();

    for (size_t i = 0; i < sections.size(); i++) {
      if (!relocatedSections[i]) continue;

      cout
        << sections[i].name
        << " @ 0x"<< std::setw(8) << (relocatedSections[i]->offset +
          (relocatedSections[i]->segment == ElfLinker::SegmentType::text ? ADDR_TEXT_BASE : ADDR_DATA_BASE)
        )
        << " size 0x" << std::setw(8) << sections[i].size << std::endl;
    }

    cout << std::endl << "relocated symbols:" << std::endl << std::endl;

    const auto& symbols = parser.getSymbols();
    const auto& relocatedSymbols = linker.getRelocatedSymbols();

    for (size_t i = 0; i < symbols.size(); i++) {
      if (!relocatedSymbols[i]) continue;

      cout
        << symbols[i].name
        << " = 0x" << std::setw(8) << relocatedSymbols[i]->value;

      if (relocatedSymbols[i]->segment) {
        cout << (*relocatedSymbols[i]->segment == ElfLinker::SegmentType::text ? " (text)" : " (data)");
      } else {
        cout << " (abs)";
      }

      cout << std::endl;
    }

    cout << std::dec;
  }
#endif
}

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

  ElfLinker elfLinker(ADDR_TEXT_BASE, ADDR_DATA_BASE, elfParser);
  try {
    elfLinker.link(EXTERNAL_SYMBOLS);
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
      << std::endl
      << "ARM entrypoint: 0x"
      << std::hex << std::setw(8) << std::setfill('0') << myArmEntrypoint << std::dec
      << std::endl;
  #endif
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeELF::~CartridgeELF() {}

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

  for (size_t addr = 0; addr < 0x1000; addr += System::PAGE_SIZE) {
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
  for (size_t i = 0; i < sizeof(OVERBLANK_PROGRAM); i++)
    vcsWrite5(0x80 + i, OVERBLANK_PROGRAM[i]);
}

void CartridgeELF::vcsStartOverblank()
{
	myTransactionQueue.injectROM(0x4c);
	myTransactionQueue.injectROM(0x80);
	myTransactionQueue.injectROM(0x00);
  myTransactionQueue.yield(0x0080);
}

CartridgeELF::BusTransaction CartridgeELF::BusTransaction::transactionYield(uInt16 address)
{
  address &= 0x1fff;
  return {.address = address, .value = 0, .yield = true};
}

CartridgeELF::BusTransaction CartridgeELF::BusTransaction::transactionDrive(uInt16 address, uInt8 value)
{
  address &= 0x1fff;
  return {.address = address, .value = value, .yield = false};
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::BusTransaction::setBusState(bool& drive, uInt8& value)
{
  if (yield) {
    drive = false;
  } else {
    drive = true;
    value = this->value;
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


