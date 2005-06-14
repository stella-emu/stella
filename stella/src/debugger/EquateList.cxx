
#include "bspf.hxx"
#include "Equate.hxx"
#include "EquateList.hxx"

// built in labels
static struct Equate ourVcsEquates[] = {
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
}

// FIXME: use something smarter than a linear search in the future.
char *EquateList::getLabel(int addr) {
	for(int i=0; ourVcsEquates[i].label != NULL; i++)
		if(ourVcsEquates[i].address == addr)
			return ourVcsEquates[i].label;

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
	sprintf(buf, fmt, addr);
	return buf;
}

int EquateList::getAddress(char *label) {
	for(int i=0; ourVcsEquates[i].label != NULL; i++)
		if( strcmp(ourVcsEquates[i].label, label) == 0 )
			return ourVcsEquates[i].address;

	return -1;
}
