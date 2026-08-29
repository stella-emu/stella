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

#ifndef CARTRIDGE_DF_HXX
#define CARTRIDGE_DF_HXX

class System;

#include "bspf.hxx"
#include "CartEnhanced.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "CartDFWidget.hxx"
#endif

/**
  DF is a direct extension of the EF scheme, doubling the bank count to 32
  (128K total).  Any access in $1FC0-$1FDF switches to the bank whose number
  equals the low 5 bits of the address (bank 0 at $1FC0, bank 31 at $1FDF).
  "DF" derives from this hotspot range.  The DFSC variant adds 128 bytes of
  SuperChip RAM at the standard split window ($1000-$107F write, $1080-$10FF
  read).

  @author  Mike Saarna, Thomas Jentzsch
*/
class CartridgeDF : public CartridgeEnhanced
{
  friend class CartridgeDFWidget;

  public:
    /**
      Create a new cartridge using the specified image

      @param image     Span of the ROM image
      @param md5       The md5sum of the ROM image
      @param settings  A reference to the various settings (read-only)
      @param bsSize    The size specified by the bankswitching scheme
    */
    CartridgeDF(ByteSpan image, string_view md5,
                const Settings& settings, size_t bsSize = 128_KB);
    ~CartridgeDF() override = default;

  public:
    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const override { return "CartridgeDF"; }

  #ifdef DEBUGGER_SUPPORT
    /**
      Get debugger widget responsible for accessing the inner workings
      of the cart.
    */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
        const GUI::Font& nfont) override
    {
      return new CartridgeDFWidget(boss, lfont, nfont, *this);
    }
  #endif

    uInt16 hotspot() const override { return 0x1FC0; }
    bool supportsSaveDisassembly() const override { return true; }

  private:
    bool checkSwitchBank(uInt16 address, uInt8) override;
    uInt16 getStartBank() const override { return 15; }

private:
    // Following constructors and assignment operators not supported
    CartridgeDF() = delete;
    CartridgeDF(const CartridgeDF&) = delete;
    CartridgeDF(CartridgeDF&&) = delete;
    CartridgeDF& operator=(const CartridgeDF&) = delete;
    CartridgeDF& operator=(CartridgeDF&&) = delete;
};

#endif  // CARTRIDGE_DF_HXX
