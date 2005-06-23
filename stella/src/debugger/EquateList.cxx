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
// $Id: EquateList.cxx,v 1.14 2005-06-23 01:10:25 urchlay Exp $
//============================================================================

#include <string>
#include <iostream>
#include <fstream>

#include "bspf.hxx"
#include "Equate.hxx"
#include "EquateList.hxx"
#include "Debugger.hxx"

// built in labels
static Equate hardCodedEquates[] = {
  { "VSYNC", 0x00 },
  { "VBLANK", 0x01 },
  { "WSYNC", 0x02 },
  { "RSYNC", 0x03    },
  { "NUSIZ0", 0x04 },
  { "NUSIZ1", 0x05 },
  { "COLUP0", 0x06 },
  { "COLUP1", 0x07 },
  { "COLUPF", 0x08 },
  { "COLUBK", 0x09 },
  { "CTRLPF", 0x0A },
  { "REFP0", 0x0B },
  { "REFP1", 0x0C },
  { "PF0", 0x0D },
  { "PF1", 0x0E },
  { "PF2", 0x0F },
  { "RESP0", 0x10 },
  { "RESP1", 0x11 },
  { "RESM0", 0x12 },
  { "RESM1", 0x13 },
  { "RESBL", 0x14 },
  { "AUDC0", 0x15 },
  { "AUDC1", 0x16 },
  { "AUDF0", 0x17 },
  { "AUDF1", 0x18 },
  { "AUDV0", 0x19 },
  { "AUDV1", 0x1A   },
  { "GRP0", 0x1B },
  { "GRP1", 0x1C },
  { "ENAM0", 0x1D },
  { "ENAM1", 0x1E },
  { "ENABL", 0x1F },
  { "HMP0", 0x20 },
  { "HMP1", 0x21 },
  { "HMM0", 0x22 },
  { "HMM1", 0x23 },
  { "HMBL", 0x24 },
  { "VDELP0", 0x25 },
  { "VDEL01", 0x26 },
  { "VDELP1", 0x26 },
  { "VDELBL", 0x27 },
  { "RESMP0", 0x28 },
  { "RESMP1", 0x29 },
  { "HMOVE", 0x2A },
  { "HMCLR", 0x2B },
  { "CXCLR", 0x2C },
  { "CXM0P", 0x30 },
  { "CXM1P", 0x31 },
  { "CXP0FB", 0x32 },
  { "CXP1FB", 0x33 },
  { "CXM0FB", 0x34 },
  { "CXM1FB", 0x35 },
  { "CXBLPF", 0x36 },
  { "CXPPMM", 0x37 },
  { "INPT0", 0x38 },
  { "INPT1", 0x39 },
  { "INPT2", 0x3A },
  { "INPT3", 0x3B },
  { "INPT4", 0x3C },
  { "INPT5", 0x3D },
  { "SWCHA", 0x0280 },
  { "SWCHB", 0x0282 },
  { "SWACNT", 0x281 },
  { "SWBCNT", 0x283 },
  { "INTIM", 0x0284 },
  { "TIM1T", 0x0294 },
  { "TIM8T", 0x0295 },
  { "TIM64T", 0x0296 },
  { "TIM1024T", 0x0297 }
};

EquateList::EquateList() {
	// cerr << sizeof(hardCodedEquates)/sizeof(struct Equate) << endl;
	int size = sizeof(hardCodedEquates)/sizeof(struct Equate);

	for(int i=0; i<size; i++)
		ourVcsEquates.push_back(hardCodedEquates[i]);
	calcSize();
}

EquateList::~EquateList() {
	ourVcsEquates.clear();
}

int EquateList::calcSize() {
	currentSize = ourVcsEquates.size();
	return currentSize;
}

// FIXME: use something smarter than a linear search in the future.
char *EquateList::getLabel(int addr) {
	// cerr << "getLabel(" << addr << ")" << endl;
	for(int i=0; i<currentSize; i++) {
		// cerr << "Checking ourVcsEquates[" << i << "] (" << ourVcsEquates[i].label << ")" << endl;
		if(ourVcsEquates[i].address == addr) {
			// cerr << "Found label " << ourVcsEquates[i].label << endl;
			return ourVcsEquates[i].label;
		}
	}

	return NULL;
}

// returns either the label, or a formatted hex string
// if no label found.
char *EquateList::getFormatted(int addr, int places) {
	static char fmt[10], buf[255];
	char *res = getLabel(addr);
	if(res != NULL)
		return res;

	sprintf(fmt, "$%%0%dx", places);
	//cerr << addr << ", " << fmt << ", " << places << endl;
	sprintf(buf, fmt, addr);
	return buf;
}

int EquateList::getAddress(const char *lbl) {
	// cerr << "getAddress(" << lbl << ")" << endl;
	// cerr << ourVcsEquates[0].label << endl;
	// cerr << "shit" << endl;
	for(int i=0; i<currentSize; i++) {
		// cerr << "Looking at " << ourVcsEquates[i].label << endl;
		if( STR_CASE_CMP(ourVcsEquates[i].label, lbl) == 0 )
			if(ourVcsEquates[i].address >= 0)
				return ourVcsEquates[i].address;
	}

	return -1;
}

bool EquateList::undefine(string& label) {
	return undefine(label.c_str());
}

bool EquateList::undefine(const char *lbl) {
	for(int i=0; i<currentSize; i++) {
		if( STR_CASE_CMP(ourVcsEquates[i].label, lbl) == 0 ) {
			ourVcsEquates[i].address = -1;
			return true;
		}
	}

	return false;
}

bool EquateList::saveFile(string file) {
	char buf[256];

	ofstream out(file.c_str());
	if(!out.is_open())
		return false;

	out << "--- Symbol List (sorted by symbol)" << endl;

	int hardSize = sizeof(hardCodedEquates)/sizeof(struct Equate);

	for(int i=hardSize; i<currentSize; i++) {
		int a = ourVcsEquates[i].address;
		if(a >= 0) {
			sprintf(buf, "%-24s %04x                  \n", ourVcsEquates[i].label, a);
			out << buf;
		}
	}

	out << "--- End of Symbol List." << endl;
	return true;
}

string EquateList::loadFile(string file) {
	int lines = 0;
	string curLabel;
	int curVal;
	// string::size_type p;
	char buffer[256]; // FIXME: static buffers suck

	// cerr << "loading file " << file << endl;

	ifstream in(file.c_str());
	if(!in.is_open())
		return "Unable to read symbols from " + file;

	int hardSize = sizeof(hardCodedEquates)/sizeof(struct Equate);

	// Make sure the hard-coded equates show up first
	ourVcsEquates.clear();
	for(int i=0; i<hardSize; i++) {
		ourVcsEquates.push_back(hardCodedEquates[i]);
	}

	while( !in.eof() ) {
		curVal = 0;
		curLabel = "";

		if(!in.getline(buffer, 255))
			break;

		if(buffer[0] != '-') {
			curLabel = extractLabel(buffer);
			if((curVal = extractValue(buffer)) < 0)
				return "invalid symbol file";

			addEquate(curLabel, curVal);

			// cerr << "label: " << curLabel << ", address: " << curVal << endl;
			// cerr << buffer;
			lines++;
		}
	}
	in.close();

	calcSize();

	// dumpAll();
	return "loaded " + file + " OK";
}

void EquateList::addEquate(string label, int address) {
	// FIXME - this is a memleak and *must* be fixed
	//         ideally, the Equate class should hold a string, not a char*
	Equate e;
	e.label   = strdup(label.c_str());
	e.address = address;
	ourVcsEquates.push_back(e);
	calcSize();
}

int EquateList::parse4hex(char *c) {
	//cerr << c << endl;
	int ret = 0;
	for(int i=0; i<4; i++) {
		if(*c >= '0' && *c <= '9')
			ret = (ret << 4) + (*c) - '0';
		else if(*c >= 'a' && *c <= 'f')
			ret = (ret << 4) + (*c) - 'a' + 10;
		else
			return -1;
		c++;
	}

	return ret;
}

int EquateList::extractValue(char *c) {
	while(*c != ' ') {
		if(*c == '\0')
			return -1;
		c++;
	}

	while(*c == ' ') {
		if(*c == '\0')
			return -1;
		c++;
	}

	return parse4hex(c);
}

string EquateList::extractLabel(char *c) {
	string l = "";
	while(*c != ' ')
		l += *c++;

	return l;
}

string EquateList::dumpAll() {
	string ret;

	for(int i=0; i<currentSize; i++) {
		if(ourVcsEquates[i].address != -1) {
			ret += ourVcsEquates[i].label;
			ret += ": ";
			ret += Debugger::to_hex_16(ourVcsEquates[i].address);
			if(i != currentSize - 1) ret += "\n";
		}
	}
	return ret;
}
