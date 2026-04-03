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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef STRING_PARSER_HXX
#define STRING_PARSER_HXX

#include "bspf.hxx"

/**
  This class converts a string into a StringList by splitting on a delimiter
  and size.

  TODO: This class is actually optimized to work with and store string_view.
        However, StringList currently stores string, not string_view.
        This is still a little more efficient, but would be *much* more
        efficient if we could convert StringList somehow.

  @author Stephen Anthony
*/
class StringParser
{
  public:
    /**
      Split the given string based on the newline character.

      @param str  The string to split
    */
    explicit StringParser(string_view str)
      : myBuffer{str}
    {
      parseLines();
    }

    /**
      Split the given string based on the newline character, making sure that
      no string is longer than maximum string length.

      @param str    The string to split
      @param maxlen The maximum length of string to generate
    */
    StringParser(string_view str, size_t maxlen)
      : myBuffer(str)
    {
      parseWrapped(maxlen);
    }

    ~StringParser() = default;

    const StringList& stringList() const { return myStringList; }

  private:
    string myBuffer;
    StringList myStringList;

    void parseLines()
    {
      size_t start = 0;
      const size_t n = myBuffer.size();

      while(start < n)
      {
        size_t end = myBuffer.find('\n', start);
        if(end == string::npos)
          end = n;

        myStringList.emplace_back(myBuffer.data() + start, end - start);
        start = end + 1;
      }
    }

    void parseWrapped(size_t maxlen)
    {
      size_t start = 0;
      const size_t n = myBuffer.size();

      while(start < n)
      {
        size_t end = myBuffer.find('\n', start);
        if(end == string::npos)
          end = n;

        wrapLine(string_view(myBuffer.data() + start, end - start), maxlen);
        start = end + 1;
      }
    }

    void wrapLine(string_view line, size_t maxlen)
    {
      size_t beg = 0;

      while(beg < line.size())
      {
        const size_t len = std::min(maxlen, line.size() - beg);
        size_t split = beg + len;

        if(split < line.size())
        {
          const size_t space = line.rfind(' ', split);
          if(space != string_view::npos && space > beg)
            split = space;
        }

        myStringList.emplace_back(line.data() + beg, split - beg);

        // Skip space if we split on one
        if(split < line.size() && line[split] == ' ')
          beg = split + 1;
        else
          beg = split;
      }
    }

  private:
    // Following constructors and assignment operators not supported
    StringParser() = delete;
    StringParser(const StringParser&) = delete;
    StringParser(StringParser&&) = delete;
    StringParser& operator=(const StringParser&) = delete;
    StringParser& operator=(StringParser&&) = delete;
};

#endif
