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

#ifndef CARTRIDGE_FA_HXX
#define CARTRIDGE_FA_HXX

#include "bspf.hxx"
#include "CartEnhanced.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "CartFAWidget.hxx"
#endif

/**
  Cartridge class used for CBS' RAM Plus cartridges (used by Tunnel Runner,
  Omega Race, etc.).  Three 4K banks are selected by accessing $1FF8-$1FFA,
  but with an unusual constraint: bit D0 of the written value must be 1 for
  the switch to fire.  This was an intentional design choice to reduce the
  chance of accidental bank changes from stray writes.

  The 256 bytes of RAM use a larger split window than the SuperChip: write
  port occupies $1000-$10FF and read port occupies $1100-$11FF.  FA2 is an
  extended version of this scheme supporting six or seven banks with
  persistent RAM saved to flash.

  @author  Bradford W. Mott, Thomas Jentzsch
*/
class CartridgeFA : public CartridgeEnhanced
{
  friend class CartridgeFAWidget;

  public:
    /**
      Create a new cartridge using the specified image

      @param image     Span of the ROM image
      @param md5       The md5sum of the ROM image
      @param settings  A reference to the various settings (read-only)
      @param bsSize    The size specified by the bankswitching scheme
    */
    CartridgeFA(ByteSpan image, string_view md5,
                const Settings& settings, size_t bsSize = 12_KB);
    ~CartridgeFA() override = default;

  public:
    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const override { return "CartridgeFA"; }

  #ifdef DEBUGGER_SUPPORT
    /**
      Get debugger widget responsible for accessing the inner workings
      of the cart.
    */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
        const GUI::Font& nfont) override
    {
      return new CartridgeFAWidget(boss, lfont, nfont, *this);
    }
  #endif

    uInt16 hotspot() const override { return 0x1FF8; }
    bool supportsSaveDisassembly() const override { return true; }

  private:
    bool checkSwitchBank(uInt16 address, uInt8) override;
    uInt16 getStartBank() const override { return 2; }

  private:
    // RAM size
    static constexpr size_t RAM_SIZE = 0x100;

  private:
    // Following constructors and assignment operators not supported
    CartridgeFA() = delete;
    CartridgeFA(const CartridgeFA&) = delete;
    CartridgeFA(CartridgeFA&&) = delete;
    CartridgeFA& operator=(const CartridgeFA&) = delete;
    CartridgeFA& operator=(CartridgeFA&&) = delete;
};

#endif  // CARTRIDGE_FA_HXX
