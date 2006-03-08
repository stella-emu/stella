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
// $Id: GameList.cxx,v 1.7 2006-03-08 20:03:03 stephena Exp $
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
  myArray.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameList::appendGame(const string& rom, const string& name,
                          const string& note, bool isDir)
{
  Entry g;
  g._rom   = rom;
  g._name  = name;
  g._note  = note;
  g._isdir = isDir;

  myArray.push_back(g);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameList::sortByName()
{
  if(myArray.size() <= 1)
    return;

  // Simple selection sort
  for (unsigned int i = 0; i < myArray.size()-1; i++)
  {
    unsigned int min = i;
    for (unsigned int j = i+1; j < myArray.size(); j++)
    {
      // TODO - add option for this
      string atJ   = myArray[j]._name;
      string atMin = myArray[min]._name;
      transform(atJ.begin(), atJ.end(), atJ.begin(), (int(*)(int)) toupper);
      transform(atMin.begin(), atMin.end(), atMin.begin(), (int(*)(int)) toupper);

      if (atJ < atMin)
        min = j;
    }
    if (min != i)
      SWAP(myArray[min], myArray[i]);
  }
}
