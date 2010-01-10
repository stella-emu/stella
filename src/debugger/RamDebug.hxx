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
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella team
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
#include "Cart.hxx"
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
    void addRamArea(const RamAreaList& areas);

    const DebuggerState& getState();
    const DebuggerState& getOldState() { return myOldState; }

    void saveOldState();
    string toString();

    // The following assume that the given addresses are using the
    // correct read/write port ranges; no checking will be done to
    // confirm this.
    uInt8 read(uInt16 addr);
    void write(uInt16 addr, uInt8 value);

    // Return the address at which an invalid read was performed in a
    // write port area.
    int readFromWritePort();

  private:
    RamState myState;
    RamState myOldState;

    RamAreaList myRamAreas;
};

#endif
