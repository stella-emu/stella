
#include "TIADebug.hxx"
#include "System.hxx"
#include "Debugger.hxx"

TIADebug::TIADebug(TIA *tia) {
	myTIA = tia;
}

TIADebug::~TIADebug() {
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

void TIADebug::updateTIA() {
	// not working the way I expected:
	// myTIA->updateFrame(myTIA->mySystem->cycles() * 3);
}

string TIADebug::spriteState() {
	string ret;

	// TODO: prettier printing (table?)
	// TODO: NUSIZ
	// TODO: missiles, ball, PF
	// TODO: collisions
	// TODO: audio

	ret += "P0: data=";
	ret += Debugger::to_bin_8(myTIA->myGRP0);
	ret += "/";
	ret += Debugger::to_hex_8(myTIA->myGRP0);
	ret += ", ";
	if(!myTIA->myREFP0) ret += "not ";
	ret += "reflected, pos=";
	ret += Debugger::to_hex_8(myTIA->myPOSP0);
	ret += ", color=";
	ret += Debugger::to_hex_8(myTIA->myCOLUP0 & 0xff);
	ret += ", ";
	if(!myTIA->myVDELP0) ret += "not ";
	ret += " delayed";
	ret += "\n";

	ret += "P1: data=";
	ret += Debugger::to_bin_8(myTIA->myGRP1);
	ret += "/";
	ret += Debugger::to_hex_8(myTIA->myGRP1);
	ret += ", ";
	if(!myTIA->myREFP1) ret += "not ";
	ret += "reflected, pos=";
	ret += Debugger::to_hex_8(myTIA->myPOSP1);
	ret += ", color=";
	ret += Debugger::to_hex_8(myTIA->myCOLUP1 & 0xff);
	ret += ", ";
	if(!myTIA->myVDELP1) ret += "not ";
	ret += "delayed";
	ret += "\n";

	return ret;
}
