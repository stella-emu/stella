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
// $Id: TIADebug.cxx,v 1.5 2005-07-03 01:36:39 urchlay Exp $
//============================================================================

#include "TIADebug.hxx"
#include "System.hxx"
#include "Debugger.hxx"

TIADebug::TIADebug(TIA *tia) {
	myTIA = tia;

	nusizStrings[0] = "size=8, copies=1";
	nusizStrings[1] = "size=8, copies=2, spacing=8";
	nusizStrings[2] = "size=8, copies=2, spacing=18";
	nusizStrings[3] = "size=8, copies=3, spacing=8";
	nusizStrings[4] = "size=8, copies=2, spacing=38";
	nusizStrings[5] = "size=10, copies=1";
	nusizStrings[6] = "size=8, copies=3, spacing=18";
	nusizStrings[7] = "size=20, copies=1";
}

TIADebug::~TIADebug() {
}

void TIADebug::setDebugger(Debugger *d) {
	myDebugger = d;
}

int TIADebug::frameCount() {
	return myTIA->myFrameCounter;
}

int TIADebug::scanlines() {
	return myTIA->scanlines();
}

bool TIADebug::vsync() {
	return (myTIA->myVSYNC & 2) == 2;
}

bool TIADebug::vblank() {
	return (myTIA->myVBLANK & 2) == 2;
}

void TIADebug::updateTIA() {
	// not working the way I expected:
	// myTIA->updateFrame(myTIA->mySystem->cycles() * 3);
}

string TIADebug::colorSwatch(uInt8 c) {
	string ret;

	ret += char((c >> 1) | 0x80);
	ret += "\177     ";
	ret += "\177\003 ";

	return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string booleanWithLabel(string label, bool value) {
  if(value)
    return label + ":On";
  else
    return label + ":Off";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIADebug::state() {
	string ret;

	// TODO: prettier printing (table?)
	// TODO: joysticks, pots, switches
	// TODO: audio

	// TIA::myCOLUxx are stored 4 copies to each int, we need only 1
	uInt8 COLUP0 = myTIA->myCOLUP0 & 0xff;
	uInt8 COLUP1 = myTIA->myCOLUP1 & 0xff;
	uInt8 COLUPF = myTIA->myCOLUPF & 0xff;
	uInt8 COLUBK = myTIA->myCOLUBK & 0xff;

	// TIA::myPF holds all 3 PFx regs, we shall extract
	int PF = myTIA->myPF;
	uInt8 PF0 = PF & 0x0f;
	uInt8 PF1 = (PF >>  4) & 0xff;
	uInt8 PF2 = (PF >> 12) & 0xff;

	// Hope Brad never changes this:
	uInt16 coll = myTIA->myCollision;

	// calculate sizes
	uInt8 ballSize = 1 << (myTIA->myCTRLPF & 0x18);
	uInt8 m0Size = 1 <<   (myTIA->myNUSIZ0 & 0x18);
	uInt8 m1Size = 1 <<   (myTIA->myNUSIZ1 & 0x18);

	// easier to use a table for these:
	string p0Size = nusizStrings[myTIA->myNUSIZ0 & 0x07];
	string p1Size = nusizStrings[myTIA->myNUSIZ1 & 0x07];

	// build up output, then return it.
	ret += "scanline ";

	ret += myDebugger->valueToString(myTIA->scanlines());
	ret += "  ";

	ret += booleanWithLabel("VSYNC", vsync());
	ret += "  ";

	ret += booleanWithLabel("VBLANK", vblank());
	ret += "\n";

	ret += "COLUP0: ";
	ret += myDebugger->valueToString(COLUP0);
	ret += "/";
	ret += colorSwatch(COLUP0);

	ret += "COLUP1: ";
	ret += myDebugger->valueToString(COLUP1);
	ret += "/";
	ret += colorSwatch(COLUP1);

	ret += "COLUPF: ";
	ret += myDebugger->valueToString(COLUPF);
	ret += "/";
	ret += colorSwatch(COLUPF);

	ret += "COLUBK: ";
	ret += myDebugger->valueToString(COLUBK);
	ret += "/";
	ret += colorSwatch(COLUBK);

	ret += "\n";

	ret += "P0: data=";
   // TODO:    ret += myDebugger->invIfChanged(myTIA->myGRP0, oldGRP0);
	ret += Debugger::to_bin_8(myTIA->myGRP0);
	ret += "/";
	ret += Debugger::to_hex_8(myTIA->myGRP0);
	ret += ", ";
	ret += p0Size;
	ret += ", ";
	if(!myTIA->myREFP0) ret += "not ";
	ret += "reflected, pos=";
	ret += Debugger::to_hex_8(myTIA->myPOSP0);
	ret += ", ";
	if(!myTIA->myVDELP0) ret += "not ";
	ret += "delayed, HMP0=";
	ret += Debugger::to_hex_8(myTIA->myHMP0);
	ret += "\n";

	ret += "P1: data=";
	ret += Debugger::to_bin_8(myTIA->myGRP1);
	ret += "/";
	ret += Debugger::to_hex_8(myTIA->myGRP1);
	ret += ", ";
	ret += p1Size;
	ret += ", ";
	if(!myTIA->myREFP1) ret += "not ";
	ret += "reflected, pos=";
	ret += Debugger::to_hex_8(myTIA->myPOSP1);
	ret += ", ";
	if(!myTIA->myVDELP1) ret += "not ";
	ret += "delayed, HMP1=";
	ret += Debugger::to_hex_8(myTIA->myHMP1);
	ret += "\n";

	ret += "M0: ";
	ret += (myTIA->myENAM0 ? " enabled" : "disabled");
	ret += ", pos=";
	ret += Debugger::to_hex_8(myTIA->myPOSM0);
	ret += ", size=";
	ret += Debugger::to_hex_8(m0Size);
	ret += ", ";
	if(!myTIA->myRESMP0) ret += "not ";
	ret += "reset, HMM0=";
	ret += Debugger::to_hex_8(myTIA->myHMM0);
	ret += "\n";

	ret += "M1: ";
	ret += (myTIA->myENAM1 ? " enabled" : "disabled");
	ret += ", pos=";
	ret += Debugger::to_hex_8(myTIA->myPOSM1);
	ret += ", size=";
	ret += Debugger::to_hex_8(m1Size);
	ret += ", ";
	if(!myTIA->myRESMP1) ret += "not ";
	ret += "reset, HMM1=";
	ret += Debugger::to_hex_8(myTIA->myHMM1);
	ret += "\n";

	ret += "BL: ";
	ret += (myTIA->myENABL ? " enabled" : "disabled");
	ret += ", pos=";
	ret += Debugger::to_hex_8(myTIA->myPOSBL);
	ret += ", size=";
	ret += Debugger::to_hex_8(ballSize);
	ret += ", ";
	if(!myTIA->myVDELBL) ret += "not ";
	ret += "delayed, HMBL=";
	ret += Debugger::to_hex_8(myTIA->myHMBL);
	ret += "\n";

	ret += "PF0: ";
	ret += Debugger::to_bin_8(PF0);
	ret += "/";
	ret += Debugger::to_hex_8(PF0);
	ret += ", PF1: ";
	ret += Debugger::to_bin_8(PF1);
	ret += "/";
	ret += Debugger::to_hex_8(PF1);
	ret += ", PF2: ";
	ret += Debugger::to_bin_8(PF2);
	ret += "/";
	ret += Debugger::to_hex_8(PF2);
	ret += "\n  (pf ";
	if(! (myTIA->myCTRLPF & 0x01) ) ret += "not ";
	ret += "reflected, ";
	if(! (myTIA->myCTRLPF & 0x02) ) ret += "not ";
	ret += "score, ";
	if(! (myTIA->myCTRLPF & 0x04) ) ret += "not ";
	ret += "priority)";
	ret += "\n";

	ret += "Collisions: ";
	if(coll && 0x0001) ret += "M0-P1 ";
	if(coll && 0x0002) ret += "M0-P0 ";
	if(coll && 0x0004) ret += "M1-P0 ";
	if(coll && 0x0008) ret += "M1-P1 ";
	if(coll && 0x0010) ret += "P0-PF ";
	if(coll && 0x0020) ret += "P0-BL ";
	if(coll && 0x0040) ret += "P1-PF ";
	ret += "\n  ";
	if(coll && 0x0080) ret += "P1-BL ";
	if(coll && 0x0100) ret += "M0-PF ";
	if(coll && 0x0200) ret += "M0-BL ";
	if(coll && 0x0400) ret += "M1-PF ";
	if(coll && 0x0800) ret += "M1-BL ";
	if(coll && 0x1000) ret += "BL-PF ";
	if(coll && 0x2000) ret += "P0-P1 ";
	if(coll && 0x4000) ret += "M0-M1 ";

	// note: last "ret +=" line should not contain \n, caller will add.
	return ret;
}
