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

#include "bspf.hxx"

class CortexM0
{
  public:
    enum class BusTransactionResult : uInt8 {
      ok,
      stopAndRollback,
      stop,
      fail
    };

    class BusTransactionDelegate {
      public:
        virtual ~BusTransactionDelegate() = default;

        virtual BusTransactionResult read32(uInt32 address, uInt32& value) = 0;
        virtual BusTransactionResult read16(uInt32 address, uInt16& value) = 0;

        virtual BusTransactionResult write32(uInt32 address, uInt32 value) = 0;
        virtual BusTransactionResult write16(uInt32 address, uInt16 value) = 0;

        virtual BusTransactionResult fetch16(uInt32 address, uInt16& value, uInt8& op) = 0;
    };

    static constexpr uInt32 PAGE_SIZE = 4096;

  public:
    CortexM0();

    CortexM0& mapRegionData(uInt32 pageBase, uInt32 pageCount,
                            bool readOnly, uInt8* backingStore);

    CortexM0& mapRegionCode(uInt32 pageBase, uInt32 pageCount,
                            bool readOnly, uInt8* backingStore);

    CortexM0& mapRegionDelegate(uInt32 pageBase, uInt32 pageCount,
                                bool readOnly, BusTransactionDelegate* delegate);

    CortexM0& mapDefault(BusTransactionDelegate* delegate);

    CortexM0& reset();
    CortexM0& setRegister(uInt8 regno, uInt32 value);

    static uInt8 decodeInstructionWord(uInt16 instructionWord);

    BusTransactionResult run(uInt32 maxCycles, uInt32& cycles);

  private:

    enum class MemoryRegionType : uInt8 {
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
      uInt8* ops;
    };

    struct MemoryRegion {
      ~MemoryRegion() {
        if (type == MemoryRegionType::directCode)
          std::free(access.accessCode.ops);
      }

      MemoryRegionType type{MemoryRegionType::unmapped};

      uInt32 base;
      uInt32 size;
      bool readOnly;

      union {
        MemoryRegionAccessData accessData;
        MemoryRegionAccessCode accessCode;
        BusTransactionDelegate* delegate;
      } access;
    };

  private:
    MemoryRegion& setupMapping(uInt32 pageBase, uInt32 pageCount,
                               bool readOnly, MemoryRegionType type);

    BusTransactionResult read32(uInt32 address, uInt32& value);
    BusTransactionResult read16(uInt32 address, uInt16& value);

    BusTransactionResult write32(uInt32 address, uInt32 value);
    BusTransactionResult write16(uInt32 address, uInt16 value);

    BusTransactionResult fetch16(uInt32 address, uInt16& value, uInt8& op);

    void do_cvflag(uInt32 a, uInt32 b, uInt32 c);

    int execute();

  private:
    std::array<uInt32, 16> reg_norm; // normal execution mode, do not have a thread mode
    uInt32 znFlags{0};
    uInt32 cFlag{0};
    uInt32 vFlag{0};

    std::array<MemoryRegion, 0x100> myRegions;
    unique_ptr<uInt8[]> myPageMap;
    uInt8 myNextRegionIndex{0};
    BusTransactionDelegate* myDefaultDelegate{nullptr};

    static constexpr uInt32
      CPSR_N = 1u << 31,
      CPSR_Z = 1u << 30,
      CPSR_C = 1u << 29,
      CPSR_V = 1u << 28;

  private:
    // Following constructors and assignment operators not supported
    CortexM0(const CortexM0&) = delete;
    CortexM0(CortexM0&&) = delete;
    CortexM0& operator=(const CortexM0&) = delete;
    CortexM0& operator=(CortexM0&&) = delete;
};

#endif  // CORTEX_M0
