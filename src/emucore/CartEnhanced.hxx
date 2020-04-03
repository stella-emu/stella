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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef CARTRIDGEENHANCED_HXX
#define CARTRIDGEENHANCED_HXX

class System;

#include "bspf.hxx"
#include "Cart.hxx"

/**
  Enhanced cartridge base class used for multiple cart types.

  @author  Thomas Jentzsch
*/
class CartridgeEnhanced : public Cartridge
{
  public:
    /**
      Create a new cartridge using the specified image

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
      @param md5       The md5sum of the ROM image
      @param settings  A reference to the various settings (read-only)
    */
    CartridgeEnhanced(const ByteBuffer& image, size_t size, const string& md5,
                const Settings& settings);
    virtual ~CartridgeEnhanced() = default;

  public:
    /**
      Install cartridge in the specified system.  Invoked by the system
      when the cartridge is attached to it.

      @param system The system the device should install itself in
    */
    void install(System& system) override;

    /**
      Reset device to its power-on state
    */
    void reset() override;


    /**
      Install pages for the specified bank in the system.

      @param bank The bank that should be installed in the system
    */
    bool bank(uInt16 bank, uInt16 slice);

    /**
      Install pages for the specified bank in the system.

      @param bank The bank that should be installed in the system
    */
    bool bank(uInt16 bank) override { return this->bank(bank, 0); }

    /**
      Get the current bank.

      @param address The address to use when querying the bank
    */
    uInt16 getBank(uInt16 address = 0) const override;

    /**
      Query the number of banks supported by the cartridge.
    */
    uInt16 bankCount() const override;

    /**
      Patch the cartridge ROM.

      @param address  The ROM address to patch
      @param value    The value to place into the address
      @return    Success or failure of the patch operation
    */
    bool patch(uInt16 address, uInt8 value) override;

    /**
      Access the internal ROM image for this cartridge.

      @param size  Set to the size of the internal ROM image data
      @return  A pointer to the internal ROM image data
    */
    const uInt8* getImage(size_t& size) const override;

    /**
      Save the current state of this cart to the given Serializer.

      @param out  The Serializer object to use
      @return  False on any errors, else true
    */
    bool save(Serializer& out) const override;

    /**
      Load the current state of this cart from the given Serializer.

      @param in  The Serializer object to use
      @return  False on any errors, else true
    */
    bool load(Serializer& in) override;

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

  protected:
    // Pointer to a dynamically allocated ROM image of the cartridge
    ByteBuffer myImage{nullptr};

    // Pointer to a dynamically allocated RAM area of the cartridge
    ByteBuffer myRAM{nullptr};

    uInt16 myBankShift{BANK_SHIFT};

    uInt16 myBankSize{BANK_SIZE};

    uInt16 myBankMask{BANK_MASK};

    uInt16 myRamSize{RAM_SIZE};

    uInt16 myRamMask{RAM_MASK};

    bool myDirectPeek{true};

    // Indicates the offset into the ROM image (aligns to current bank)
    uInt16 myBankOffset{0};

    // Indicates the slice mapped into each of the bank segments
    WordBuffer myCurrentBankOffset{nullptr};

  private:
    // log(ROM bank size) / log(2)
    static constexpr uInt16 BANK_SHIFT = 12;

    // bank size
    static constexpr uInt16 BANK_SIZE = 1 << BANK_SHIFT; // 2 ^ 12 = 4K

    // bank mask
    static constexpr uInt16 BANK_MASK = BANK_SIZE - 1;

    // bank segments
    static constexpr uInt16 BANK_SEGS = 1;

    // RAM size
    static constexpr uInt16 RAM_SIZE = 0;

    // RAM mask
    static constexpr uInt16 RAM_MASK = 0;

    // Size of the ROM image
    size_t mySize{0};

  protected:
    /**
      Check hotspots and switch bank if triggered.
    */
    virtual bool checkSwitchBank(uInt16 address, uInt8 value = 0) = 0;

  private:
    virtual uInt16 getStartBank() const { return 0; }

    virtual uInt16 romHotspot() const { return 0; }

  private:
    // Following constructors and assignment operators not supported
    CartridgeEnhanced() = delete;
    CartridgeEnhanced(const CartridgeEnhanced&) = delete;
    CartridgeEnhanced(CartridgeEnhanced&&) = delete;
    CartridgeEnhanced& operator=(const CartridgeEnhanced&) = delete;
    CartridgeEnhanced& operator=(CartridgeEnhanced&&) = delete;
};

#endif
