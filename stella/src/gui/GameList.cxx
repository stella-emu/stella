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
// $Id: GameList.cxx,v 1.9 2006-03-10 00:29:46 stephena Exp $
//
//   Based on code from KStella - Stella frontend
//   Copyright (C) 2003-2005 Stephen Anthony
//============================================================================

#include <cctype>
#include <algorithm>

#include "GuiUtils.hxx"
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
                          const string& note, bool isDir)
{
  Entry* g = new Entry;
  g->_name  = name;
  g->_path  = path;
  g->_note  = note;
  g->_isdir = isDir;

  myArray.push_back(g);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameList::clear()
{
  for(unsigned int i = 0; i < myArray.size(); ++i)
    delete myArray[i];

  myArray.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameList::sortByName()
{
  if(myArray.size() <= 1)
    return;

  QuickSort(myArray, 0, myArray.size()-1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameList::QuickSort(EntryList& a, int left, int right)
{
  int l_hold = left;
  int r_hold = right;
  Entry* pivot_entry = a[left];

  while (left < right)
  {
    while ((compare(a[right]->_name, pivot_entry->_name) >= 0) && (left < right))
      right--;
    if (left != right)
    {
      a[left] = a[right];
      left++;
    }
    while ((compare(a[left]->_name, pivot_entry->_name) <= 0) && (left < right))
      left++;
    if (left != right)
    {
      a[right] = a[left];
      right--;
    }
  }

  a[left] = pivot_entry;
  int pivot = left;
  left = l_hold;
  right = r_hold;
  if (left < pivot)
    QuickSort(a, left, pivot-1);
  if (right > pivot)
    QuickSort(a, pivot+1, right);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int GameList::compare(const string& s1, const string& s2)
{
  string::const_iterator it1=s1.begin();
  string::const_iterator it2=s2.begin();

  // Stop when either string's end has been reached
  while( (it1 != s1.end()) && (it2 != s2.end()) ) 
  { 
    if(::toupper(*it1) != ::toupper(*it2)) // letters differ?
      // return -1 to indicate smaller than, 1 otherwise
      return (::toupper(*it1)  < ::toupper(*it2)) ? -1 : 1; 

    // proceed to the next character in each string
    ++it1;
    ++it2;
  }
  size_t size1 = s1.size(), size2 = s2.size(); // cache lengths

  // return -1,0 or 1 according to strings' lengths
  if (size1 == size2) 
    return 0;
  else
    return (size1 < size2) ? -1 : 1;
}
