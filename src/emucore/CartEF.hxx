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

#ifndef CARTRIDGE_EF_HXX
#define CARTRIDGE_EF_HXX

class System;

#include "bspf.hxx"
#include "CartEnhanced.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "CartEFWidget.hxx"
#endif

/**
  Cartridge class used for Homestar Runner by Paul Slocum.  The scheme
  provides 16 4K banks (64K total), selected by accessing any address in
  the range $1FE0-$1FEF: the low 4 bits of the address determine the bank
  number (bank 0 at $1FE0, bank 15 at $1FEF).  The name "EF" derives from
  this hotspot range.  DF (32 banks) and BF (64 banks) are direct extensions
  of this same scheme with a wider hotspot window.

  @author  Stephen Anthony, Thomas Jentzsch
*/
class CartridgeEF : public CartridgeEnhanced
{
  friend class CartridgeEFWidget;

  public:
    /**
      Create a new cartridge using the specified image

      @param image     Span of the ROM image
      @param md5       The md5sum of the ROM image
      @param settings  A reference to the various settings (read-only)
      @param bsSize    The size specified by the bankswitching scheme
    */
    CartridgeEF(ByteSpan image, string_view md5,
                const Settings& settings, size_t bsSize = 64_KB);
    ~CartridgeEF() override = default;

  public:
    /**
      Reset device to its power-on state
    */
    void reset() override;

    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const override { return "CartridgeEF"; }

  #ifdef DEBUGGER_SUPPORT
    /**
      Get debugger widget responsible for accessing the inner workings
      of the cart.
    */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
        const GUI::Font& nfont, int x, int y, int w, int h) override
    {
      return new CartridgeEFWidget(boss, lfont, nfont, x, y, w, h, *this);
    }
  #endif

    uInt16 hotspot() const override { return 0x1FE0; }
    bool supportsSaveDisassembly() const override { return true; }

  private:
    bool checkSwitchBank(uInt16 address, uInt8) override;
    uInt16 getStartBank() const override { return 1; }

  private:
    // Following constructors and assignment operators not supported
    CartridgeEF() = delete;
    CartridgeEF(const CartridgeEF&) = delete;
    CartridgeEF(CartridgeEF&&) = delete;
    CartridgeEF& operator=(const CartridgeEF&) = delete;
    CartridgeEF& operator=(CartridgeEF&&) = delete;
};

#endif  // CARTRIDGE_EF_HXX
