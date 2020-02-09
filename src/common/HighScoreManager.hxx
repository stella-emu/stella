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

//#include "bspf.hxx"

class OSystem;

class HighScoreManager
{

public:

  static const uInt32 MAX_PLAYERS = 4;
  static const uInt32 MAX_SCORE_ADDR = 3;

  struct Formats {
    uInt32 numDigits;
    uInt32 trailingZeroes;
    bool scoreBCD;
    bool varBCD;
    bool varZeroBased;
  };


  struct Addresses {
    uInt16 scoreAddr[HighScoreManager::MAX_PLAYERS][HighScoreManager::MAX_SCORE_ADDR];
    uInt16 varAddr;
    uInt16 playerAddr;
  };

  HighScoreManager(OSystem& osystem);
  virtual ~HighScoreManager() = default;

  /*
    Methods for returning high score related variables
    Return -1 if undefined
  */
  uInt32 numVariations() const;
  uInt32 numPlayers() const;

  bool getFormats(Formats& formats) const;
  bool getAddresses(Addresses& addresses) const;

  uInt32 numDigits() const;
  uInt32 trailingZeroes() const;
  bool scoreBCD() const;
  bool varBCD() const;
  bool varZeroBased() const;

  uInt16 varAddress() const;
  uInt16 playerAddress() const;

  uInt32 numAddrBytes(Int32 digits = -1, Int32 trailing = -1) const;

  // current values
  Int32 player();
  Int32 variation();
  Int32 score();

private:
  Properties& properties(Properties& props) const;
  string getPropIdx(PropType type, uInt32 idx = 0) const;
  Int16 peek(uInt16 addr);
  bool parseAddresses(uInt32& variation, uInt32& player, uInt32 scores[]);

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
