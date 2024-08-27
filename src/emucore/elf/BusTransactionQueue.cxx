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

#include "BusTransactionQueue.hxx"

#include "Serializable.hxx"
#include "exception/FatalEmulationError.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BusTransactionQueue::Transaction BusTransactionQueue::Transaction::transactionYield(
  uInt16 address, uInt64 timestamp, uInt16 mask
) {
  address &= 0x1fff;
  return {.address = address, .mask = mask, .value = 0, .timestamp = timestamp, .yield = true};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BusTransactionQueue::Transaction BusTransactionQueue::Transaction::transactionDrive(
  uInt16 address, uInt8 value, uInt64 timestamp
) {
  address &= 0x1fff;
  return {.address = address, .mask = 0xffff, .value = value, .timestamp = timestamp, .yield = false};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BusTransactionQueue::Transaction::setBusState(bool& bs_drive, uInt8& bs_value) const
{
  if (yield) {
    bs_drive = false;
  } else {
    bs_drive = true;
    bs_value = this->value;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BusTransactionQueue::BusTransactionQueue(size_t capacity)
  : myQueueCapacity{capacity},
    myQueue{make_unique<Transaction[]>(myQueueCapacity)}
{
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BusTransactionQueue& BusTransactionQueue::reset()
{
  myQueueNext = myQueueSize = 0;
  myNextInjectAddress = 0;
  myTimestamp = 0;

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BusTransactionQueue::save(Serializer& out) const
{
  try {
    out.putLong(myQueueSize);
    out.putShort(myNextInjectAddress);
    out.putLong(myTimestamp);

    for (size_t i = 0; i < myQueueSize; i++)
      myQueue[(myQueueNext + i) % myQueueCapacity].serialize(out);
  }
  catch (...) {
    cerr << "ERROR: failed to save bus transaction queue\n";
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BusTransactionQueue::load(Serializer& in)
{
  try {
    reset();

    myQueueSize = in.getLong();
    myNextInjectAddress = in.getShort();
    myTimestamp = in.getLong();

    if (myQueueSize > myQueueCapacity) return false;

    for (size_t i = 0; i < myQueueSize; i++)
      myQueue[i].deserialize(in);
  }
  catch(...) {
    cerr << "ERROR: failed to load bus transaction queue\n";
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BusTransactionQueue::Transaction::serialize(Serializer& out) const
{
  out.putShort(address);
  out.putShort(mask);
  out.putByte(value);
  out.putLong(timestamp);
  out.putBool(yield);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BusTransactionQueue::Transaction::deserialize(Serializer& in)
{
  address = in.getShort();
  mask = in.getShort();
  value = in.getByte();
  timestamp = in.getLong();
  yield = in.getBool();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BusTransactionQueue& BusTransactionQueue::setNextInjectAddress(uInt16 address)
{
  myNextInjectAddress = address;

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 BusTransactionQueue::getNextInjectAddress() const
{
  return myNextInjectAddress;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BusTransactionQueue& BusTransactionQueue::setTimestamp(uInt64 timestamp)
{
  myTimestamp = timestamp;

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BusTransactionQueue& BusTransactionQueue::injectROM(uInt8 value)
{
  injectROMAt(value, myNextInjectAddress);

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BusTransactionQueue& BusTransactionQueue::injectROMAt(uInt8 value, uInt16 address)
{
  push(Transaction::transactionDrive(address, value, myTimestamp));
  myNextInjectAddress = address + 1;

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BusTransactionQueue& BusTransactionQueue::stuffByte(uInt8 value, uInt16 address)
{
  push(Transaction::transactionDrive(address, value, myTimestamp));

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BusTransactionQueue& BusTransactionQueue::yield(uInt16 address, uInt16 mask)
{
  push(Transaction::transactionYield(address, myTimestamp, mask));

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BusTransactionQueue::hasPendingTransaction() const
{
  return myQueueSize > 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BusTransactionQueue::Transaction* BusTransactionQueue::getNextTransaction(uInt16 address, uInt64 timestamp)
{
  if (myQueueSize == 0) return nullptr;

  Transaction* nextTransaction = &myQueue[myQueueNext];
  if (
    nextTransaction->address != (address & 0x1fff & nextTransaction->mask) ||
    nextTransaction->timestamp > timestamp
  ) return nullptr;

  myQueueNext = (myQueueNext + 1) % myQueueCapacity;
  myQueueSize--;

  return nextTransaction;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BusTransactionQueue::Transaction* BusTransactionQueue::peekNextTransaction()
{
  return myQueueSize > 0 ? &myQueue[myQueueNext] : nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BusTransactionQueue::push(const Transaction& transaction)
{
  if (myQueueSize > 0) {
    Transaction& lastTransaction = myQueue[(myQueueNext + myQueueSize - 1) % myQueueCapacity];

    if (lastTransaction.address == transaction.address) {
      lastTransaction = transaction;
      return;
    }
  }

  if (myQueueSize == myQueueCapacity)
    throw FatalEmulationError("read stream overflow");

  myQueue[(myQueueNext + myQueueSize++) % myQueueCapacity] = transaction;
}
