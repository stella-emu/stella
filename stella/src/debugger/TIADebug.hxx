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
// $Id: TIADebug.hxx,v 1.25 2009-01-12 15:11:54 stephena Exp $
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

// Indices for various IntArray in TiaState
enum {
  P0, P1, M0, M1, BL
};

class TiaState : public DebuggerState
{
  public:
    IntArray ram;
    IntArray coluRegs;
    IntArray gr;
    IntArray pos;
    IntArray hm;
    IntArray pf;
    IntArray size;
    IntArray aud;
};

class TIADebug : public DebuggerSystem
{
  public:
    TIADebug(Debugger& dbg, Console& console);

    const DebuggerState& getState();
    const DebuggerState& getOldState() { return myOldState; }

    void saveOldState();
    string toString();

	 /* TIA byte (or part of a byte) registers */
    uInt8 nusiz0(int newVal = -1);
    uInt8 nusiz1(int newVal = -1);
    uInt8 nusizP0(int newVal = -1);
    uInt8 nusizP1(int newVal = -1);
    uInt8 nusizM0(int newVal = -1);
    uInt8 nusizM1(int newVal = -1);
    const string& nusizP0String() { return nusizStrings[nusizP0()]; }
    const string& nusizP1String() { return nusizStrings[nusizP1()]; }

    uInt8 coluP0(int newVal = -1);
    uInt8 coluP1(int newVal = -1);
    uInt8 coluPF(int newVal = -1);
    uInt8 coluBK(int newVal = -1);

    uInt8 sizeBL(int newVal = -1);
    uInt8 ctrlPF(int newVal = -1);

    uInt8 pf0(int newVal = -1);
    uInt8 pf1(int newVal = -1);
    uInt8 pf2(int newVal = -1);

    uInt8 grP0(int newVal = -1);
    uInt8 grP1(int newVal = -1);
    uInt8 posP0(int newVal = -1);
    uInt8 posP1(int newVal = -1);
    uInt8 posM0(int newVal = -1);
    uInt8 posM1(int newVal = -1);
    uInt8 posBL(int newVal = -1);
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

    bool refPF(int newVal = -1);
    bool scorePF(int newVal = -1);
    bool priorityPF(int newVal = -1);

    /* Collision registers */
    bool collM0_P1(int newVal = -1) { return collision(0,  newVal); }
    bool collM0_P0(int newVal = -1) { return collision(1,  newVal); }
    bool collM1_P0(int newVal = -1) { return collision(2,  newVal); }
    bool collM1_P1(int newVal = -1) { return collision(3,  newVal); }
    bool collP0_PF(int newVal = -1) { return collision(4,  newVal); }
    bool collP0_BL(int newVal = -1) { return collision(5,  newVal); }
    bool collP1_PF(int newVal = -1) { return collision(6,  newVal); }
    bool collP1_BL(int newVal = -1) { return collision(7,  newVal); }
    bool collM0_PF(int newVal = -1) { return collision(8,  newVal); }
    bool collM0_BL(int newVal = -1) { return collision(9,  newVal); }
    bool collM1_PF(int newVal = -1) { return collision(10, newVal); }
    bool collM1_BL(int newVal = -1) { return collision(11, newVal); }
    bool collBL_PF(int newVal = -1) { return collision(12, newVal); }
    bool collP0_P1(int newVal = -1) { return collision(13, newVal); }
    bool collM0_M1(int newVal = -1) { return collision(14, newVal); }

    /* TIA strobe registers */
    void strobeWsync() { mySystem.poke(WSYNC, 0); }
    void strobeRsync() { mySystem.poke(RSYNC, 0); } // not emulated!
    void strobeResP0() { mySystem.poke(RESP0, 0); }
    void strobeResP1() { mySystem.poke(RESP1, 0); }
    void strobeResM0() { mySystem.poke(RESM0, 0); }
    void strobeResM1() { mySystem.poke(RESM1, 0); }
    void strobeResBL() { mySystem.poke(RESBL, 0); }
    void strobeHmove() { mySystem.poke(HMOVE, 0); }
    void strobeHmclr() { mySystem.poke(HMCLR, 0); }
    void strobeCxclr() { mySystem.poke(CXCLR, 0); }

	 /* read-only internal TIA state */
    int scanlines();
    int frameCount();
    int clocksThisLine();
    bool vsync();
    bool vblank();
    int vsyncAsInt()  { return int(vsync());  } // so we can use _vsync pseudo-register
    int vblankAsInt() { return int(vblank()); } // so we can use _vblank pseudo-register

  private:
    /** Display a color patch for color at given index in the palette */
    string colorSwatch(uInt8 c);

    /** Get/set specific bits in the collision register (used by collXX_XX) */
    bool collision(int collID, int newVal);

    string booleanWithLabel(string label, bool value);

  private:
    TiaState myState;
    TiaState myOldState;

    TIA& myTIA;

    string nusizStrings[8];
};

#endif
