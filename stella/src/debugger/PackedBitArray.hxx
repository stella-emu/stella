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
// $Id: PackedBitArray.hxx,v 1.3 2006-12-08 16:49:04 stephena Exp $
//============================================================================

#ifndef PACKEDBITARRAY_HXX
#define PACKEDBITARRAY_HXX

#include "bspf.hxx"

#define wordSize ( (sizeof(unsigned int)) * 8)

class PackedBitArray {
	public:
		PackedBitArray(int length);
		~PackedBitArray();

		int isSet(unsigned int bit);
		int isClear(unsigned int bit);

		void set(unsigned int bit);
		void clear(unsigned int bit);
		void toggle(unsigned int bit);

	private:
		// number of bits in the array:
		int size;

		// number of unsigned ints (size/wordSize):
		int words;

		// the array itself:
		unsigned int *bits;
};

#endif
