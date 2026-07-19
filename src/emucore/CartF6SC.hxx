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

#ifndef CARTRIDGE_F6SC_HXX
#define CARTRIDGE_F6SC_HXX

#include "CartF6.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "CartF6SCWidget.hxx"
#endif

/**
  F6SC is F6 with 128 bytes of SuperChip RAM.  Banking works identically to
  F6: four 4K banks (16K total), selected by accessing $1FF6-$1FF9.  The SC
  RAM uses the standard split window: write port $1000-$107F, read port
  $1080-$10FF.

  @author  Bradford W. Mott, Thomas Jentzsch
*/
class CartridgeF6SC : public CartridgeF6
{
  friend class CartridgeF6SCWidget;

  public:
    /**
      Create a new cartridge using the specified image

      @param image     Span of the ROM image
      @param md5       The md5sum of the ROM image
      @param settings  A reference to the various settings (read-only)
      @param bsSize    The size specified by the bankswitching scheme
    */
    CartridgeF6SC(ByteSpan image, string_view md5,
                  const Settings& settings, size_t bsSize = 16_KB);
    ~CartridgeF6SC() override = default;

  public:
    /**
      Query whether the cart RAM allows code execution.

      @return  true, if code execution is allowed
    */
    bool executableCartRam() const override { return false; }

    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const override { return "CartridgeF6SC"; }

  #ifdef DEBUGGER_SUPPORT
    /**
      Get debugger widget responsible for accessing the inner workings
      of the cart.
    */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
        const GUI::Font& nfont) override
    {
      return new CartridgeF6SCWidget(boss, lfont, nfont, *this);
    }
  #endif

  private:
    // RAM size
    static constexpr size_t RAM_SIZE = 0x80;

  private:
    // Following constructors and assignment operators not supported
    CartridgeF6SC() = delete;
    CartridgeF6SC(const CartridgeF6SC&) = delete;
    CartridgeF6SC(CartridgeF6SC&&) = delete;
    CartridgeF6SC& operator=(const CartridgeF6SC&) = delete;
    CartridgeF6SC& operator=(CartridgeF6SC&&) = delete;
};

#endif  // CARTRIDGE_F6SC_HXX
