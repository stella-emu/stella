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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
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
    uInt8 SWCHA_R, SWCHA_W, SWACNT, SWCHB;
    BoolArray swchaReadBits;
    BoolArray swchaWriteBits;
    BoolArray swacntBits;
    BoolArray swchbBits;

    uInt8 TIM1T, TIM8T, TIM64T, TIM1024T, INTIM, TIMINT;
    Int32 TIMCLKS;

    bool P0_PIN1, P0_PIN2, P0_PIN3, P0_PIN4, P0_PIN6;
    bool P1_PIN1, P1_PIN2, P1_PIN3, P1_PIN4, P1_PIN6;
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
    uInt8 swacnt(int newVal = -1);
    uInt8 swchb(int newVal = -1);

    /* Timer registers & associated clock */
    uInt8 tim1T(int newVal = -1);
    uInt8 tim8T(int newVal = -1);
    uInt8 tim64T(int newVal = -1);
    uInt8 tim1024T(int newVal = -1);
    uInt8 intim();
    uInt8 timint();
    Int32 timClocks();

    /* Controller pins, from the POV of 'outside' the system
       (ie, state is determined by what the controller sends to the RIOT)
       Setting a pin to false is the same as if the external controller
       pulled the pin low
    */
    void setP0Pins(bool Pin1, bool Pin2, bool Pin3, bool Pin4, bool Pin6);
    void setP1Pins(bool Pin1, bool Pin2, bool Pin3, bool Pin4, bool Pin6);

    /* Console switches */
    bool diffP0(int newVal = -1);
    bool diffP1(int newVal = -1);
    bool tvType(int newVal = -1);
    bool select(int newVal = -1);
    bool reset(int newVal = -1);

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
