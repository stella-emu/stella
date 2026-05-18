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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef CARTRIDGE_DEVCARD_HXX
#define CARTRIDGE_DEVCARD_HXX

class System;

#include "bspf.hxx"
#include "Cart.hxx"

/**
  The DevCard (DevKit) is a 7800 development cartridge that widens the
  System bus to 16 bits and exposes 24K of flat RAM across six
  non-contiguous 4K windows where both A14|A15 and A12 are high:

    0x5000-0x5FFF  0x7000-0x7FFF  0x9000-0x9FFF
    0xB000-0xBFFF  0xD000-0xDFFF  0xF000-0xFFFF

  Windows with A12 low (0x4000, 0x6000, ...) are not mapped to avoid
  bus contention with TIA/RIOT.  All mapped windows are directly
  readable and writable.  There is no bankswitching.

  The 24KB ROM image initialises the RAM on reset; the six 4K chunks
  are stored sequentially starting at image offset 0.

  @author  Stephen Anthony
*/
class CartridgeDevCard : public Cartridge
{
  public:
    /**
      Create a new DevCard cartridge using the specified image.

      @param image     Span of the ROM image (initialises RAM on reset)
      @param md5       The md5sum of the ROM image
      @param settings  A reference to the various settings (read-only)
    */
    CartridgeDevCard(ByteSpan image, string_view md5, const Settings& settings);
    ~CartridgeDevCard() override = default;

  public:
    void reset() override;
    void install(System& system) override;
    uInt8 peek(uInt16 address) override;
    bool poke(uInt16 address, uInt8 value) override;
    bool patch(uInt16 address, uInt8 value) override;
    ByteSpan getImage() const override;
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;
    string name() const override { return "CartridgeDevCard"; }

  #ifdef DEBUGGER_SUPPORT
    uInt16 bankOrigin(uInt16 bank, uInt16 PC = 0) const override;
  #endif

  private:
    // Six 4K windows: 0x5000, 0x7000, 0x9000, 0xB000, 0xD000, 0xF000
    static constexpr size_t NUM_WINDOWS = 6;
    static constexpr size_t WINDOW_SIZE = 4_KB;
    static constexpr size_t RAM_SIZE    = NUM_WINDOWS * WINDOW_SIZE;  // 24KB

    static constexpr std::array<uInt16, NUM_WINDOWS> WINDOWS = {
      0x5000, 0x7000, 0x9000, 0xB000, 0xD000, 0xF000
    };

    // Map a CPU address to its RAM offset
    static uInt32 ramOffset(uInt16 address) {
      // Windows are at x000 for x in {5,7,9,B,D,F}:
      // windowIdx = ((addr >> 12) - 5) / 2
      const uInt32 windowIdx = (static_cast<uInt32>(address >> 12) - 5U) / 2U;
      return windowIdx * WINDOW_SIZE + (address & (WINDOW_SIZE - 1));
    }

    // Working RAM (initialised from ROM image, writable at runtime)
    std::array<uInt8, RAM_SIZE> myRAM{};

    // Original ROM image for re-initialisation on reset
    std::vector<uInt8> myImage;

  private:
    // Following constructors and assignment operators not supported
    CartridgeDevCard() = delete;
    CartridgeDevCard(const CartridgeDevCard&) = delete;
    CartridgeDevCard(CartridgeDevCard&&) = delete;
    CartridgeDevCard& operator=(const CartridgeDevCard&) = delete;
    CartridgeDevCard& operator=(CartridgeDevCard&&) = delete;
};

#endif  // CARTRIDGE_DEVCARD_HXX
