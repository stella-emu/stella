
#include "bspf.hxx"
#include "PackedBitArray.hxx"

/*

		boolean set();
		boolean clear();

		void set(int bit);
		void clear(int bit);
		void toggle(int bit);
*/

PackedBitArray::PackedBitArray(int length) {
	size = length;
	words = length / wordSize + 1;
	bits = new unsigned int[ words ];

	// FIXME: find out if this is necessary (does a new array
	// start out zeroed already?
	for(int i=0; i<words; i++)
		bits[i] = 0;

	// cerr << "size==" << size << " words==" << words << endl;
}

PackedBitArray::~PackedBitArray() {
	delete bits;
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

	// cerr << "word==" << word << ", bit==" << bit << endl;

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
