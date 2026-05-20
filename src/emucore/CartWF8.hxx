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

#ifndef CARTRIDGE_WF8_HXX
#define CARTRIDGE_WF8_HXX

#include "CartEnhanced.hxx"
#ifdef DEBUGGER_SUPPORT
#include "CartWF8Widget.hxx"
#endif

/**
  Cartridge class used for certain Coleco 8K bankswitched games.  WF8 is a
  variant of F8: it reuses the $1FF8 hotspot address but selects the bank
  based on bit D3 of the written value rather than by which of two addresses
  was accessed.  A written value with D3=0 ($00-$07) selects bank 0; D3=1
  ($08-$0F) selects bank 1.  There is only one hotspot address; $1FF9 is not
  used.

  @author  Thomas Jentzsch
*/
class CartridgeWF8 : public CartridgeEnhanced
{
  friend class CartridgeWF4Widget;

  public:
    /**
      Create a new cartridge using the specified image

      @param image     Span of the ROM image
      @param md5       The md5sum of the ROM image
      @param settings  A reference to the various settings (read-only)
      @param bsSize    The size specified by the bankswitching scheme
    */
    CartridgeWF8(ByteSpan image, string_view md5,
                 const Settings& settings, size_t bsSize = 8_KB);
    ~CartridgeWF8() override = default;

    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const override { return "CartridgeWF8"; }

  #ifdef DEBUGGER_SUPPORT
    /**
      Get debugger widget responsible for accessing the inner workings
      of the cart.
    */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
                                 const GUI::Font& nfont, int x, int y,
                                 int w, int h) override
    {
      return new CartridgeWF8Widget(boss, lfont, nfont, x, y, w, h, *this);
    }
  #endif

    uInt16 hotspot() const override { return 0x1FF8; }

  private:
    bool checkSwitchBank(uInt16 address, uInt8 value) override;

  private:
    // Following constructors and assignment operators not supported
    CartridgeWF8() = delete;
    CartridgeWF8(const CartridgeWF8&) = delete;
    CartridgeWF8(CartridgeWF8&&) = delete;
    CartridgeWF8& operator=(const CartridgeWF8&) = delete;
    CartridgeWF8& operator=(CartridgeWF8&&) = delete;
};

#endif  // CARTRIDGE_WF8_HXX
