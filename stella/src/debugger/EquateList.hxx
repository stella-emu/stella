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
// Copyright (c) 1995-2006 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: EquateList.hxx,v 1.14 2006-12-08 16:49:00 stephena Exp $
//============================================================================

#ifndef EQUATELIST_HXX
#define EQUATELIST_HXX

#include <map>

#include "bspf.hxx"
#include "Equate.hxx"
#include "Array.hxx"

using namespace std;

typedef map<int, Equate> addrToLabel;
typedef map<string, Equate> labelToAddr;

typedef Common::Array<Equate> Equates;

class EquateList {
	public:
		EquateList();
		~EquateList();
		const string& getLabel(int addr);
		const string& getLabel(int addr, int flags);
		const char *getFormatted(int addr, int places);
		const char *getFormatted(int addr, int places, const int flags);
		int getAddress(const string& label);
		int getAddress(const string& label, const int flags);
		void addEquate(string label, int address);
		// void addEquate(string label, int address, const int flags);
		bool saveFile(string file);
		string loadFile(string file);
		bool undefine(string& label);
		bool undefine(const char *lbl);
		//string dumpAll();
		int countCompletions(const char *in);
		const char *getCompletions();
		const char *getCompletionPrefix();

	private:
		int calcSize();
		int parse4hex(char *c);
		string extractLabel(char *c);
		int extractValue(char *c);
      string completions;
      string compPrefix;

	private:
		//Equates ourVcsEquates;
		int currentSize;
		labelToAddr myFwdMap;
		addrToLabel myRevMap;
};

#endif
