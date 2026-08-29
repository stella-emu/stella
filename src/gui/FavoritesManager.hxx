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

#ifndef FAVORITES_MANAGER_HXX
#define FAVORITES_MANAGER_HXX

class Settings;

#include <deque>
#include <unordered_map>
#include <unordered_set>

#include "bspf.hxx"

/**
  Manages user defined favorites, recently played ROMs and most popular ROMs.

  @author  Thomas Jentzsch
*/
class FavoritesManager
{
  public:
    using UserList    = std::vector<string>;
    using RecentList  = std::deque<string>;
    // A ROM path paired with its popularity score
    using PopularType = std::pair<string, uInt32>;
    using PopularList = std::vector<PopularType>;

    explicit FavoritesManager(Settings& settings);
    ~FavoritesManager() = default;

    // Load/save all three lists as JSON, via the _favoriteroms/_recentroms/
    // _popularroms settings
    void load();
    void save();
    // Empties all three lists (without touching settings until save())
    void clear();

    // User favorites
    void addUser(string_view path);
    void removeUser(string_view path);
    void removeAllUser();
    // Adds if absent, else removes; returns the resulting favorite state
    bool toggleUser(string_view path);
    bool existsUser(string_view path) const;
    // Newest-first, or alphabetical if the 'altsorting' setting is on
    const UserList& userList() const;

    // Records a play of 'path': bumps it in both Recently Played and Most Popular
    void update(string_view path);

    // Recently played
    void removeRecent(string_view path);
    void removeAllRecent();
    // Newest-first, or alphabetical if the 'altsorting' setting is on
    const RecentList& recentList() const;

    // Most popular
    bool removePopular(string_view path);
    void removeAllPopular();
    // Most-popular-first, or alphabetical if the 'altsorting' setting is on
    const PopularList& popularList() const;


  private:
    using PopularMap = std::unordered_map<
        string, uInt32, BSPF::StringHash, std::equal_to<>>;
    using UserSet = std::unordered_set<
        string, BSPF::StringHash, std::equal_to<>>;

    UserSet myUserSet;
    // Play order, oldest first; capped at myMaxRecent (see addRecent())
    RecentList myRecentList;
    // Path -> popularity score, aged/evicted by incPopular() once full
    PopularMap myPopularMap;
    // Cap on myRecentList's size, from the 'maxrecentroms' setting
    uInt32 myMaxRecent{20};

    Settings& mySettings;

  private:
    // Moves 'path' to the end of myRecentList, trimming to myMaxRecent
    void addRecent(string_view path);
    // Bumps 'path's popularity score; once the map is full, ages every entry
    // and evicts whichever fall below the threshold
    void incPopular(string_view path);
    const PopularList& sortedPopularList(bool sortByName = false) const;

  private:
    // Following constructors and assignment operators not supported
    FavoritesManager() = delete;
    FavoritesManager(const FavoritesManager&) = delete;
    FavoritesManager(FavoritesManager&&) = delete;
    FavoritesManager& operator=(const FavoritesManager&) = delete;
    FavoritesManager& operator=(FavoritesManager&&) = delete;
};

#endif  // FAVORITES_MANAGER_HXX
