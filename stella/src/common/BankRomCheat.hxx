
#ifndef BANK_ROM_CHEAT_HXX
#define BANK_ROM_CHEAT_HXX

#include "OSystem.hxx"
#include "Cheat.hxx"

class BankRomCheat : public Cheat {
	public:
		BankRomCheat(OSystem *os, string code);
		~BankRomCheat();

		virtual bool enabled();
		virtual bool enable();
		virtual bool disable();


	private:
		bool _enabled;
		uInt8 savedRom[16];
		uInt16 address;
		uInt8 value;
		uInt8 count;
		int bank;
		OSystem *myOSystem;
};

#endif
