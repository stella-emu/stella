
#ifndef EQUATELIST_HXX
#define EQUATELIST_HXX

class EquateList {
	public:
		EquateList();
		char *getLabel(int addr);
		char *EquateList::getFormatted(int addr, int places);
		int getAddress(char *label);
};

#endif
