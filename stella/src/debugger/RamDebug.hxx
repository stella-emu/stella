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
// Copyright (c) 1995-2006 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: RamDebug.hxx,v 1.4 2006-12-08 16:49:04 stephena Exp $
//============================================================================

#ifndef RAM_DEBUG_HXX
#define RAM_DEBUG_HXX

class System;

#include "Array.hxx"
#include "DebuggerSystem.hxx"

class RamState : public DebuggerState
{
  public:
    IntArray ram;
};

class RamDebug : public DebuggerSystem
{
  public:
    RamDebug(Debugger* dbg, Console* console);

    DebuggerState& getState();
    DebuggerState& getOldState() { return myOldState; }

    void saveOldState();

    int read(int offset);
    void write(int offset, int value);

  private:
    RamState myState;
    RamState myOldState;

    System* mySystem;
};

#endif
