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
// $Id: EquateList.hxx,v 1.5 2005-06-16 00:20:11 stephena Exp $
//============================================================================

#ifndef EQUATELIST_HXX
#define EQUATELIST_HXX

#include <string>

class EquateList {
	public:
		EquateList();
		char *getLabel(int addr);
		char *EquateList::getFormatted(int addr, int places);
		int getAddress(const char *label);
		string loadFile(string file);
		void dumpAll();

	private:
		int calcSize();
		int parse4hex(char *c);
		string EquateList::getLabel(char *c);

		struct Equate *ourVcsEquates;
		int currentSize;
};

#endif
