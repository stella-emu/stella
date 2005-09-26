
// FIXME - don't use the debugger for this, since it may not be included
//#include "Debugger.hxx"
#include "BankRomCheat.hxx"

BankRomCheat::BankRomCheat(OSystem *os, string code) {
	myOSystem = os;
	_enabled = false;

	if(code.length() == 7)
		code = "0" + code;

	bank = unhex(code.substr(0, 2));
	address = 0xf000 + unhex(code.substr(2, 3));
	value = unhex(code.substr(5, 2));
	count = unhex(code.substr(7, 1)) + 1;
}

BankRomCheat::~BankRomCheat() {
}

bool BankRomCheat::enabled() { return _enabled; }

bool BankRomCheat::enable() {
	int oldBank = myOSystem->console().cartridge().bank();
	myOSystem->console().cartridge().bank(bank);
	for(int i=0; i<count; i++) {
		savedRom[i] = myOSystem->console().cartridge().peek(address + i);
		myOSystem->console().cartridge().patch(address + i, value);
	}
	myOSystem->console().cartridge().bank(oldBank);
	return _enabled = true;
}

bool BankRomCheat::disable() {
	int oldBank = myOSystem->console().cartridge().bank();
	myOSystem->console().cartridge().bank(bank);
	for(int i=0; i<count; i++) {
		myOSystem->console().cartridge().patch(address + i, savedRom[i]);
	}
	myOSystem->console().cartridge().bank(oldBank);
	return _enabled = false;
}
