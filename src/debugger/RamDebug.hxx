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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef RAM_DEBUG_HXX
#define RAM_DEBUG_HXX

class System;

#include "bspf.hxx"
#include "Array.hxx"
#include "DebuggerSystem.hxx"

// pointer types for RamDebug instance methods
typedef int (RamDebug::*RAMDEBUG_INT_METHOD)();

// call the pointed-to method on the (global) CPU debugger object.
#define CALL_RAMDEBUG_METHOD(method) ( ( Debugger::debugger().ramDebug().*method)() )

class RamState : public DebuggerState
{
  public:
    IntArray ram;    // The actual data values
    IntArray rport;  // Address for reading from RAM
    IntArray wport;  // Address for writing to RAM
};

class RamDebug : public DebuggerSystem
{
  public:
    RamDebug(Debugger& dbg, Console& console);

    /**
      Let the RAM debugger subsystem treat this area as addressable memory.

      @param start    The beginning of the RAM area (0x0000 - 0x2000)
      @param size     Total number of bytes of area
      @param roffset  Offset to use when reading from RAM (read port)
      @param woffset  Offset to use when writing to RAM (write port)
    */
    void addRamArea(uInt16 start, uInt16 size, uInt16 roffset, uInt16 woffset);

    const DebuggerState& getState();
    const DebuggerState& getOldState() { return myOldState; }

    void saveOldState();
    string toString();

    // The following assume that the given addresses are using the
    // correct read/write port ranges; no checking will be done to
    // confirm this.
    uInt8 read(uInt16 addr);
    void write(uInt16 addr, uInt8 value);

    // These methods are used by the debugger when we wish to know
    // if an illegal read from a write port has been performed.
    // It's up to each Cartridge to report the error, and a
    // conditional breakpoint must be set in the debugger to check
    // for occurrences of this.
    //
    // Note that each time readFromWritePort() returns a hit, the status
    // is reset.
    int readFromWritePort();
    void setReadFromWritePort(uInt16 address);

  private:
    RamState myState;
    RamState myOldState;

    uInt16 myReadFromWritePortAddress;
};

#endif
