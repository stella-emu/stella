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
// Copyright (c) 1995-1998 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: NullDev.hxx,v 1.1.1.1 2001-12-27 19:54:31 bwmott Exp $
//============================================================================

#ifndef NULLDEVICE_HXX
#define NULLDEVICE_HXX

class System;

#include "bspf.hxx"
#include "Device.hxx"

/**
  Class that represents a "null" device.  The basic idea is that a
  null device is installed in a 6502 based system anywhere there are
  holes in the address space (i.e. no real device attached). 
 
  @author  Bradford W. Mott
  @version $Id: NullDev.hxx,v 1.1.1.1 2001-12-27 19:54:31 bwmott Exp $
*/
class NullDevice : public Device
{
  public:
    /**
      Create a new null device
    */
    NullDevice();

    /**
      Destructor
    */
    virtual ~NullDevice();

  public:
    /**
      Get a null terminated string which is the device's name (i.e. "M6532")

      @return The name of the device
    */
    virtual const char* name() const;

    /**
      Reset device to its power-on state
    */
    virtual void reset();

    /**
      Install device in the specified system.  Invoked by the system
      when the device is attached to it.

      @param system The system the device should install itself in
    */
    virtual void install(System& system);

  public:
    /**
      Get the byte at the specified address

      @return The byte at the specified address
    */
    virtual uInt8 peek(uInt16 address);

    /**
      Change the byte at the specified address to the given value

      @param address The address where the value should be stored
      @param value The value to be stored at the address
    */
    virtual void poke(uInt16 address, uInt8 value);
};
#endif
 
