
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
