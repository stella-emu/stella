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
//
//   Based on code from KStella - Stella frontend
//   Copyright (C) 2003-2005 Stephen Anthony
//============================================================================

#include <cctype>
#include <algorithm>

#include "GameList.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GameList::GameList()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GameList::~GameList()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameList::appendGame(const string& name, const string& path,
                          const string& md5, bool isDir)
{
  myArray.emplace_back(name, path, md5, isDir);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameList::sortByName()
{
  if(myArray.size() < 2)
    return;

  auto cmp = [](const Entry& a, const Entry& b)
  {
    auto it1 = a._name.begin(), it2 = b._name.begin();

    // Account for ending ']' character in directory entries
    auto end1 = a._isdir ? a._name.end() - 1 : a._name.end();
    auto end2 = b._isdir ? b._name.end() - 1 : b._name.end();

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
