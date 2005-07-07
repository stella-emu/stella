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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: CpuDebug.hxx,v 1.1 2005-07-07 18:56:41 stephena Exp $
//============================================================================

#ifndef CPU_DEBUG_HXX
#define CPU_DEBUG_HXX

class System;

#include "Array.hxx"
#include "Console.hxx"
#include "DebuggerSystem.hxx"

class CpuState : public DebuggerState
{
  public:
    int PC, SP, PS, A, X, Y;
    BoolArray PSbits;
};

class CpuDebug : public DebuggerSystem
{
  public:
    CpuDebug(Console* console);

    DebuggerState& getState();
    DebuggerState& getOldState() { return myOldState; }

    void saveOldState();

    void setA(int a);
    void setX(int x);
    void setY(int y);
    void setSP(int sp);
    void setPC(int pc);

  private:
    CpuState myState;
    CpuState myOldState;

    System* mySystem;
};

#endif
