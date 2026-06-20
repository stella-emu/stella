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

#ifndef THUMB_BUS_DELEGATE_HXX
#define THUMB_BUS_DELEGATE_HXX

#include "bspf.hxx"

class Thumbulator;

/**
  Interface for things the Thumbulator core cannot service on its own and must
  hand back to the owning cartridge.  Modeled on CortexM0::BusTransactionDelegate,
  so the legacy ARM core moves toward the same decoupled structure as the newer
  one: the core no longer needs to know which cartridge type it is running.

  Currently this covers the 32-bit ARM driver routines reached via a branch into
  code the Thumb core cannot execute.  Unlike CortexM0 (whose ELF carts service
  memory accesses via a delegate), Harmony carts never service Thumbulator memory
  accesses -- the only non-RAM/ROM addresses are the fixed LPC2103 peripherals,
  which are chip-level and stay inside the core -- so no read/write hooks are
  needed here.

  @author  Stephen Anthony
*/
class ThumbBusDelegate
{
  public:
    ThumbBusDelegate() = default;
    virtual ~ThumbBusDelegate() = default;

    /**
      Called when the core branches into 32-bit ARM code it cannot execute.
      The delegate matches 'pc' against its own driver entry points and uses
      the register/cycle accessors on 'core' to read arguments, write results
      and account for the routine's cost.

      @param pc    The pipeline-adjusted address branched to
      @param core  The calling Thumbulator, for register/cycle access
      @return  True if the routine was emulated and execution should resume
               after the call; false to stop the core (custom code complete,
               or an unhandled address)
    */
    virtual bool armBranch(uInt32 pc, Thumbulator& core) = 0;

  private:
    // Following constructors and assignment operators not supported
    ThumbBusDelegate(const ThumbBusDelegate&) = delete;
    ThumbBusDelegate(ThumbBusDelegate&&) = delete;
    ThumbBusDelegate& operator=(const ThumbBusDelegate&) = delete;
    ThumbBusDelegate& operator=(ThumbBusDelegate&&) = delete;
};

#endif  // THUMB_BUS_DELEGATE_HXX
