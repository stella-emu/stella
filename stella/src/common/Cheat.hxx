
#ifndef CHEAT_HXX
#define CHEAT_HXX

#include "bspf.hxx"
#include "OSystem.hxx"

class Cheat {
	public:
		static Cheat *parse(OSystem *osystem, string code);
		static uInt16 unhex(string hex);

		virtual ~Cheat();

		virtual bool enabled() = 0;
		virtual bool enable() = 0;
		virtual bool disable() = 0;

	protected:
		//	Cheat(string code);
};

#endif
