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
// $Id: PackedBitArray.cxx,v 1.4 2005-06-29 13:11:03 stephena Exp $
//============================================================================

#include "bspf.hxx"
#include "PackedBitArray.hxx"

PackedBitArray::PackedBitArray(int length) {
	size = length;
	words = length / wordSize + 1;
	bits = new unsigned int[ words ];

	// FIXME: find out if this is necessary (does a new array
	// start out zeroed already?
	for(int i=0; i<words; i++)
		bits[i] = 0;
}

PackedBitArray::~PackedBitArray() {
	delete[] bits;
}

int PackedBitArray::isSet(unsigned int bit) {
	unsigned int word = bit / wordSize;
	bit %= wordSize;

	return (bits[word] & (1 << bit));
}

int PackedBitArray::isClear(unsigned int bit) {
	unsigned int word = bit / wordSize;
	bit %= wordSize;

	return !(bits[word] & (1 << bit));
}

void PackedBitArray::toggle(unsigned int bit) {
	unsigned int word = bit / wordSize;
	bit %= wordSize;

	bits[word] ^= (1 << bit);
}

void PackedBitArray::set(unsigned int bit) {
	unsigned int word = bit / wordSize;
	bit %= wordSize;

	bits[word] |= (1 << bit);
}

void PackedBitArray::clear(unsigned int bit) {
	unsigned int word = bit / wordSize;
	bit %= wordSize;

	bits[word] &= (~(1 << bit));
}
