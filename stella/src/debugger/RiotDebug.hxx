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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: RiotDebug.hxx,v 1.1 2008-04-19 21:11:52 stephena Exp $
//============================================================================

#ifndef RIOT_DEBUG_HXX
#define RIOT_DEBUG_HXX

class Debugger;
class RiotDebug;

#include "Array.hxx"
#include "DebuggerSystem.hxx"

class RiotState : public DebuggerState
{
  public:
    uInt8 SWCHA, SWCHB, SWACNT, SWBCNT;
    BoolArray swchaBits;
    BoolArray swchbBits;
    BoolArray swacntBits;
    BoolArray swbcntBits;

    uInt8 TIM1T, TIM8T, TIM64T, TIM1024T, INTIM, TIMINT;
    Int32 TIMCLKS;
};

class RiotDebug : public DebuggerSystem
{
  public:
    RiotDebug(Debugger& dbg, Console& console);

    const DebuggerState& getState();
    const DebuggerState& getOldState() { return myOldState; }

    void saveOldState();
    string toString();

    /* Port A and B registers */
    uInt8 swcha(int newVal = -1);
    uInt8 swchb(int newVal = -1);
    uInt8 swacnt(int newVal = -1);
    uInt8 swbcnt(int newVal = -1);

    /* Timer registers & associated clock */
    uInt8 tim1T(int newVal = -1);
    uInt8 tim8T(int newVal = -1);
    uInt8 tim64T(int newVal = -1);
    uInt8 tim1024T(int newVal = -1);
    uInt8 intim();
    uInt8 timint();
    Int32 timClocks();

    /* Port A description */
    string dirP0String();
    string dirP1String();

    /* Port B description */
    string diffP0String();
    string diffP1String();
    string tvTypeString();
    string switchesString();

  private:
    RiotState myState;
    RiotState myOldState;
};

#endif
