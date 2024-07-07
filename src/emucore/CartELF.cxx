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

#ifdef DUMP_ELF
  void dumpElf(const ElfParser& elfParser) {
    cout << "ELF sections:" << std::endl << std::endl;

    size_t i = 0;
    for (auto& section: elfParser.getSections()) {
      if (section.type != 0x00) cout << i << " " << section << std::endl;
      i++;
    }

    auto symbols = elfParser.getSymbols();
    cout << std::endl << "ELF symbols:" << std::endl << std::endl;
    if (symbols.size() > 0) {
      i = 0;
      for (auto& symbol: symbols)
        cout << (i++) << " " << symbol << std::endl;
    }

    i = 0;
    for (auto& section: elfParser.getSections()) {
      auto rels = elfParser.getRelocations(i++);
      if (!rels) continue;

      cout
        << std::endl << "ELF relocations for section "
        << section.name << ":" << std::endl << std::endl;

      for (auto& rel: *rels) cout << rel << std::endl;
    }
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeELF::CartridgeELF(const ByteBuffer& image, size_t size, string_view md5,
                           const Settings& settings)
  : Cartridge(settings, md5), myImageSize(size)
{
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


