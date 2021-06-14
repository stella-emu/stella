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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef CARTRIDGE_ARM_HXX
#define CARTRIDGE_ARM_HXX

#include "Thumbulator.hxx"
#include "Cart.hxx"

/**
  Abstract base class for ARM carts.

  @author  Thomas Jentzsch
*/
class CartridgeARM : public Cartridge
{
  friend class CartridgeARMWidget;

  public:
    CartridgeARM(const string& md5, const Settings& settings);
    ~CartridgeARM() override = default;

  protected:
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

    // Get number of memory accesses of last and last but one ARM runs.
    void updateCycles(int cycles);
    const Thumbulator::Stats& stats() const { return myStats; }
    const Thumbulator::Stats& prevStats() const { return myPrevStats; }

    void incCycles(bool enable);
    void cycleFactor(double factor);
    double cycleFactor() const { return myThumbEmulator->cycleFactor(); }


  protected:
    // Pointer to the Thumb ARM emulator object
    unique_ptr<Thumbulator> myThumbEmulator;

    // ARM code increases 6507 cycles
    bool myIncCycles{false};

    Thumbulator::Stats myStats{0};
    Thumbulator::Stats myPrevStats{0};

  private:
    // Following constructors and assignment operators not supported
    CartridgeARM() = delete;
    CartridgeARM(const CartridgeARM&) = delete;
    CartridgeARM(CartridgeARM&&) = delete;
    CartridgeARM& operator=(const CartridgeARM&) = delete;
    CartridgeARM& operator=(CartridgeARM&&) = delete;
};

#endif
