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
// Copyright (c) 1995-2010 by Bradford W. Mott, Stephen Anthony
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
  clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameList::appendGame(const string& name, const string& path,
                          const string& md5, bool isDir)
{
  Entry g;
  g._name  = name;
  g._path  = path;
  g._md5   = md5;
  g._isdir = isDir;

  myArray.push_back(g);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameList::sortByName()
{
  if(myArray.size() <= 1)
    return;

  sort(myArray.begin(), myArray.end());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool GameList::Entry::operator< (const Entry& g) const
{
  string::const_iterator it1 = _name.begin();
  string::const_iterator it2 = g._name.begin();

  // Account for ending ']' character in directory entries
  string::const_iterator end1 = _isdir ? _name.end() - 1 : _name.end();
  string::const_iterator end2 = g._isdir ? g._name.end() - 1 : g._name.end();

  // Stop when either string's end has been reached
  while((it1 != end1) && (it2 != end2)) 
  { 
    if(toupper(*it1) != toupper(*it2)) // letters differ?
      return toupper(*it1) < toupper(*it2);

    // proceed to the next character in each string
    ++it1;
    ++it2;
  }
  return _name.size() < g._name.size();
}
