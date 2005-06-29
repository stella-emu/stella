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
// $Id: TIADebug.hxx,v 1.4 2005-06-29 13:11:03 stephena Exp $
//============================================================================

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
		string spriteState();

	private:
		TIA *myTIA;
};


#endif
