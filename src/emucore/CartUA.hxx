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

#ifndef CARTRIDGE_UA_HXX
#define CARTRIDGE_UA_HXX

#include "bspf.hxx"
#include "CartEnhanced.hxx"
#include "System.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "CartUAWidget.hxx"
#endif

/**
  Cartridge class used for UA Limited's 8K bankswitched games.  Like the
  0840 and SB schemes, UA places its hotspots below $1000 rather than near
  $1FF0: $0220 selects bank 0 and $0240 selects bank 1.  Brazilian licensed
  hardware used shifted addresses ($02A0/$02C0); additional aliases are also
  accepted.  A swapHotspots flag reverses which address maps to which bank,
  accommodating images that were dumped with banks in reversed order.  Known
  titles: Funky Fish, Pleiades.

  @author  Bradford W. Mott, Thomas Jentzsch
*/
class CartridgeUA : public CartridgeEnhanced
{
  friend class CartridgeUAWidget;

  public:
    /**
      Create a new cartridge using the specified image

      @param image         Span of the ROM image
      @param md5           The md5sum of the ROM image
      @param settings      A reference to the various settings (read-only)
      @param swapHotspots  Swap hotspots
    */
    CartridgeUA(ByteSpan image, string_view md5,
                const Settings& settings, bool swapHotspots = false);
    ~CartridgeUA() override = default;

  public:
    /**
      Install cartridge in the specified system.  Invoked by the system
      when the cartridge is attached to it.

      @param system The system the device should install itself in
    */
    void install(System& system) override;

    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const override {
      return mySwappedHotspots ? "CartridgeUASW" : "CartridgeUA";
    }

  #ifdef DEBUGGER_SUPPORT
    /**
      Get debugger widget responsible for accessing the inner workings
      of the cart.
    */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
        const GUI::Font& nfont, int x, int y, int w, int h) override
    {
      return new CartridgeUAWidget(boss, lfont, nfont, x, y, w, h, *this, mySwappedHotspots);
    }
  #endif

  public:
    /**
      Get the byte at the specified address.

      @return The byte at the specified address
    */
    uInt8 peek(uInt16 address) override;

    /**
      Change the byte at the specified address to the given value

      @param address The address where the value should be stored
      @param value The value to be stored at the address
      @return  True if the poke changed the device address space, else false
    */
    bool poke(uInt16 address, uInt8 value) override;

    uInt16 hotspot() const override { return 0x0220; }

  private:
    bool checkSwitchBank(uInt16 address, uInt8) override;

  private:
    // Previous Device's page access
    std::array<System::PageAccess, 2> myHotSpotPageAccess;

    // Indicates if banks are swapped ("Mickey" cart)
    bool mySwappedHotspots{false};

  private:
    // Following constructors and assignment operators not supported
    CartridgeUA() = delete;
    CartridgeUA(const CartridgeUA&) = delete;
    CartridgeUA(CartridgeUA&&) = delete;
    CartridgeUA& operator=(const CartridgeUA&) = delete;
    CartridgeUA& operator=(CartridgeUA&&) = delete;
};

#endif  // CARTRIDGE_UA_HXX
