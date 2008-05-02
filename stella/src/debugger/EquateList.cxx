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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: EquateList.cxx,v 1.30 2008-05-02 01:19:48 stephena Exp $
//============================================================================

#include <fstream>

#include "bspf.hxx"
#include "Equate.hxx"
#include "EquateList.hxx"
#include "Debugger.hxx"

// built in labels
static Equate hardCodedEquates[] = {

// Standard $00-based TIA write locations:
  { "VSYNC",  0x00, EQF_WRITE },
  { "VBLANK", 0x01, EQF_WRITE },
  { "WSYNC",  0x02, EQF_WRITE },
  { "RSYNC",  0x03, EQF_WRITE },
  { "NUSIZ0", 0x04, EQF_WRITE },
  { "NUSIZ1", 0x05, EQF_WRITE },
  { "COLUP0", 0x06, EQF_WRITE },
  { "COLUP1", 0x07, EQF_WRITE },
  { "COLUPF", 0x08, EQF_WRITE },
  { "COLUBK", 0x09, EQF_WRITE },
  { "CTRLPF", 0x0A, EQF_WRITE },
  { "REFP0",  0x0B, EQF_WRITE },
  { "REFP1",  0x0C, EQF_WRITE },
  { "PF0",    0x0D, EQF_WRITE },
  { "PF1",    0x0E, EQF_WRITE },
  { "PF2",    0x0F, EQF_WRITE },
  { "RESP0",  0x10, EQF_WRITE },
  { "RESP1",  0x11, EQF_WRITE },
  { "RESM0",  0x12, EQF_WRITE },
  { "RESM1",  0x13, EQF_WRITE },
  { "RESBL",  0x14, EQF_WRITE },
  { "AUDC0",  0x15, EQF_WRITE },
  { "AUDC1",  0x16, EQF_WRITE },
  { "AUDF0",  0x17, EQF_WRITE },
  { "AUDF1",  0x18, EQF_WRITE },
  { "AUDV0",  0x19, EQF_WRITE },
  { "AUDV1",  0x1A, EQF_WRITE },
  { "GRP0",   0x1B, EQF_WRITE },
  { "GRP1",   0x1C, EQF_WRITE },
  { "ENAM0",  0x1D, EQF_WRITE },
  { "ENAM1",  0x1E, EQF_WRITE },
  { "ENABL",  0x1F, EQF_WRITE },
  { "HMP0",   0x20, EQF_WRITE },
  { "HMP1",   0x21, EQF_WRITE },
  { "HMM0",   0x22, EQF_WRITE },
  { "HMM1",   0x23, EQF_WRITE },
  { "HMBL",   0x24, EQF_WRITE },
  { "VDELP0", 0x25, EQF_WRITE },
  { "VDEL01", 0x26, EQF_WRITE },
  { "VDELP1", 0x26, EQF_WRITE },
  { "VDELBL", 0x27, EQF_WRITE },
  { "RESMP0", 0x28, EQF_WRITE },
  { "RESMP1", 0x29, EQF_WRITE },
  { "HMOVE",  0x2A, EQF_WRITE },
  { "HMCLR",  0x2B, EQF_WRITE },
  { "CXCLR",  0x2C, EQF_WRITE },

// Mirrored $30-based TIA write locations:
  { "VSYNC.30",  0x30, EQF_WRITE },
  { "VBLANK.30", 0x31, EQF_WRITE },
  { "WSYNC.30",  0x32, EQF_WRITE },
  { "RSYNC.30",  0x33, EQF_WRITE },
  { "NUSIZ0.30", 0x34, EQF_WRITE },
  { "NUSIZ1.30", 0x35, EQF_WRITE },
  { "COLUP0.30", 0x36, EQF_WRITE },
  { "COLUP1.30", 0x37, EQF_WRITE },
  { "COLUPF.30", 0x38, EQF_WRITE },
  { "COLUBK.30", 0x39, EQF_WRITE },
  { "CTRLPF.30", 0x3A, EQF_WRITE },
  { "REFP0.30",  0x3B, EQF_WRITE },
  { "REFP1.30",  0x3C, EQF_WRITE },
  { "PF0.30",    0x3D, EQF_WRITE },
  { "PF1.30",    0x3E, EQF_WRITE },
  { "PF2.30",    0x3F, EQF_WRITE },
  { "RESP0.30",  0x40, EQF_WRITE },
  { "RESP1.30",  0x41, EQF_WRITE },
  { "RESM0.30",  0x42, EQF_WRITE },
  { "RESM1.30",  0x43, EQF_WRITE },
  { "RESBL.30",  0x44, EQF_WRITE },
  { "AUDC0.30",  0x45, EQF_WRITE },
  { "AUDC1.30",  0x46, EQF_WRITE },
  { "AUDF0.30",  0x47, EQF_WRITE },
  { "AUDF1.30",  0x48, EQF_WRITE },
  { "AUDV0.30",  0x49, EQF_WRITE },
  { "AUDV1.30",  0x4A, EQF_WRITE },
  { "GRP0.30",   0x4B, EQF_WRITE },
  { "GRP1.30",   0x4C, EQF_WRITE },
  { "ENAM0.30",  0x4D, EQF_WRITE },
  { "ENAM1.30",  0x4E, EQF_WRITE },
  { "ENABL.30",  0x4F, EQF_WRITE },
  { "HMP0.30",   0x50, EQF_WRITE },
  { "HMP1.30",   0x51, EQF_WRITE },
  { "HMM0.30",   0x52, EQF_WRITE },
  { "HMM1.30",   0x53, EQF_WRITE },
  { "HMBL.30",   0x54, EQF_WRITE },
  { "VDELP0.30", 0x55, EQF_WRITE },
  { "VDEL01.30", 0x56, EQF_WRITE },
  { "VDELP1.30", 0x56, EQF_WRITE },
  { "VDELBL.30", 0x57, EQF_WRITE },
  { "RESMP0.30", 0x58, EQF_WRITE },
  { "RESMP1.30", 0x59, EQF_WRITE },
  { "HMOVE.30",  0x5A, EQF_WRITE },
  { "HMCLR.30",  0x5B, EQF_WRITE },
  { "CXCLR.30",  0x5C, EQF_WRITE },

// Standard $00-based TIA read locations:
  { "CXM0P",  0x00, EQF_READ },
  { "CXM1P",  0x01, EQF_READ },
  { "CXP0FB", 0x02, EQF_READ },
  { "CXP1FB", 0x03, EQF_READ },
  { "CXM0FB", 0x04, EQF_READ },
  { "CXM1FB", 0x05, EQF_READ },
  { "CXBLPF", 0x06, EQF_READ },
  { "CXPPMM", 0x07, EQF_READ },
  { "INPT0",  0x08, EQF_READ },
  { "INPT1",  0x09, EQF_READ },
  { "INPT2",  0x0A, EQF_READ },
  { "INPT3",  0x0B, EQF_READ },
  { "INPT4",  0x0C, EQF_READ },
  { "INPT5",  0x0D, EQF_READ },

// Mirrored $10-based TIA read locations:
  { "CXM0P.10",  0x10, EQF_READ },
  { "CXM1P.10",  0x11, EQF_READ },
  { "CXP0FB.10", 0x12, EQF_READ },
  { "CXP1FB.10", 0x13, EQF_READ },
  { "CXM0FB.10", 0x14, EQF_READ },
  { "CXM1FB.10", 0x15, EQF_READ },
  { "CXBLPF.10", 0x16, EQF_READ },
  { "CXPPMM.10", 0x17, EQF_READ },
  { "INPT0.10",  0x18, EQF_READ },
  { "INPT1.10",  0x19, EQF_READ },
  { "INPT2.10",  0x1A, EQF_READ },
  { "INPT3.10",  0x1B, EQF_READ },
  { "INPT4.10",  0x1C, EQF_READ },
  { "INPT5.10",  0x1D, EQF_READ },

// Mirrored $20-based TIA read locations:
  { "CXM0P.20",  0x20, EQF_READ },
  { "CXM1P.20",  0x21, EQF_READ },
  { "CXP0FB.20", 0x22, EQF_READ },
  { "CXP1FB.20", 0x23, EQF_READ },
  { "CXM0FB.20", 0x24, EQF_READ },
  { "CXM1FB.20", 0x25, EQF_READ },
  { "CXBLPF.20", 0x26, EQF_READ },
  { "CXPPMM.20", 0x27, EQF_READ },
  { "INPT0.20",  0x28, EQF_READ },
  { "INPT1.20",  0x29, EQF_READ },
  { "INPT2.20",  0x2A, EQF_READ },
  { "INPT3.20",  0x2B, EQF_READ },
  { "INPT4.20",  0x2C, EQF_READ },
  { "INPT5.20",  0x2D, EQF_READ },

// Mirrored $30-based TIA read locations:
  { "CXM0P.30",  0x30, EQF_READ },
  { "CXM1P.30",  0x31, EQF_READ },
  { "CXP0FB.30", 0x32, EQF_READ },
  { "CXP1FB.30", 0x33, EQF_READ },
  { "CXM0FB.30", 0x34, EQF_READ },
  { "CXM1FB.30", 0x35, EQF_READ },
  { "CXBLPF.30", 0x36, EQF_READ },
  { "CXPPMM.30", 0x37, EQF_READ },
  { "INPT0.30",  0x38, EQF_READ },
  { "INPT1.30",  0x39, EQF_READ },
  { "INPT2.30",  0x3A, EQF_READ },
  { "INPT3.30",  0x3B, EQF_READ },
  { "INPT4.30",  0x3C, EQF_READ },
  { "INPT5.30",  0x3D, EQF_READ },

// Standard RIOT locations (read, write, or both):
  { "SWCHA",    0x280, EQF_READ | EQF_WRITE },
  { "SWCHB",    0x282, EQF_READ | EQF_WRITE },
  { "SWACNT",   0x281, EQF_WRITE },
  { "SWBCNT",   0x283, EQF_WRITE },
  { "INTIM",    0x284, EQF_READ  },
  { "TIM1T",    0x294, EQF_WRITE },
  { "TIM8T",    0x295, EQF_WRITE },
  { "TIM64T",   0x296, EQF_WRITE },
  { "TIM1024T", 0x297, EQF_WRITE }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EquateList::EquateList()
{
  int size = sizeof(hardCodedEquates)/sizeof(struct Equate);

  for(int i = 0; i < size; i++)
  {
    Equate e = hardCodedEquates[i];

    myHardcodedFwdMap.insert(make_pair(e.label, e));
    myHardcodedRevMap.insert(make_pair(e.address, e));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EquateList::~EquateList()
{
  myHardcodedFwdMap.clear();
  myHardcodedRevMap.clear();

  myUserFwdMap.clear();
  myUserRevMap.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EquateList::addEquate(const string& label, int address)
{
  // First check if this already exists as a hard-coded equate
  LabelToAddr::const_iterator iter = myHardcodedFwdMap.find(label);
  if(iter != myHardcodedFwdMap.end() && iter->second.address == address)
  {
    cerr << "skipping " << label << endl;
    return;
  }
  removeEquate(label);

cerr << "add: label = " << label << ", address = " << hex << address << endl;

  // Create a new user equate, and analyze the address to determine its
  // probable type (ie, what flags?)
  Equate e;
  e.label = label;
  e.address = address;
  e.flags = EQF_USER;

  if(address >= 0x80 && address <= 0xff)
    e.flags |= EQF_RAM;
  else if(address & 0xf000 == 0xf000)
    e.flags |= EQF_ROM;

  myUserFwdMap.insert(make_pair(label, e));
  myUserRevMap.insert(make_pair(address, e));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EquateList::removeEquate(const string& label)
{
  LabelToAddr::iterator iter = myUserFwdMap.find(label);
  if(iter == myUserFwdMap.end())
  {
    return false;
  }
  else
  {
	 // FIXME: error check?
    // FIXME: memory leak!
    myUserRevMap.erase( myUserRevMap.find(iter->second.address) );
    myUserFwdMap.erase(iter);
    return true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& EquateList::getLabel(int addr, const int flags)
{
  AddrToLabel::const_iterator iter = myHardcodedRevMap.find(addr);

  if(iter != myHardcodedRevMap.end())
  {
    // FIXME - until we fix the issue of correctly setting the equate
    //         flags to something other than 'EQF_ANY' by default,
    //         this comparison will almost always fail
    if(1)//flags == EQF_ANY || iter->second.flags & flags)
      return iter->second.label;
  }
  return EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EquateList::getFormatted(int addr, int places, const int flags)
{
  char fmt[10], buf[255];
  const string& label = getLabel(addr, flags);

  if(label != "")
    return label;

  sprintf(fmt, "$%%0%dx", places);
  sprintf(buf, fmt, addr);

  return buf;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int EquateList::getAddress(const string& label, const int flags)
{
  LabelToAddr::const_iterator iter = myHardcodedFwdMap.find(label);
  if(iter == myHardcodedFwdMap.end())
    return -1;
  else
    return iter->second.address;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EquateList::loadFile(const string& file)
{
  int pos = 0, lines = 0, curVal;
  string curLabel;
  char line[1024];

  ifstream in(file.c_str());
  if(!in.is_open())
    return "Unable to read symbols from " + file;

  myUserFwdMap.clear();
  myUserRevMap.clear();

  while( !in.eof() )
  {
    curVal = 0;
    curLabel = "";

    int got = in.get();

    if(got == -1 || got == '\r' || got == '\n' || pos == 1023) {
      line[pos] = '\0';
      pos = 0;

      if(strlen(line) > 0 && line[0] != '-')
      {
        curLabel = extractLabel(line);
        if((curVal = extractValue(line)) < 0)
          return "invalid symbol file";
  
        addEquate(curLabel, curVal);
  
        lines++;
      }
    }
    else
    {
      line[pos++] = got;
    }
  }
  in.close();

  return "loaded " + file + " OK";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EquateList::saveFile(const string& file)
{
  char buf[256];

  ofstream out(file.c_str());
  if(!out.is_open())
    return false;

  out << "--- Symbol List (sorted by symbol)" << endl;

  LabelToAddr::iterator iter;
  for(iter = myHardcodedFwdMap.begin(); iter != myHardcodedFwdMap.end(); iter++)
  {
    sprintf(buf, "%-24s %04x                  \n", iter->second.label.c_str(), iter->second.address);
    out << buf;
  }

  out << "--- End of Symbol List." << endl;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int EquateList::countCompletions(const char *in)
{
  int count = 0;
  myCompletions = myCompPrefix = "";

  LabelToAddr::iterator iter;
  for(iter = myHardcodedFwdMap.begin(); iter != myHardcodedFwdMap.end(); iter++)
  {
    const char *l = iter->first.c_str();

    if(BSPF_strncasecmp(l, in, strlen(in)) == 0)
    {
      if(myCompPrefix == "")
        myCompPrefix += l;
      else
      {
        int nonMatch = 0;
        const char *c = myCompPrefix.c_str();
        while(*c != '\0' && tolower(*c) == tolower(l[nonMatch]))
        {
          c++;
          nonMatch++;
        }
        myCompPrefix.erase(nonMatch, myCompPrefix.length());
      }

      if(count++) myCompletions += "  ";
        myCompletions += l;
    }
  }

  return count;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EquateList::extractLabel(char *c)
{
  string l = "";
  while(*c != ' ')
    l += *c++;

  return l;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int EquateList::extractValue(char *c)
{
  while(*c != ' ')
  {
    if(*c == '\0')
      return -1;
    c++;
  }

  while(*c == ' ')
  {
    if(*c == '\0')
      return -1;
    c++;
  }

  int ret = 0;
  for(int i=0; i<4; i++)
  {
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
