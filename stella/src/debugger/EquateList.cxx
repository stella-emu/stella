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
// $Id: EquateList.cxx,v 1.19 2005-10-11 17:14:34 stephena Exp $
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EquateList::EquateList()
{
  int size = sizeof(hardCodedEquates)/sizeof(struct Equate);

  for(int i=0; i<size; i++)
  {
    string l = hardCodedEquates[i].label;	
    int a = hardCodedEquates[i].address;

    myFwdMap.insert(make_pair(l, a));
    myRevMap.insert(make_pair(a, l));
  }
  calcSize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EquateList::~EquateList()
{
  myFwdMap.clear();
  myRevMap.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int EquateList::calcSize() {
	currentSize = myFwdMap.size();
	return currentSize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& EquateList::getLabel(int addr)
{
  static string nothing = "";
  addrToLabel::const_iterator iter = myRevMap.find(addr);

  if(iter == myRevMap.end())
    return nothing;
  else
    return iter->second;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// returns either the label, or a formatted hex string
// if no label found.
const char *EquateList::getFormatted(int addr, int places)
{
  static char fmt[10], buf[255];
  string res = getLabel(addr);
  if(res != "")
    return res.c_str();

  sprintf(fmt, "$%%0%dx", places);
  //cerr << addr << ", " << fmt << ", " << places << endl;
  sprintf(buf, fmt, addr);

  return buf;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int EquateList::getAddress(const string& label)
{
  labelToAddr::const_iterator iter = myFwdMap.find(label);
  if(iter == myFwdMap.end())
    return -1;
  else
    return iter->second;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EquateList::undefine(string& label)
{
  labelToAddr::iterator iter = myFwdMap.find(label);
  if(iter == myFwdMap.end())
  {
    return false;
  }
  else
  {
    myRevMap.erase( myRevMap.find(iter->second) ); // FIXME: error check?
    myFwdMap.erase(iter);
    return true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EquateList::saveFile(string file)
{
  char buf[256];

  ofstream out(file.c_str());
  if(!out.is_open())
    return false;

  out << "--- Symbol List (sorted by symbol)" << endl;

  labelToAddr::iterator iter;
  for(iter = myFwdMap.begin(); iter != myFwdMap.end(); iter++)
  {
    sprintf(buf, "%-24s %04x                  \n", iter->first.c_str(), iter->second);
    out << buf;
  }

  out << "--- End of Symbol List." << endl;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EquateList::loadFile(string file)
{
  int lines = 0, curVal;
  string curLabel;
  char buffer[256]; // FIXME: static buffers suck

  ifstream in(file.c_str());
  if(!in.is_open())
    return "Unable to read symbols from " + file;

  myFwdMap.clear();
  myRevMap.clear();

  while( !in.eof() )
  {
    curVal = 0;
    curLabel = "";

    if(!in.getline(buffer, 255))
      break;

    if(buffer[0] != '-')
    {
      curLabel = extractLabel(buffer);
      if((curVal = extractValue(buffer)) < 0)
        return "invalid symbol file";

      addEquate(curLabel, curVal);

      lines++;
    }
  }
  in.close();

  calcSize();

  return "loaded " + file + " OK";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EquateList::addEquate(string label, int address)
{
  undefine(label);
  myFwdMap.insert(make_pair(label, address));
  myRevMap.insert(make_pair(address, label));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int EquateList::parse4hex(char *c)
{
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

  return parse4hex(c);
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
int EquateList::countCompletions(const char *in)
{
  int count = 0;
  completions = compPrefix = "";

  labelToAddr::iterator iter;
  for(iter = myFwdMap.begin(); iter != myFwdMap.end(); iter++)
  {
    const char *l = iter->first.c_str();

    if(STR_N_CASE_CMP(l, in, strlen(in)) == 0)
    {
      if(compPrefix == "")
        compPrefix += l;
      else
      {
        int nonMatch = 0;
        const char *c = compPrefix.c_str();
        while(*c != '\0' && tolower(*c) == tolower(l[nonMatch]))
        {
          c++;
          nonMatch++;
        }
        compPrefix.erase(nonMatch, compPrefix.length());
      }

      if(count++) completions += "  ";
        completions += l;
    }
  }

  return count;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char *EquateList::getCompletions()
{
  return completions.c_str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char *EquateList::getCompletionPrefix()
{
  return compPrefix.c_str();
}
