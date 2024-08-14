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

#include "VcsLib.hxx"

#include <cmath>

#include "BusTransactionQueue.hxx"
#include "ElfEnvironment.hxx"
#include "exception/FatalEmulationError.hxx"

using namespace elfEnvironment;

namespace {
  CortexM0::err_t memset(uInt32 target, uInt8 value, uInt32 size, CortexM0& cortex)
  {
    const uInt16 value16 = value | (value << 8);
    const uInt32 value32 = value16 | (value16 << 16);
    CortexM0::err_t err = CortexM0::ERR_NONE;
    uInt32 ptr = target;

    while (ptr < target + size) {
      if ((ptr & 0x03) == 0 && size - (ptr - target) >= 4) {
        err = cortex.write32(ptr, value32);
        ptr += 4;
      }
      else if ((ptr & 0x01) == 0 && size - (ptr - target) >= 2) {
        err = cortex.write16(ptr, value16);
        ptr += 2;
      }
      else {
        err = cortex.write8(ptr, value);
        ptr++;
      }

      if (err) return err;
    }

    cortex.setRegister(0, target);
    return 0;
  }

  CortexM0::err_t memcpy(uInt32 dest, uInt32 src, uInt32 size, CortexM0& cortex)
  {
    CortexM0::err_t err = CortexM0::ERR_NONE;
    const uInt32 destOrig = dest;

    while (size > 0) {
      if (((dest | src) & 0x03) == 0 && size >= 4) {
        uInt32 value = 0;

        err = cortex.read32(src, value);
        if (err) return err;

        err = cortex.write32(dest, value);

        size -= 4;
        dest += 4;
        src += 4;
      }
      else if (((dest | src) & 0x01) == 0 && size >= 2) {
        uInt16 value = 0;

        err = cortex.read16(src, value);
        if (err) return err;

        err = cortex.write16(dest, value);

        size -= 2;
        dest += 2;
        src += 2;
      }
      else {
        uInt8 value = 0;

        err = cortex.read8(src, value);
        if (err) return err;

        err = cortex.write8(dest, value);

        size--;
        dest++;
        src++;
      }

      if (err) return err;
    }

    cortex.setRegister(0, destOrig);
    return 0;
  }
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VcsLib::VcsLib(BusTransactionQueue& transactionQueue)
  : myTransactionQueue{transactionQueue}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VcsLib::save(Serializer& out) const
{
  try {
    out.putByte(myStuffMaskA);
    out.putByte(myStuffMaskX);
    out.putByte(myStuffMaskY);
    out.putBool(myIsWaitingForRead);
    out.putShort(myWaitingForReadAddress);
    out.putShort(myCurrentAddress);
    out.putBool(myCurrentValue);

    if (!myRand.save(out)) return false;
  }
  catch (...) {
    cerr << "ERROR: failed to save vcslib\n";
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VcsLib::load(Serializer& in)
{
  reset();

  try {
    myStuffMaskA = in.getByte();
    myStuffMaskX = in.getByte();
    myStuffMaskY = in.getByte();
    myIsWaitingForRead = in.getBool();
    myWaitingForReadAddress = in.getShort();
    myCurrentAddress = in.getShort();
    myCurrentValue = in.getBool();

    if (!myRand.load(in)) return false;
  }
  catch(...) {
    cerr << "ERROR: failed to load vcslib\n";
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VcsLib::reset()
{
  myStuffMaskA = myStuffMaskX = myStuffMaskY = 0x00;
  myIsWaitingForRead = false;
  myWaitingForReadAddress = 0;
  myCurrentAddress = 0;
  myCurrentValue = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VcsLib::vcsWrite5(uInt8 zpAddress, uInt8 value)
{
	myTransactionQueue
    .injectROM(0xa9)
	  .injectROM(value)
	  .injectROM(0x85)
	  .injectROM(zpAddress)
    .yield(zpAddress);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VcsLib::vcsCopyOverblankToRiotRam()
{
  for (uInt8 i = 0; i < OVERBLANK_PROGRAM_SIZE; i++)
    vcsWrite5(0x80 + i, OVERBLANK_PROGRAM[i]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VcsLib::vcsStartOverblank()
{
	myTransactionQueue
    .injectROM(0x4c)
	  .injectROM(0x80)
	  .injectROM(0x00)
    .yield(0x0080);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VcsLib::vcsEndOverblank()
{
  myTransactionQueue
    .injectROMAt(0x00, 0x1fff)
    .yield(0x00ac)
    .setNextInjectAddress(0x1000);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VcsLib::vcsNop2n(uInt16 n)
{
  if (n == 0) return;

  myTransactionQueue
    .injectROM(0xea)
    .setNextInjectAddress(myTransactionQueue.getNextInjectAddress() + n - 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VcsLib::vcsLda2(uInt8 value)
{
  myTransactionQueue
	  .injectROM(0xa9)
    .injectROM(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0::err_t VcsLib::stackOperation(uInt16& value, uInt8& op, uInt8 opcode)
{
  myTransactionQueue
    .injectROM(opcode)
    .yield(0, 0x1000);

  return returnFromStub(value, op);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0::err_t VcsLib::fetch16(uInt32 address, uInt16& value, uInt8& op, CortexM0& cortex)
{
  uInt32 arg = 0;
  CortexM0::err_t err = CortexM0::ERR_NONE;

  if (myTransactionQueue.size() >= elfEnvironment::QUEUE_SIZE_LIMIT)
    return CortexM0::errCustom(ERR_STOP_EXECUTION);

  myTransactionQueue.setTimestamp(cortex.getCycles());

  switch (address) {
    case ADDR_MEMSET:
      err = memset(cortex.getRegister(0), cortex.getRegister(1), cortex.getRegister(2), cortex);
      if (err) return err;

      return returnFromStub(value, op);

    case ADDR_MEMCPY:
      err = memcpy(cortex.getRegister(0), cortex.getRegister(1), cortex.getRegister(2), cortex);
      if (err) return err;

      return returnFromStub(value, op);

    case ADDR_VCS_LDA_FOR_BUS_STUFF2:
      vcsLda2(myStuffMaskA);
      return returnFromStub(value, op);

    case ADDR_VCS_LDX_FOR_BUS_STUFF2:
      vcsLda2(myStuffMaskX);
      return returnFromStub(value, op);

    case ADDR_VCS_LDY_FOR_BUS_STUFF2:
      vcsLda2(myStuffMaskY);
      return returnFromStub(value, op);

    case ADDR_VCS_WRITE3:
      arg = cortex.getRegister(0);

      myTransactionQueue
        .injectROM(0x85)
        .injectROM(arg)
        .stuffByte(cortex.getRegister(1), arg);

      return returnFromStub(value, op);

    case ADDR_VCS_JMP3:
      myTransactionQueue
    	  .injectROM(0x4c)
        .injectROM(0x00)
        .injectROM(0x10)
        .setNextInjectAddress(0x1000);

      return returnFromStub(value, op);

    case ADDR_VCS_NOP2:
      myTransactionQueue.injectROM(0xea);
      return returnFromStub(value, op);

    case ADDR_VCS_NOP2N:
      vcsNop2n(cortex.getRegister(0));
      return returnFromStub(value, op);

    case ADDR_VCS_WRITE5:
      vcsWrite5(cortex.getRegister(0), cortex.getRegister(1));
      return returnFromStub(value, op);

    case ADDR_VCS_WRITE6:
      arg = cortex.getRegister(0);

      myTransactionQueue
    	  .injectROM(0xa9)
	      .injectROM(cortex.getRegister(1))
	      .injectROM(0x8d)
	      .injectROM(arg)
	      .injectROM(arg >> 8)
	      .yield(arg);

      return returnFromStub(value, op);

    case ADDR_VCS_LDA2:
      vcsLda2(cortex.getRegister(0));
      return returnFromStub(value, op);

    case ADDR_VCS_LDX2:
      myTransactionQueue
	      .injectROM(0xa2)
        .injectROM(cortex.getRegister(0));

      return returnFromStub(value, op);

    case ADDR_VCS_LDY2:
      myTransactionQueue
	      .injectROM(0xa0)
        .injectROM(cortex.getRegister(0));

      return returnFromStub(value, op);

    case ADDR_VCS_SAX3:
      arg = cortex.getRegister(0);

      myTransactionQueue
        .injectROM(0x87)
	      .injectROM(arg)
	      .yield(arg);

      return returnFromStub(value, op);

    case ADDR_VCS_STA3:
      arg = cortex.getRegister(0);

      myTransactionQueue
    	  .injectROM(0x85)
        .injectROM(arg)
        .yield(arg);

      return returnFromStub(value, op);

    case ADDR_VCS_STX3:
      arg = cortex.getRegister(0);

      myTransactionQueue
    	  .injectROM(0x86)
        .injectROM(arg)
        .yield(arg);

      return returnFromStub(value, op);

    case ADDR_VCS_STY3:
      arg = cortex.getRegister(0);

      myTransactionQueue
    	  .injectROM(0x84)
        .injectROM(arg)
        .yield(arg);

      return returnFromStub(value, op);

    case ADDR_VCS_STA4:
      arg = cortex.getRegister(0);

      myTransactionQueue
      	.injectROM(0x8d)
	      .injectROM(arg)
	      .injectROM(arg >> 8)
	      .yield(arg);

      return returnFromStub(value, op);

    case ADDR_VCS_STX4:
      arg = cortex.getRegister(0);

      myTransactionQueue
      	.injectROM(0x8e)
	      .injectROM(arg)
	      .injectROM(arg >> 8)
	      .yield(arg);

      return returnFromStub(value, op);

    case ADDR_VCS_STY4:
      arg = cortex.getRegister(0);

      myTransactionQueue
      	.injectROM(0x8c)
	      .injectROM(arg)
	      .injectROM(arg >> 8)
	      .yield(arg);

      return returnFromStub(value, op);

    case ADDR_VCS_COPY_OVERBLANK_TO_RIOT_RAM:
      vcsCopyOverblankToRiotRam();
      return returnFromStub(value, op);

    case ADDR_VCS_START_OVERBLANK:
      vcsStartOverblank();
      return returnFromStub(value, op);

    case ADDR_VCS_END_OVERBLANK:
      vcsEndOverblank();
      return returnFromStub(value, op);

    case ADDR_VCS_READ4:
      if (myIsWaitingForRead) {
        if (myTransactionQueue.size() > 0 || myCurrentAddress != myWaitingForReadAddress)
          return CortexM0::errCustom(ERR_STOP_EXECUTION);

        myIsWaitingForRead = false;
        cortex.setRegister(0, myCurrentValue);

        return returnFromStub(value, op);
      } else {
        arg = cortex.getRegister(0);

        myIsWaitingForRead = true;
        myWaitingForReadAddress = arg;

        myTransactionQueue
          .injectROM(0xad)
	        .injectROM(arg)
	        .injectROM(arg >> 8)
          .yield(arg);

        return CortexM0::errCustom(ERR_STOP_EXECUTION);
      }

    case ADDR_RANDINT:
      cortex.setRegister(0, myRand.next());
      return returnFromStub(value, op);

    case ADDR_VCS_TXS2:
      myTransactionQueue.injectROM(0x9a);
      return returnFromStub(value, op);

    case ADDR_VCS_JSR6:
      arg = cortex.getRegister(0);

      myTransactionQueue
        .injectROM(0x20)
        .injectROM(arg)
        .yield(0, 0x1000)
        .injectROM(arg >> 8)
        .setNextInjectAddress(arg & 0x1fff);

      return returnFromStub(value, op);

    case ADDR_VCS_PHA3:
      return stackOperation(value, op, 0x48);

    case ADDR_VCS_PHP3:
      return stackOperation(value, op, 0x08);

    case ADDR_VCS_PLA4:
      return stackOperation(value, op, 0x68);

    case ADDR_VCS_PLP4:
      return stackOperation(value, op, 0x28);

    case ADDR_VCS_PLA4_EX:
      FatalEmulationError::raise("unimplemented: vcsPla4Ex");

    case ADDR_VCS_PLP4_EX:
      FatalEmulationError::raise("unimplemented: vcsPlp4Ex");

    case ADDR_VCS_JMP_TO_RAM3:
      arg = cortex.getRegister(0);

      myTransactionQueue
      	.injectROM(0x4c)
      	.injectROM(arg)
      	.injectROM(arg >> 8)
      	.yield(arg);

      return returnFromStub(value, op);

    case ADDR_VCS_WAIT_FOR_ADDRESS:
      FatalEmulationError::raise("unimplemented: vcsWaitForAddress");

    case ADDR_INJECT_DMA_DATA:
      FatalEmulationError::raise("unimplemented: vcsInjectDmaData");

    default:
      return CortexM0::errIntrinsic(CortexM0::ERR_UNMAPPED_FETCH16, address);
  }
}
