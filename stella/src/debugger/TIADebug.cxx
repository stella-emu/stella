
#include "TIADebug.hxx"
#include "System.hxx"

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
