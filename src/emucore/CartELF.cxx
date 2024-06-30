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

namespace {
  constexpr size_t READ_STREAM_CAPACITY = 1024;

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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeELF::CartridgeELF(const ByteBuffer& image, size_t size, string_view md5,
                           const Settings& settings)
  : Cartridge(settings, md5), myImageSize(size)
{
  myImage = make_unique<uInt8[]>(size);
  std::memcpy(myImage.get(), image.get(), size);

  myLastPeekResult = make_unique<uInt8[]>(0x1000);
  std::fill_n(myLastPeekResult.get(), 0x1000, 0);

  createRomAccessArrays(0x1000);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeELF::~CartridgeELF() {}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::reset()
{
  std::fill_n(myLastPeekResult.get(), 0x1000, 0);

  myReadStream.reset();
	myReadStream.push(0x00, 0x0ffc);
	myReadStream.push(0x10);
  myReadStream.setNextPushAddress(0);

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
  if (myReadStream.isYield()) return mySystem->getDataBusState();

  myLastPeekResult[address & 0xfff] = myReadStream.pop(address);
  return myLastPeekResult[address & 0xfff];
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::vcsWrite5(uInt8 zpAddress, uInt8 value)
{
	myReadStream.push(0xa9);
	myReadStream.push(value);
	myReadStream.push(0x85);
	myReadStream.push(zpAddress);
  myReadStream.yield();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::vcsCopyOverblankToRiotRam()
{
  for (size_t i = 0; i < sizeof(OVERBLANK_PROGRAM); i++)
    vcsWrite5(0x80 + i, OVERBLANK_PROGRAM[i]);
}

void CartridgeELF::vcsStartOverblank()
{
	myReadStream.push(0x4c);
	myReadStream.push(0x80);
	myReadStream.push(0x00);
  myReadStream.yield();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeELF::ReadStream::ReadStream()
{
  myStream = make_unique<ScheduledRead[]>(READ_STREAM_CAPACITY);
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::ReadStream::reset()
{
  myStreamNext = myStreamSize = myNextPushAddress = 0;
  myIsYield = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::ReadStream::setNextPushAddress(uInt16 address)
{
  myNextPushAddress = address;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::ReadStream::push(uInt8 value)
{
  if (myNextPushAddress > 0xfff)
    throw FatalEmulationError("read stream pointer overflow");

  push(value, myNextPushAddress);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::ReadStream::push(uInt8 value, uInt16 address)
{
  if (myStreamSize == READ_STREAM_CAPACITY)
    throw FatalEmulationError("read stream overflow");

  address &= 0xfff;

  myStream[(myStreamNext + myStreamSize++) % READ_STREAM_CAPACITY] =
    {.address = address, .value = value, .yield = false};

  myNextPushAddress = address + 1;
  myIsYield = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELF::ReadStream::yield()
{
  if (myStreamSize == 0)
    throw new FatalEmulationError("yield called on empty stream");

  myStream[(myStreamNext + myStreamSize - 1) % READ_STREAM_CAPACITY].yield = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeELF::ReadStream::isYield() const
{
  return myIsYield;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeELF::ReadStream::hasPendingRead() const
{
  return myStreamSize > 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeELF::ReadStream::pop(uInt16 readAddress)
{
  if (myStreamSize == 0) {
    ostringstream s;
    s << "read stream underflow at 0x" << std::hex << std::setw(4) << readAddress;
    throw FatalEmulationError(s.str());
  }

  if ((readAddress & 0xfff) != myStream[myStreamNext].address)
  {
    ostringstream s;
    s <<
      "unexcpected cartridge read from 0x" << std::hex << std::setw(4) <<
      std::setfill('0') << (readAddress & 0xfff) << " expected 0x" << myStream[myStreamNext].address;

    throw FatalEmulationError(s.str());
  }

  myIsYield = myStream[myStreamNext].yield && myStreamSize == 1;
  const uInt8 value = myStream[myStreamNext++].value;

  myStreamNext %= READ_STREAM_CAPACITY;
  myStreamSize--;

  return value;
}
