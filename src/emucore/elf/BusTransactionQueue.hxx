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

#ifndef BUS_TRANSACTION_QUEUE
#define BUS_TRANSACTION_QUEUE

#include "Serializable.hxx"
#include "bspf.hxx"

class Serializer;

class BusTransactionQueue: public Serializable {
  public:
    struct Transaction {
      static Transaction transactionYield(uInt16 address, uInt64 timestamp, uInt16 mask);
      static Transaction transactionDrive(uInt16 address, uInt8 value, uInt64 timestamp);

      void setBusState(bool& drive, uInt8& value) const;

      uInt16 address{0};
      uInt16 mask{0xffff};
      uInt8 value{0};
      uInt64 timestamp{0};
      bool yield{false};

      void serialize(Serializer& out) const;
      void deserialize(Serializer& in);
    };

  public:
    explicit BusTransactionQueue(size_t capacity);
    ~BusTransactionQueue() override = default;

    BusTransactionQueue& reset();

    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;

    BusTransactionQueue& setNextInjectAddress(uInt16 address);
    uInt16 getNextInjectAddress() const;

    BusTransactionQueue& setTimestamp(uInt64 timestamp);
    BusTransactionQueue& injectROM(uInt8 value);
    BusTransactionQueue& injectROMAt(uInt8 value, uInt16 address);
    BusTransactionQueue& stuffByte(uInt8 value, uInt16 address);

    BusTransactionQueue& yield(uInt16 address, uInt16 mask = 0xffff);

    bool hasPendingTransaction() const;
    Transaction* getNextTransaction(uInt16 address, uInt64 timestamp);
    Transaction* peekNextTransaction();

    size_t size() const {
      return myQueueSize;
    }

  private:
    void push(const Transaction& transaction);

  private:
    const size_t myQueueCapacity{0};

    unique_ptr<Transaction[]> myQueue;
    size_t myQueueNext{0};
    size_t myQueueSize{0};

    uInt16 myNextInjectAddress{0};
    uInt64 myTimestamp{0};

  private:
    BusTransactionQueue(const BusTransactionQueue&) = delete;
    BusTransactionQueue(BusTransactionQueue&&) = delete;
    BusTransactionQueue& operator=(const BusTransactionQueue&) = delete;
    BusTransactionQueue& operator=(BusTransactionQueue&&) = delete;
};

#endif // BUS_TRANSACTION_QUEUE
