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

#ifndef CARTRIDGE_F4_HXX
#define CARTRIDGE_F4_HXX

#include "CartEnhanced.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "CartF4Widget.hxx"
#endif

/**
  Cartridge class used for Atari's 32K bankswitched games.  Eight 4K banks
  are selected by accessing $1FF4-$1FFB: the low 3 bits of the address give
  the bank number (bank 0 at $1FF4, bank 7 at $1FFB).  F4 is a direct
  extension of the F6 (16K) and F8 (8K) schemes.  Used by Atari titles such
  as Mario Bros., Super Breakout, and Star Wars.  The F4SC variant adds 128
  bytes of SuperChip RAM.

  @author  Bradford W. Mott, Thomas Jentzsch
*/
class CartridgeF4 : public CartridgeEnhanced
{
  friend class CartridgeF4Widget;

  public:
    /**
      Create a new cartridge using the specified image

      @param image     Span of the ROM image
      @param md5       The md5sum of the ROM image
      @param settings  A reference to the various settings (read-only)
      @param bsSize    The size specified by the bankswitching scheme
    */
    CartridgeF4(ByteSpan image, string_view md5,
                const Settings& settings, size_t bsSize = 32_KB);
    ~CartridgeF4() override = default;

  public:
    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const override { return "CartridgeF4"; }

  #ifdef DEBUGGER_SUPPORT
    /**
      Get debugger widget responsible for accessing the inner workings
      of the cart.
    */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
        const GUI::Font& nfont) override
    {
      return new CartridgeF4Widget(boss, lfont, nfont, *this);
    }
  #endif

    uInt16 hotspot() const override { return 0x1FF4; }
    bool supportsSaveDisassembly() const override { return true; }

  private:
    bool checkSwitchBank(uInt16 address, uInt8) override;

private:
    // Following constructors and assignment operators not supported
    CartridgeF4() = delete;
    CartridgeF4(const CartridgeF4&) = delete;
    CartridgeF4(CartridgeF4&&) = delete;
    CartridgeF4& operator=(const CartridgeF4&) = delete;
    CartridgeF4& operator=(CartridgeF4&&) = delete;
};

#endif  // CARTRIDGE_F4_HXX
