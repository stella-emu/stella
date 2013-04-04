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
// Copyright (c) 1995-2013 by Bradford W. Mott, Stephen Anthony
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
  This class converts a string into a StringList by splitting on a delimiter
  and size.

  @author Stephen Anthony
*/
class StringParser
{
  public:
    /**
      Split the given string based on delimiter (by default, the newline
      character, and by desired length (by default, not used).

      @param str    The string to split
      @param len    The maximum length of string to generate (0 means unlimited)
      @param delim  The character indicating the end of a line (newline by default)
    */
    StringParser(const string& str, uInt32 len = 0, char delim = '\n')
    {
      stringstream buf(str);
      string line;
      while(std::getline(buf, line, delim))
      {
        myStringList.push_back(line);
      }
    }

    const StringList& stringList() const { return myStringList; }

  private:
    StringList myStringList;
};

#endif
