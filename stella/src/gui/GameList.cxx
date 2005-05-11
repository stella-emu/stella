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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: GameList.cxx,v 1.2 2005-05-11 01:44:39 stephena Exp $
//
//   Based on code from KStella - Stella frontend
//   Copyright (C) 2003-2005 Stephen Anthony
//============================================================================

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
void GameList::appendGame(const string& rom, const string& name, const string& note)
{
  Entry g;
  g._rom  = rom;
  g._name = name;
  g._note = note;

  myArray.push_back(g);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameList::sortByName()
{
  // Simple selection sort
  for(uInt32 i = 0; i < myArray.size()-1; i++)
  {
    uInt32 min = i;
    for (uInt32 j = i+1; j < myArray.size(); j++)
      if (myArray[j]._name < myArray[min]._name)
        min = j;
      if (min != i)
        SWAP(myArray[min], myArray[i]);
  }
}
