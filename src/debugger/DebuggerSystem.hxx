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

#ifndef DEBUGGER_SYSTEM_HXX
#define DEBUGGER_SYSTEM_HXX

#include "Debugger.hxx"
#include "Console.hxx"

/**
  The DebuggerState class is used as a base class for state in all
  DebuggerSystem objects.  We make it a class so we can take advantage
  of the copy constructor.
 */
class DebuggerState
{
  public:
    DebuggerState()  { }
    ~DebuggerState() { }
};

/**
  The base class for all debugger objects.  Its real purpose is to
  clean up the Debugger API, partitioning it into separate
  subsystems.
 */
class DebuggerSystem
{
  public:
    DebuggerSystem(Debugger& dbg, Console& console) :
      myDebugger(dbg), myConsole(console), mySystem(console.system()) { }
    virtual ~DebuggerSystem() { }

    virtual const DebuggerState& getState() = 0;
    virtual const DebuggerState& getOldState() = 0;

    virtual void saveOldState() = 0;
    virtual string toString() = 0;

  protected:
    Debugger& myDebugger;
    Console& myConsole;
    System& mySystem;
};

#endif
