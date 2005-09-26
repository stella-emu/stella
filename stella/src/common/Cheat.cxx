
#include "Cheat.hxx"
#include "CheetahCheat.hxx"
#include "BankRomCheat.hxx"

uInt16 Cheat::unhex(string hex) {
	int ret = 0;

	for(unsigned int i=0; i<hex.size(); i++) {
		char c = hex[i];

		ret *= 16;
		if(c >= '0' && c <= '9')
			ret += c - '0';
		else if(c >= 'A' && c <= 'F')
			ret += c - 'A' + 10;
		else
			ret += c - 'a' + 10;
	}

	return ret;
}

Cheat* Cheat::parse(OSystem *osystem, string code) {
	for(unsigned int i=0; i<code.size(); i++)
		if(!isxdigit(code[i]))
			return 0;

	switch(code.size()) {
		case 7:
		case 8:
			return new BankRomCheat(osystem, code);

		case 6:
			return new CheetahCheat(osystem, code);

		default:
			return 0;
	}
}

Cheat::~Cheat() {
}
