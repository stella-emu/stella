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
#include "CortexM0.hxx"
#include "BusTransactionQueue.hxx"
#include "VcsLib.hxx"

class ElfLinker;

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

    void consoleChanged(ConsoleTiming timing) override;

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
    uInt64 getArmCycles() const;

    uInt8 driveBus(uInt16 address, uInt8 value);
    void syncClock(const BusTransactionQueue::Transaction& transaction);

    void parseAndLinkElf();
    void setupMemoryMap();

    uInt32 getCoreClock() const;
    uInt32 getSystemType() const;
    void jumpToMain();

    void runArm();

  private:
    ByteBuffer myImage;
    size_t myImageSize{0};

    System* mySystem{nullptr};

    unique_ptr<uint8_t[]> myLastPeekResult;
    BusTransactionQueue myTransactionQueue;

    bool myIsBusDriven{false};
    uInt8 myDriveBusValue{0};

    uInt32 myArmEntrypoint{0};

    CortexM0 myCortexEmu;
    unique_ptr<ElfLinker> myLinker;

    unique_ptr<uInt8[]> mySectionStack;
    unique_ptr<uInt8[]> mySectionText;
    unique_ptr<uInt8[]> mySectionData;
    unique_ptr<uInt8[]> mySectionRodata;
    unique_ptr<uInt8[]> mySectionTables;

    VcsLib myVcsLib;

    ConsoleTiming myConsoleTiming{ConsoleTiming::ntsc};
    uInt32 myArmCyclesPer6502Cycle{80};

    Int64 myArmCyclesOffset{0};
};

#endif // CARTRIDGE_ELF
