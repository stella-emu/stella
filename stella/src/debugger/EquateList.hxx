
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

	private:
		int calcSize();
		int parse4hex(char *c);
		string EquateList::getLabel(char *c);
		void dumpAll();

		struct Equate *ourVcsEquates;
		int currentSize;
};

#endif
