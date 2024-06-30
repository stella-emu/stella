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

#ifndef CARTRIDGE_ELF
#define CARTRIDGE_ELF

#include "bspf.hxx"
#include "Cart.hxx"

class CartridgeELF: public Cartridge {
  public:
    CartridgeELF(const ByteBuffer& image, size_t size, string_view md5,
                 const Settings& settings);
    virtual ~CartridgeELF();

  // Methods from Device
  public:
    void reset() override;

    void install(System& system) override;

    bool save(Serializer& out) const override;

    bool load(Serializer& in) override;

    uInt8 peek(uInt16 address) override;
    uInt8 peekOob(uInt16 address) override;

    bool poke(uInt16 address, uInt8 value) override;

  // Methods from Cartridge
  public:
    bool bankChanged() override { return false; }

    bool patch(uInt16 address, uInt8 value) override { return false; }

    const ByteBuffer& getImage(size_t& size) const override;

    string name() const override { return "CartridgeELF"; };

    uInt8 overdrivePeek(uInt16 address, uInt8 value) override;

    uInt8 overdrivePoke(uInt16 address, uInt8 value) override;

    bool doesBusStuffing() override { return true; }

  private:
    uInt8 driveBus(uInt16 address, uInt8 value);

    void vcsWrite5(uInt8 zpAddress, uInt8 value);
    void vcsCopyOverblankToRiotRam();
    void vcsStartOverblank();

  private:
    struct BusTransaction {
      static BusTransaction transactionYield(uInt16 address);
      static BusTransaction transactionDrive(uInt16 address, uInt8 value);

      void setBusState(bool& drive, uInt8& value);

      uInt16 address;
      uInt8 value;
      bool yield;
    };

    class BusTransactionQueue {
      public:
        BusTransactionQueue();

        void reset();

        void setNextPushAddress(uInt16 address);
        void injectROM(uInt8 value);
        void injectROM(uInt8 value, uInt16 address);

        void yield(uInt16 address);


        bool hasPendingTransaction() const;
        BusTransaction* getNextTransaction(uInt16 address);

      private:
        void push(const BusTransaction& transaction);

      private:
        unique_ptr<BusTransaction[]> myQueue;
        size_t myQueueNext{0};
        size_t myQueueSize{0};

        uInt16 myNextInjectAddress{0};
    };

  private:
    ByteBuffer myImage;
    size_t myImageSize{0};

    System* mySystem{nullptr};

    unique_ptr<uint8_t[]> myLastPeekResult;
    BusTransactionQueue myTransactionQueue;

    bool myIsBusDriven{false};
    uInt8 myDriveBusValue{0};
};

#endif // CARTRIDGE_ELF
