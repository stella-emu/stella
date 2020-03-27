//============================================================================
//
// MM     MM  6666  555555  0000   2222
// MMMM MMMM 66  66 55     00  00 22  22
// MM MMM MM 66     55     00  00     22
// MM  M  MM 66666  55555  00  00  22222  --  "A 6502 Microprocessor Emulator"
// MM     MM 66  66     55 00  00 22
// MM     MM 66  66 55  55 00  00 22
// MM     MM  6666   5555   0000  222222
//
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef DEVICE_HXX
#define DEVICE_HXX

class System;

#include "Console.hxx"
#include "Serializable.hxx"
#include "bspf.hxx"

/**
  Abstract base class for devices which can be attached to a 6502
  based system.

  @author  Bradford W. Mott
*/
class Device : public Serializable
{
  public:
    enum AccessType {
      NONE        = 0,
      REFERENCED  = 1 << 0, /* 0x01, code somewhere in the program references it,
                               i.e. LDA $F372 referenced $F372 */
      VALID_ENTRY = 1 << 1, /* 0x02, addresses that can have a label placed in front of it.
                               A good counterexample would be "FF00: LDA $FE00"; $FF01
                               would be in the middle of a multi-byte instruction, and
                               therefore cannot be labelled. */

      // The following correspond to specific types that can be set within the
      // debugger, or specified in a Distella cfg file, and are listed in order
      // of decreasing hierarchy
      //
      CODE  = 1 << 10, // 0x400, disassemble-able code segments
      TCODE = 1 << 9,  // 0x200, (tentative) disassemble-able code segments
      GFX   = 1 << 8,  // 0x100, addresses loaded into GRPx registers
      PGFX  = 1 << 7,  // 0x080, addresses loaded into PFx registers
      COL   = 1 << 6,  // 0x040, addresses loaded into COLUPx registers
      PCOL  = 1 << 5,  // 0x010, addresses loaded into COLUPF register
      BCOL  = 1 << 4,  // 0x010, addresses loaded into COLUBK register
      DATA  = 1 << 3,  // 0x008, addresses loaded into registers other than GRPx / PFx
      ROW   = 1 << 2,  // 0x004, all other addresses
      // special type for poke()
      WRITE = TCODE    // 0x200, address written to
    };
    using AccessFlags = uInt16;

  public:
    Device() = default;
    virtual ~Device() = default;

  public:
    /**
      Reset device to its power-on state.

      *DO NOT* call this method until the device has been attached to
      the System.  In fact, it should never be necessary to call this
      method directly at all.
    */
    virtual void reset() = 0;

    /**
      Notification method invoked by the system when the console type
      has changed.  It may be necessary to override this method for
      devices that want to know about console changes.

      @param timing  Enum representing the new console type
    */
    virtual void consoleChanged(ConsoleTiming timing) { }

    /**
      Install device in the specified system.  Invoked by the system
      when the device is attached to it.

      @param system The system the device should install itself in
    */
    virtual void install(System& system) = 0;

    /**
      Save the current state of this device to the given Serializer.

      @param out  The Serializer object to use
      @return  False on any errors, else true
    */
    virtual bool save(Serializer& out) const override = 0;

    /**
      Load the current state of this device from the given Serializer.

      @param in  The Serializer object to use
      @return  False on any errors, else true
    */
    virtual bool load(Serializer& in) override = 0;

  public:
    /**
      Get the byte at the specified address

      @return The byte at the specified address
    */
    virtual uInt8 peek(uInt16 address) = 0;

    /**
      Change the byte at the specified address to the given value

      @param address The address where the value should be stored
      @param value The value to be stored at the address

      @return  True if the poke changed the device address space, else false
    */
    virtual bool poke(uInt16 address, uInt8 value) { return false; }

    /**
      Query the given address for its access flags

      @param address The address to modify
    */
    virtual AccessFlags getAccessFlags(uInt16 address) const { return AccessType::NONE; }

    /**
      Change the given address type to use the given access flags

      @param address The address to modify
      @param flags   A bitfield of AccessType directives for the given address
    */
    virtual void setAccessFlags(uInt16 address, AccessFlags flags) { }

  protected:
    /// Pointer to the system the device is installed in or the null pointer
    System* mySystem{nullptr};

  private:
    // Following constructors and assignment operators not supported
    Device(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(const Device&) = delete;
    Device& operator=(Device&&) = delete;
};

#endif
