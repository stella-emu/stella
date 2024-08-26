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

//============================================================================
// This code is based on the "Thumbulator" by David Welch (dwelch@dwelch.com)
// Code is public domain and used with the author's consent
//============================================================================

#ifndef CORTEX_M0
#define CORTEX_M0

#include <variant>

#include "Serializable.hxx"
#include "bspf.hxx"

class Serializer;

class CortexM0: public Serializable
{
  public:
    using err_t = uInt64;

    class BusTransactionDelegate {
      public:
        BusTransactionDelegate() = default;
        virtual ~BusTransactionDelegate() = default;

        virtual err_t read32(uInt32 address, uInt32& value, CortexM0& cortex);
        virtual err_t read16(uInt32 address, uInt16& value, CortexM0& cortex);
        virtual err_t read8(uInt32 address, uInt8& value, CortexM0& cortex);

        virtual err_t write32(uInt32 address, uInt32 value, CortexM0& cortex);
        virtual err_t write16(uInt32 address, uInt16 value, CortexM0& cortex);
        virtual err_t write8(uInt32 address, uInt8 value, CortexM0& cortex);

        virtual err_t fetch16(uInt32 address, uInt16& value, uInt8& op, CortexM0& cortex);

      private:
        // Following constructors and assignment operators not supported
        BusTransactionDelegate(const BusTransactionDelegate&) = delete;
        BusTransactionDelegate(BusTransactionDelegate&&) = delete;
        BusTransactionDelegate& operator=(const BusTransactionDelegate&) = delete;
        BusTransactionDelegate& operator=(BusTransactionDelegate&&) = delete;
    };

    static constexpr uInt32 PAGE_SIZE = 4096;

    static constexpr err_t ERR_NONE = 0;
    static constexpr err_t ERR_UNMAPPED_READ32 = 1;
    static constexpr err_t ERR_UNMAPPED_READ16 = 2;
    static constexpr err_t ERR_UNMAPPED_READ8 = 3;
    static constexpr err_t ERR_UNMAPPED_WRITE32 = 4;
    static constexpr err_t ERR_UNMAPPED_WRITE16 = 5;
    static constexpr err_t ERR_UNMAPPED_WRITE8 = 6;
    static constexpr err_t ERR_UNMAPPED_FETCH16 = 7;
    static constexpr err_t ERR_WRITE_ACCESS_DENIED = 8;
    static constexpr err_t ERR_ACCESS_ALIGNMENT_FAULT = 9;
    static constexpr err_t ERR_BKPT = 10;
    static constexpr err_t ERR_INVALID_OPERATING_MODE = 11;
    static constexpr err_t ERR_UNIMPLEMENTED_INST = 12;
    static constexpr err_t ERR_SWI = 13;
    static constexpr err_t ERR_UNDEFINED_INST = 14;

    static constexpr bool isErrCustom(err_t err) {
      return (err & 0xff) == 0;
    }

    static constexpr uInt32 getErrCustom(err_t err)  {
      return (err & 0xffffffff) >> 8;
    }

    static constexpr uInt8 getErrInstrinsic(err_t err)  {
      return err;
    }

    static constexpr uInt32 getErrExtra(err_t err) {
      return err >> 32;
    }

    static constexpr err_t errCustom(uInt32 code, uInt32 extra = 0) {
      return ((static_cast<uInt64>(code) << 8) & 0xffffffff) | (static_cast<uInt64>(extra) << 32);
    }

    static constexpr err_t errIntrinsic(uInt8 code, uInt32 extra = 0) {
      return static_cast<uInt64>(code) | (static_cast<uInt64>(extra) << 32);
    }

    static string describeError(err_t error);

  public:
    CortexM0();
    ~CortexM0() override = default;

    CortexM0& resetMappings();

    CortexM0& mapRegionData(uInt32 pageBase, uInt32 pageCount,
                            bool readOnly, uInt8* backingStore);

    CortexM0& mapRegionCode(uInt32 pageBase, uInt32 pageCount,
                            bool readOnly, uInt8* backingStore);

    CortexM0& mapRegionDelegate(uInt32 pageBase, uInt32 pageCount,
                                bool readOnly, BusTransactionDelegate* delegate);

    CortexM0& mapDefault(BusTransactionDelegate* delegate);

    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;
    void saveDirtyRegions(Serializer& out) const;
    void loadDirtyRegions(Serializer& in);

    CortexM0& reset();
    CortexM0& setPc(uInt32 pc);
    CortexM0& setRegister(uInt8 regno, uInt32 value);
    uInt32 getRegister(uInt32 regno) const;

    bool getN() const;
    bool getZ() const;
    bool getC() const;
    bool getV() const;

    static uInt8 decodeInstructionWord(uInt16 instructionWord);

    err_t run(uInt32 maxCycles, uInt32& cycles);

    err_t read32(uInt32 address, uInt32& value);
    err_t read16(uInt32 address, uInt16& value);
    err_t read8(uInt32 address, uInt8& value);

    err_t write32(uInt32 address, uInt32 value);
    err_t write16(uInt32 address, uInt16 value);
    err_t write8(uInt32 address, uInt8 value);

    uInt64 getCycles() const {
      return myCycleCounter;
    }

  private:

    enum class MemoryRegionType: uInt8 {
      directData,
      directCode,
      delegate,
      unmapped
    };

    struct MemoryRegionAccessData {
      uInt8* backingStore;
    };

    struct MemoryRegionAccessCode {
      uInt8* backingStore;
      unique_ptr<uInt8[]> ops;
    };

    struct MemoryRegion {
      MemoryRegion() = default;
      ~MemoryRegion() = default;

      MemoryRegionType type{MemoryRegionType::unmapped};

      uInt32 base{0};
      uInt32 size{0};
      bool readOnly{false};

      bool dirty{false};
      uInt32 accessWatermarkLow{static_cast<uInt32>(~0)};
      uInt32 accessWatermarkHigh{0};

      std::variant<
        MemoryRegionAccessData,   // ::get<0>, directData
        MemoryRegionAccessCode,   // ::get<1>, directCode
        BusTransactionDelegate*,  // ::get<2>, delegate
        std::monostate
      > access;

      void reset();
      void saveDirtyBits(Serializer& out) const;
      void loadDirtyBits(Serializer& in);

      private:
        MemoryRegion(const MemoryRegion&) = delete;
        MemoryRegion(MemoryRegion&&) = delete;
        MemoryRegion& operator=(const MemoryRegion&) = delete;
        MemoryRegion& operator=(MemoryRegion&&) = delete;
    };

  private:
    MemoryRegion& setupMapping(uInt32 pageBase, uInt32 pageCount,
                               bool readOnly, MemoryRegionType type);

    void recompileCodeRegions();

    err_t fetch16(uInt32 address, uInt16& value, uInt8& op);

    void do_cvflag(uInt32 a, uInt32 b, uInt32 c);

    err_t execute(uInt16 inst, uInt8 op);

  private:
    std::array<uInt32, 16> reg_norm{}; // normal execution mode, do not have a thread mode
    uInt32 znFlags{0};
    uInt32 cFlag{0};
    uInt32 vFlag{0};

    std::array<MemoryRegion, 0x100> myRegions{};
    unique_ptr<uInt8[]> myPageMap;
    uInt8 myNextRegionIndex{0};
    BusTransactionDelegate* myDefaultDelegate{nullptr};

    uInt64 myCycleCounter{0};

    static constexpr uInt32
      CPSR_N = 1U << 31,
      CPSR_Z = 1U << 30,
      CPSR_C = 1U << 29,
      CPSR_V = 1U << 28;

  private:
    // Following constructors and assignment operators not supported
    CortexM0(const CortexM0&) = delete;
    CortexM0(CortexM0&&) = delete;
    CortexM0& operator=(const CortexM0&) = delete;
    CortexM0& operator=(CortexM0&&) = delete;
};

#endif  // CORTEX_M0
