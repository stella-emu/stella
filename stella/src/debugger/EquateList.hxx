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
// $Id: EquateList.hxx,v 1.9 2005-06-24 00:03:39 urchlay Exp $
//============================================================================

#ifndef EQUATELIST_HXX
#define EQUATELIST_HXX

#include "bspf.hxx"
#include "Equate.hxx"
#include "Array.hxx"

typedef GUI::Array<Equate> Equates;

class EquateList {
	public:
		EquateList();
		~EquateList();
		char *getLabel(int addr);
		char *getFormatted(int addr, int places);
		int getAddress(const char *label);
		void addEquate(string label, int address);
		bool saveFile(string file);
		string loadFile(string file);
		bool undefine(string& label);
		bool undefine(const char *lbl);
		string dumpAll();
		int countCompletions(const char *in);
		const char *getCompletions();

	private:
		int calcSize();
		int parse4hex(char *c);
		string extractLabel(char *c);
		int extractValue(char *c);
      string completions;

	private:
		Equates ourVcsEquates;
		int currentSize;
};

#endif
