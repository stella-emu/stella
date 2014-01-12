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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
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
      Split the given string based on the newline character.

      @param str  The string to split
    */
    StringParser(const string& str)
    {
      stringstream buf(str);
      string line;

      while(std::getline(buf, line, '\n'))
        myStringList.push_back(line);
    }

    /**
      Split the given string based on the newline character, making sure that
      no string is longer than maximum string length.

      @param str    The string to split
      @param maxlen The maximum length of string to generate
    */
    StringParser(const string& str, uInt16 maxlen)
    {
      stringstream buf(str);
      string line;

      while(std::getline(buf, line, '\n'))
      {
        size_t beg = 0, len = maxlen, size = line.size();

        if(size <= len)
          myStringList.push_back(line);
        else
        {
          while((beg+maxlen) < size)
          {
            size_t spos = line.find_last_of(' ', beg+len);
            if(spos > beg)
              len = spos - beg;

            myStringList.push_back(line.substr(beg, len));
            beg += len + 1;
            len = maxlen;
          }
          myStringList.push_back(line.substr(beg));
        }
      }
    }

    const StringList& stringList() const { return myStringList; }

  private:
    StringList myStringList;
};

#endif
