
#include "TIADebug.hxx"

TIADebug::TIADebug(TIA *tia) {
	myTIA = tia;
}

TIADebug::~TIADebug() {
}

int TIADebug::scanlines() {
	return myTIA->scanlines();
}

bool TIADebug::vsync() {
	return (myTIA->myVSYNC & 2) == 2;
}
