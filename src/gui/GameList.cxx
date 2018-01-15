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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
//   Based on code from KStella - Stella frontend
//   Copyright (C) 2003-2005 Stephen Anthony
//============================================================================

#include <cctype>
#include <algorithm>

#include "GameList.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameList::sortByName()
{
  if(myArray.size() < 2)
    return;

  auto cmp = [](const Entry& a, const Entry& b)
  {
    // directories always first
    if(a._isdir != b._isdir)
      return a._isdir;

    auto it1 = a._name.cbegin(), it2 = b._name.cbegin();

    // Account for ending ']' character in directory entries
    auto end1 = a._isdir ? a._name.cend() - 1 : a._name.cend();
    auto end2 = b._isdir ? b._name.cend() - 1 : b._name.cend();

    // Stop when either string's end has been reached
    while((it1 != end1) && (it2 != end2))
    {
      if(toupper(*it1) != toupper(*it2)) // letters differ?
        return toupper(*it1) < toupper(*it2);

      // proceed to the next character in each string
      ++it1;
      ++it2;
    }
    return a._name.size() < b._name.size();
  };

  sort(myArray.begin(), myArray.end(), cmp);
}
