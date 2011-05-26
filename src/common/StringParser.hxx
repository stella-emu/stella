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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef STRING_PARSER_HXX
#define STRING_PARSER_HXX

#include "StringList.hxx"
#include "bspf.hxx"

/**
  This class converts a string into a StringList by splitting on a delimiter.
  By default, the delimiter is a newline.

  @author Stephen Anthony
*/
class StringParser
{
  public:
    StringParser(const string& str, char delim = '\n')
    {
      stringstream buf(str);
      string line;
      while(std::getline(buf, line, delim))
        myStringList.push_back(line);
    }

    const StringList& stringList() const { return myStringList; }

  private:
    StringList myStringList;
};

#endif
