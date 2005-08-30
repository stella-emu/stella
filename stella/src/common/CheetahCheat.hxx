
#ifndef CHEETAH_CHEAT_HXX
#define CHEETAH_CHEAT_HXX

#include "Cheat.hxx"

class CheetahCheat : public Cheat {
	public:
		CheetahCheat(string code);
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
};

#endif
