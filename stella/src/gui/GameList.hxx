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
// $Id: GameList.hxx,v 1.11 2006-04-12 13:32:06 stephena Exp $
//
//   Based on code from KStella - Stella frontend
//   Copyright (C) 2003-2005 Stephen Anthony
//============================================================================

#ifndef GAME_LIST_HXX
#define GAME_LIST_HXX

#include <vector>
#include "bspf.hxx"

/**
  Holds the list of game info for the ROM launcher.
*/
class GameList
{
  public:
    GameList();
    ~GameList();

    inline const string& name(int i)
    { return i < (int)myArray.size() ? myArray[i]._name : EmptyString; }
    inline const string& path(int i)
    { return i < (int)myArray.size() ? myArray[i]._path : EmptyString; }
    inline const string& note(int i)
    { return i < (int)myArray.size() ? myArray[i]._note : EmptyString; }
    inline const bool isDir(int i)
    { return i < (int)myArray.size() ? myArray[i]._isdir: false; }

    inline int size() { return myArray.size(); }
    inline void clear() { myArray.clear(); }

    void appendGame(const string& name, const string& path, const string& note,
                    bool isDir = false);
    void sortByName();

  private:
    class Entry {
      public:
        string _name;
        string _path;
        string _note;
        bool   _isdir;

        bool operator < (const Entry& a) const;
    };
    vector<Entry> myArray;
};

#endif
