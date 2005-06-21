
#ifndef TIADEBUG_HXX
#define TIADEBUG_HXX

#include "TIA.hxx"

class TIADebug {
	public:
		TIADebug(TIA *tia);
		~TIADebug();

		int scanlines();
		bool vsync();

	private:
		TIA *myTIA;
};


#endif
