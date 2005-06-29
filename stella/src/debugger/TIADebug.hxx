
#ifndef TIADEBUG_HXX
#define TIADEBUG_HXX

#include "TIA.hxx"

class TIADebug {
	public:
		TIADebug(TIA *tia);
		~TIADebug();

		int scanlines();
		int frameCount();
		bool vsync();
		void updateTIA();

	private:
		TIA *myTIA;
};


#endif
