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
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef DEVICE_HXX
#define DEVICE_HXX

class System;

#include "Serializable.hxx"
#include "bspf.hxx"

/**
  Abstract base class for devices which can be attached to a 6502
  based system.

  @author  Bradford W. Mott
  @version $Id$
*/
class Device : public Serializable
{
  public:
    /**
      Create a new device
    */
    Device();

    /**
      Destructor
    */
    virtual ~Device();

  public:
    /**
      Reset device to its power-on state.

      *DO NOT* call this method until the device has been attached to
      the System.  In fact, it should never be necessary to call this
      method directly at all.
    */
    virtual void reset() = 0;

    /**
      Notification method invoked by the system right before the
      system resets its cycle counter to zero.  It may be necessary 
      to override this method for devices that remember cycle counts.
    */
    virtual void systemCyclesReset();

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
    virtual bool save(Serializer& out) const = 0;

    /**
      Load the current state of this device from the given Serializer.

      @param in  The Serializer object to use
      @return  False on any errors, else true
    */
    virtual bool load(Serializer& in) = 0;

    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    virtual string name() const = 0;

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
    */
    virtual void poke(uInt16 address, uInt8 value) = 0;

  protected:
    /// Pointer to the system the device is installed in or the null pointer
    System* mySystem;
};

#endif
