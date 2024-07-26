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

#include "exception/FatalEmulationError.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BusTransactionQueue::Transaction BusTransactionQueue::Transaction::transactionYield(uInt16 address)
{
  address &= 0x1fff;
  return {.address = address, .value = 0, .yield = true};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BusTransactionQueue::Transaction BusTransactionQueue::Transaction::transactionDrive(uInt16 address, uInt8 value)
{
  address &= 0x1fff;
  return {.address = address, .value = value, .yield = false};
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
BusTransactionQueue::BusTransactionQueue(size_t capacity) : myQueueCapacity(capacity)
{
  myQueue = make_unique<Transaction[]>(myQueueCapacity);
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BusTransactionQueue& BusTransactionQueue::reset()
{
  myQueueNext = myQueueSize = 0;
  myNextInjectAddress = 0;

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BusTransactionQueue& BusTransactionQueue::setNextInjectAddress(uInt16 address)
{
  myNextInjectAddress = address;

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BusTransactionQueue& BusTransactionQueue::injectROM(uInt8 value)
{
  injectROM(value, myNextInjectAddress);

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BusTransactionQueue& BusTransactionQueue::injectROM(uInt8 value, uInt16 address)
{
  push(Transaction::transactionDrive(address, value));
  myNextInjectAddress = address + 1;

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BusTransactionQueue& BusTransactionQueue::yield(uInt16 address)
{
  push(Transaction::transactionYield(address));

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BusTransactionQueue::hasPendingTransaction() const
{
  return myQueueSize > 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BusTransactionQueue::Transaction* BusTransactionQueue::getNextTransaction(uInt16 address)
{
  if (myQueueSize == 0) return nullptr;

  Transaction* nextTransaction = &myQueue[myQueueNext];
  if (nextTransaction->address != (address & 0x1fff)) return nullptr;

  myQueueNext = (myQueueNext + 1) % myQueueCapacity;
  myQueueSize--;

  return nextTransaction;
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
