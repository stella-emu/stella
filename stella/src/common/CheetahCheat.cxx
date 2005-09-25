
// FIXME - don't use the debugger for this, since it may not be included
//#include "Debugger.hxx"
#include "CheetahCheat.hxx"

CheetahCheat::CheetahCheat(OSystem *os, string code) {
	myOSystem = os;
	_enabled = false;

	address = 0xf000 + unhex(code.substr(0, 3));
	value = unhex(code.substr(3, 2));
	count = unhex(code.substr(5, 1)) + 1;
}

CheetahCheat::~CheetahCheat() {
}

bool CheetahCheat::enabled() { return _enabled; }

bool CheetahCheat::enable() {
	for(int i=0; i<count; i++) {
		savedRom[i] = myOSystem->console().cartridge().peek(address + i);
		myOSystem->console().cartridge().patch(address + i, value);
	}
	return _enabled = true;
}

bool CheetahCheat::disable() {
	for(int i=0; i<count; i++) {
		myOSystem->console().cartridge().patch(address + i, savedRom[i]);
	}
	return _enabled = false;
}
