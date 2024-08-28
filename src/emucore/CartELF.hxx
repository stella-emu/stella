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
#include "ElfEnvironment.hxx"
#include "ElfParser.hxx"
#include "BusTransactionQueue.hxx"
#include "VcsLib.hxx"
#include "System.hxx"

class ElfLinker;

#ifdef DEBUGGER_SUPPORT
  class CartridgeELFWidget;
  class CartridgeELFStateWidget;
#endif

class CartridgeELF: public Cartridge {
#ifdef DEBUGGER_SUPPORT
  friend CartridgeELFWidget;
  friend CartridgeELFStateWidget;
#endif

  public:
    static constexpr uInt32 MIPS_MAX = 300;
    static constexpr uInt32 MIPS_MIN = 50;
    static constexpr uInt32 MIPS_DEF = 150;

  public:
    CartridgeELF(const ByteBuffer& image, size_t size, string_view md5,
                 const Settings& settings);
    ~CartridgeELF() override = default;

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

    string name() const override { return "CartridgeELF"; }

    uInt8 overdrivePeek(uInt16 address, uInt8 value) override;

    uInt8 overdrivePoke(uInt16 address, uInt8 value) override;

    bool doesBusStuffing() override { return true; }

#ifdef DEBUGGER_SUPPORT
    CartDebugWidget* debugWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont, int x, int y, int w, int h
    ) override;

    CartDebugWidget* infoWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont, int x, int y, int w, int h
    ) override;
#endif

  public:
    string getDebugLog() const;

    std::pair<unique_ptr<uInt8[]>, size_t> getArmImage() const;

  private:
    class BusFallbackDelegate: public CortexM0::BusTransactionDelegate {
      public:
        CortexM0::err_t fetch16(uInt32 address, uInt16& value, uInt8& op, CortexM0& cortex) override;
        CortexM0::err_t read8(uInt32 address, uInt8& value, CortexM0& cortex) override;
        CortexM0::err_t read16(uInt32 address, uInt16& value, CortexM0& cortex) override;
        CortexM0::err_t read32(uInt32 address, uInt32& value, CortexM0& cortex) override;
        CortexM0::err_t write8(uInt32 address, uInt8 value, CortexM0& cortex) override;
        CortexM0::err_t write16(uInt32 address, uInt16 value, CortexM0& cortex) override;
        CortexM0::err_t write32(uInt32 address, uInt32 value, CortexM0& cortex) override;

        void setErrorsAreFatal(bool fatal);

      private:
        CortexM0::err_t handleError(
          string_view accessType, uInt32 address, CortexM0::err_t err, CortexM0& cortex
        ) const;

      private:
        bool myErrorsAreFatal{false};
    };

    enum class ExecutionStage: uInt8 {
      boot, preinit, init, main
    };

  private:
    void setupConfig();
    void resetWithConfig();

    uInt64 getArmCycles() const {
      return myCortexEmu.getCycles() + myArmCyclesOffset;
    }
    uInt64 getVcsCyclesArm() const {
      return mySystem->cycles() * myArmCyclesPer6502Cycle;
    }

    uInt8 driveBus(uInt16 address, uInt8 value);
    void syncArmTime(uInt64 armCycles);

    void parseAndLinkElf();
    void allocationSections();
    void setupMemoryMap(bool strictMode);

    void switchExecutionStage();
    void callFn(uInt32 ptr, uInt32 sp);
    void callMain();

    void runArm();

  private:
    ByteBuffer myImage;
    size_t myImageSize{0};

    System* mySystem{nullptr};

    bool myConfigStrictMode{false};
    uInt32 myConfigMips{100};
    elfEnvironment::SystemType myConfigSystemType{elfEnvironment::SystemType::ntsc};

    unique_ptr<uint8_t[]> myLastPeekResult;
    BusTransactionQueue myTransactionQueue;

    bool myIsBusDriven{false};
    uInt8 myDriveBusValue{0};

    uInt32 myArmEntrypoint{0};

    CortexM0 myCortexEmu;
    ElfParser myElfParser;
    unique_ptr<ElfLinker> myLinker;

    unique_ptr<uInt8[]> mySectionStack;
    unique_ptr<uInt8[]> mySectionText;
    unique_ptr<uInt8[]> mySectionData;
    unique_ptr<uInt8[]> mySectionRodata;
    unique_ptr<uInt8[]> mySectionTables;

    VcsLib myVcsLib;
    BusFallbackDelegate myFallbackDelegate;

    ConsoleTiming myConsoleTiming{ConsoleTiming::ntsc};
    uInt32 myArmCyclesPer6502Cycle{100};

    Int64 myArmCyclesOffset{0};

    ExecutionStage myExecutionStage{ExecutionStage::boot};
    uInt32 myInitFunctionIndex{0};

  private:
    // Following constructors and assignment operators not supported
    CartridgeELF(const CartridgeELF&) = delete;
    CartridgeELF(CartridgeELF&&) = delete;
    CartridgeELF& operator=(const CartridgeELF&) = delete;
    CartridgeELF& operator=(CartridgeELF&&) = delete;

};

#endif // CARTRIDGE_ELF
