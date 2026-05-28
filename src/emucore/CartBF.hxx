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

#ifndef CARTRIDGE_BF_HXX
#define CARTRIDGE_BF_HXX

class System;

#include "bspf.hxx"
#include "CartEnhanced.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "CartBFWidget.hxx"
#endif

/**
  BF is a further extension of the EF/DF family, providing 64 4K banks
  (256K total).  Any access in $1F80-$1FBF switches to the bank whose number
  equals the low 6 bits of the address (bank 0 at $1F80, bank 63 at $1FBF).
  "BF" derives from this hotspot range.  The BFSC variant adds 128 bytes of
  SuperChip RAM at the standard split window ($1000-$107F write, $1080-$10FF
  read).

  @author  Mike Saarna, Thomas Jentzsch
*/
class CartridgeBF : public CartridgeEnhanced
{
  friend class CartridgeBFWidget;

  public:
    /**
      Create a new cartridge using the specified image

      @param image     Span of the ROM image
      @param md5       The md5sum of the ROM image
      @param settings  A reference to the various settings (read-only)
      @param bsSize    The size specified by the bankswitching scheme
    */
    CartridgeBF(ByteSpan image, string_view md5,
                const Settings& settings, size_t bsSize = 256_KB);
    ~CartridgeBF() override = default;

  public:
    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const override { return "CartridgeBF"; }

  #ifdef DEBUGGER_SUPPORT
    /**
      Get debugger widget responsible for accessing the inner workings
      of the cart.
    */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
        const GUI::Font& nfont, int x, int y, int w, int h) override
    {
      return new CartridgeBFWidget(boss, lfont, nfont, x, y, w, h, *this);
    }
  #endif

    uInt16 hotspot() const override { return 0x1F80; }
    bool supportsSaveDisassembly() const override { return true; }

  private:
    bool checkSwitchBank(uInt16 address, uInt8) override;
    uInt16 getStartBank() const override { return 1; }

  private:
    // Following constructors and assignment operators not supported
    CartridgeBF() = delete;
    CartridgeBF(const CartridgeBF&) = delete;
    CartridgeBF(CartridgeBF&&) = delete;
    CartridgeBF& operator=(const CartridgeBF&) = delete;
    CartridgeBF& operator=(CartridgeBF&&) = delete;
};

#endif  // CARTRIDGE_BF_HXX
