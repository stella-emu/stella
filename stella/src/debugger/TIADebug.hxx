//============================================================================
//
//   SSSS    tt          lll  lll       
//  SS  SS   tt           ll   ll        
//  SS     tttttt  eeee   ll   ll   aaaa 
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: TIADebug.hxx,v 1.5 2005-07-03 01:36:39 urchlay Exp $
//============================================================================

#ifndef TIADEBUG_HXX
#define TIADEBUG_HXX

#include "TIA.hxx"
#include "Debugger.hxx"

class TIADebug {
	public:
		TIADebug(TIA *tia);
		~TIADebug();

		void setDebugger(Debugger *d);

		int scanlines();
		int frameCount();
		bool vsync();
		bool vblank();
		void updateTIA();
		string state();

	private:
		TIA *myTIA;
		Debugger *myDebugger;

		string colorSwatch(uInt8 c);

		string nusizStrings[8];
};


#endif
