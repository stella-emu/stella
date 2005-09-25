
#ifndef CHEETAH_CHEAT_HXX
#define CHEETAH_CHEAT_HXX

#include "OSystem.hxx"
#include "Cheat.hxx"

class CheetahCheat : public Cheat {
	public:
		CheetahCheat(OSystem *os, string code);
		~CheetahCheat();

		virtual bool enabled();
		virtual bool enable();
		virtual bool disable();


	private:
		bool _enabled;
		uInt8 savedRom[16];
		uInt16 address;
		uInt8 value;
		uInt8 count;
		OSystem *myOSystem;
};

#endif
