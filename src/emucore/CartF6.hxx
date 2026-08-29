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

#ifndef CARTRIDGE_F6_HXX
#define CARTRIDGE_F6_HXX

#include "CartEnhanced.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "CartF6Widget.hxx"
#endif

/**
  Cartridge class used for Atari's 16K bankswitched games.  Four 4K banks
  are selected by accessing $1FF6-$1FF9 (the low 2 bits of the address give
  the bank number).  F6 is a direct extension of F8 (8K) and is itself
  extended by F4 (32K).  Used by titles such as Dig Dug, Joust, and Pole
  Position.  The F6SC variant adds 128 bytes of SuperChip RAM.

  @author  Bradford W. Mott, Thomas Jentzsch
*/
class CartridgeF6 : public CartridgeEnhanced
{
  friend class CartridgeF6Widget;

  public:
    /**
      Create a new cartridge using the specified image

      @param image     Span of the ROM image
      @param md5       The md5sum of the ROM image
      @param settings  A reference to the various settings (read-only)
      @param bsSize    The size specified by the bankswitching scheme
    */
    CartridgeF6(ByteSpan image, string_view md5,
                const Settings& settings, size_t bsSize = 16_KB);
    ~CartridgeF6() override = default;

  public:
    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const override { return "CartridgeF6"; }

  #ifdef DEBUGGER_SUPPORT
    /**
      Get debugger widget responsible for accessing the inner workings
      of the cart.
    */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
        const GUI::Font& nfont) override
    {
      return new CartridgeF6Widget(boss, lfont, nfont, *this);
    }
  #endif

    uInt16 hotspot() const override { return 0x1FF6; }
    bool supportsSaveDisassembly() const override { return true; }

  private:
    bool checkSwitchBank(uInt16 address, uInt8 value) override;

  private:
    // Following constructors and assignment operators not supported
    CartridgeF6() = delete;
    CartridgeF6(const CartridgeF6&) = delete;
    CartridgeF6(CartridgeF6&&) = delete;
    CartridgeF6& operator=(const CartridgeF6&) = delete;
    CartridgeF6& operator=(CartridgeF6&&) = delete;
};

#endif  // CARTRIDGE_F6_HXX
