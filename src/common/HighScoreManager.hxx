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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef HIGHSCORE_MANAGER_HXX
#define HIGHSCORE_MANAGER_HXX

class OSystem;

class HighScoreManager
{

public:
  HighScoreManager(OSystem& osystem);
  virtual ~HighScoreManager() = default;

  /*
    Methods for returning high score related variables
    Return -1 if undefined
  */
  Int32 numVariations();
  Int32 numPlayers();
  Int32 variation();
  Int32 player();
  Int32 score();

private:
  Properties& properties(Properties& props) const;
  bool parseAddresses(Int32& variation, Int32& player, Int32 scores[]);

private:
  // Reference to the osystem object
  OSystem& myOSystem;

private:
  // Following constructors and assignment operators not supported
  HighScoreManager() = delete;
  HighScoreManager(const HighScoreManager&) = delete;
  HighScoreManager(HighScoreManager&&) = delete;
  HighScoreManager& operator=(const HighScoreManager&) = delete;
  HighScoreManager& operator=(HighScoreManager&&) = delete;
};
#endif
