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
// $Id: EquateList.cxx,v 1.10 2005-06-21 04:30:49 urchlay Exp $
//============================================================================

#include <string>
#include <iostream>
#include <fstream>

#include "bspf.hxx"
#include "Equate.hxx"
#include "EquateList.hxx"

// built in labels
static struct Equate hardCodedEquates[] = {
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
  { "TIM1024T", 0x0297 },
  { NULL, 0 }
};

EquateList::EquateList() {
	// cerr << sizeof(hardCodedEquates)/sizeof(struct Equate) << endl;
	int size = sizeof(hardCodedEquates)/sizeof(struct Equate) + 1;
	ourVcsEquates = new Equate[ size ];
	// for(int i=0; hardCodedEquates[i].label != NULL; i++)
	for(int i=0; i<size; i++)
		ourVcsEquates[i] = hardCodedEquates[i];
	calcSize();
}

int EquateList::calcSize() {
	currentSize = 0;
	for(int i=0; ourVcsEquates[i].label != NULL; i++)
		currentSize++;

	return currentSize;
}

// FIXME: use something smarter than a linear search in the future.
char *EquateList::getLabel(int addr) {
	// cerr << "getLabel(" << addr << ")" << endl;
	for(int i=0; ourVcsEquates[i].label != NULL; i++) {
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
	for(int i=0; ourVcsEquates[i].label != NULL; i++) {
		// cerr << "Looking at " << ourVcsEquates[i].label << endl;
		if( STR_CASE_CMP(ourVcsEquates[i].label, lbl) == 0 )
			return ourVcsEquates[i].address;
	}

	return -1;
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

	long start = in.tellg(); // save pointer to beginning of file

	// iterate over file, count lines
	while( !in.eof() ) {
		in.getline(buffer, 255);
		lines++;
	}

	// cerr << "total lines " << lines << endl;

	// allocate enough storage for all the lines plus the
	// hard-coded symbols
	int hardSize = sizeof(hardCodedEquates)/sizeof(struct Equate) - 1;
	Equate *newEquates = new Equate[lines + hardSize + 1];
	lines = hardSize;

	// Make sure the hard-coded equates show up first
	for(int i=0; i<hardSize; i++) {
		newEquates[i] = hardCodedEquates[i];
	}

	// start over, now that we've allocated enough entries.
	in.clear();
	in.seekg(start);

	while( !in.eof() ) {
		curVal = 0;
		curLabel = "";

		if(!in.getline(buffer, 255))
			break;

		if(buffer[0] != '-') {
			curLabel = getLabel(buffer);
			if((curVal = parse4hex(buffer+25)) < 0)
				return "invalid symbol file";

			struct Equate *e = new struct Equate;

			// FIXME: this is cumbersome...
			// I shouldn't have to use sprintf() here.
			// also, is this a memory leak?  I miss malloc() and free()...
			newEquates[lines] = *e;
			newEquates[lines].label = new char[curLabel.length() + 1];
			sprintf(newEquates[lines].label, "%s", curLabel.c_str());
			newEquates[lines].address = curVal;

			// cerr << "label: " << curLabel << ", address: " << curVal << endl;
			// cerr << buffer;
			lines++;
		}
	}
	in.close();

	struct Equate *e = new struct Equate;
	newEquates[lines] = *e;
	newEquates[lines].label = NULL;
	newEquates[lines].address = 0;

	delete ourVcsEquates;
	ourVcsEquates = newEquates;
	calcSize();

	// dumpAll();
	return "loaded " + file + " OK";
}

int EquateList::parse4hex(char *c) {
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

string EquateList::getLabel(char *c) {
	string l = "";
	while(*c != ' ')
		l += *c++;

	return l;
}

void EquateList::dumpAll() {
	for(int i=0; ourVcsEquates[i].label != NULL; i++)
		cerr << i << ": " << "label==" << ourVcsEquates[i].label << ", address==" << ourVcsEquates[i].address << endl;
}
