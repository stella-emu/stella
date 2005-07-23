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
// $Id: TIADebug.hxx,v 1.14 2005-07-23 15:55:21 urchlay Exp $
//============================================================================

#ifndef TIA_DEBUG_HXX
#define TIA_DEBUG_HXX

class TIA;
class Debugger;
class TiaDebug;

#include "Array.hxx"
#include "DebuggerSystem.hxx"

// pointer types for TIADebug instance methods
// (used by TiaMethodExpression)
typedef int (TIADebug::*TIADEBUG_INT_METHOD)();

// call the pointed-to method on the (global) debugger object.
#define CALL_TIADEBUG_METHOD(method) ( ( Debugger::debugger().tiaDebug().*method)() )

enum TIALabel {
	VSYNC = 0,
	VBLANK,
	WSYNC,
	RSYNC,
	NUSIZ0,
	NUSIZ1,
	COLUP0,
	COLUP1,
	COLUPF, // $08
	COLUBK,
	CTRLPF,
	REFP0,
	REFP1,
	PF0,
	PF1,
	PF2,
	RESP0,  // $10
	RESP1,
	RESM0,
	RESM1,
	RESBL,
	AUDC0,
	AUDC1,
	AUDF0,
	AUDF1,  // $18
	AUDV0,
	AUDV1,
	GRP0,
	GRP1,
	ENAM0,
	ENAM1,
	ENABL,
	HMP0,   // $20
	HMP1,
	HMM0,
	HMM1,
	HMBL,
	VDELP0,
	VDELP1,
	VDELBL,
	RESMP0, // $28
	RESMP1,
	HMOVE,
	HMCLR,
	CXCLR   // $2C
};

class TiaState : public DebuggerState
{
  public:
    IntArray ram;
    IntArray coluRegs;
};

class TIADebug : public DebuggerSystem
{
  public:
    TIADebug(Debugger* dbg, Console* console);

    DebuggerState& getState();
    DebuggerState& getOldState() { return myOldState; }

    void saveOldState();

	 /* TIA byte (or part of a byte) registers */
    uInt8 nusiz0(int newVal = -1);
    uInt8 nusiz1(int newVal = -1);

    uInt8 coluP0(int newVal = -1);
    uInt8 coluP1(int newVal = -1);
    uInt8 coluPF(int newVal = -1);
    uInt8 coluBK(int newVal = -1);

    uInt8 ctrlPF(int newVal = -1);

    uInt8 pf0(int newVal = -1);
    uInt8 pf1(int newVal = -1);
    uInt8 pf2(int newVal = -1);

    uInt8 grP0(int newVal = -1);
    uInt8 grP1(int newVal = -1);
    uInt8 hmP0(int newVal = -1);
    uInt8 hmP1(int newVal = -1);
    uInt8 hmM0(int newVal = -1);
    uInt8 hmM1(int newVal = -1);
    uInt8 hmBL(int newVal = -1);

    uInt8 audC0(int newVal = -1);
    uInt8 audC1(int newVal = -1);
    uInt8 audF0(int newVal = -1);
    uInt8 audF1(int newVal = -1);
    uInt8 audV0(int newVal = -1);
    uInt8 audV1(int newVal = -1);

	 /* TIA bool registers */
    bool refP0(int newVal = -1);
    bool refP1(int newVal = -1);
    bool enaM0(int newVal = -1);
    bool enaM1(int newVal = -1);
    bool enaBL(int newVal = -1);

	 bool vdelP0(int newVal = -1);
	 bool vdelP1(int newVal = -1);
	 bool vdelBL(int newVal = -1);

	 bool resMP0(int newVal = -1);
	 bool resMP1(int newVal = -1);

	 /* TIA strobe registers */
	 void strobeWsync() { mySystem->poke(WSYNC, 0); }
	 void strobeRsync() { mySystem->poke(RSYNC, 0); } // not emulated!
	 void strobeResP0() { mySystem->poke(RESP0, 0); }
	 void strobeResP1() { mySystem->poke(RESP1, 0); }
	 void strobeResM0() { mySystem->poke(RESM0, 0); }
	 void strobeResM1() { mySystem->poke(RESM1, 0); }
	 void strobeResBL() { mySystem->poke(RESBL, 0); }
	 void strobeHmove() { mySystem->poke(HMOVE, 0); }
	 void strobeHmclr() { mySystem->poke(HMCLR, 0); }
	 void strobeCxclr() { mySystem->poke(CXCLR, 0); }

	 /* read-only internal TIA state */
    int scanlines();
    int frameCount();
    int clocksThisLine();
    bool vsync();
    bool vblank();
    string state();

  private:
    string colorSwatch(uInt8 c);

  private:
    TiaState myState;
    TiaState myOldState;

    System* mySystem;
    TIA* myTIA;

    string nusizStrings[8];
};

#endif
