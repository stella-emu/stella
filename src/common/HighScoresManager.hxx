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

#ifndef HIGHSCORES_MANAGER_HXX
#define HIGHSCORES_MANAGER_HXX

//#include "bspf.hxx"

class OSystem;

class HighScoresManager
{

public:
  static const uInt32 MAX_PLAYERS = 4;
  static const uInt32 MAX_SCORE_ADDR = 3;

  using ScoreAddresses = BSPF::array2D<Int16, MAX_PLAYERS, MAX_SCORE_ADDR>;

  struct Formats {
    uInt32 numDigits;
    uInt32 trailingZeroes;
    bool scoreBCD;
    bool varBCD;
    bool varZeroBased;
  };


  struct Addresses {
    ScoreAddresses scoreAddr;
    uInt16 varAddr;
    uInt16 playerAddr;
  };

  HighScoresManager(OSystem& osystem);
  virtual ~HighScoresManager() = default;

  /*
    Methods for returning high scores related variables
  */
  uInt32 numVariations(const Properties& props) const;
  uInt32 numPlayers(const Properties& props) const;

  bool get(const Properties& props, Formats& formats, Addresses& addresses) const;
  void set(Properties& props, uInt32 numPlayers, uInt32 numVariations,
           const Formats& formats, const Addresses& addresses) const;

  uInt32 numDigits(const Properties& props) const;
  uInt32 trailingZeroes(const Properties& props) const;
  bool scoreBCD(const Properties& props) const;
  bool varBCD(const Properties& props) const;
  bool varZeroBased(const Properties& props) const;

  uInt16 varAddress(const Properties& props) const;
  uInt16 playerAddress(const Properties& props) const;

  uInt32 numAddrBytes(Int32 digits, Int32 trailing) const;
  uInt32 numAddrBytes(const Properties& props) const;

  // current values
  Int32 player();
  Int32 variation();
  Int32 score();

  Int32 score(uInt32 player, uInt32 numAddrBytes, uInt32 trailingZeroes, bool isBCD) const;
  Int32 score(const Properties& props, uInt32 player) const;

private:
  Properties& properties(Properties& props) const;
  string getPropIdx(const Properties& props, PropType type, uInt32 idx = 0) const;
  Int16 peek(uInt16 addr) const;
  bool parseAddresses(uInt32& variation, uInt32& player, uInt32 scores[]);

private:
  // Reference to the osystem object
  OSystem& myOSystem;

private:
  // Following constructors and assignment operators not supported
  HighScoresManager() = delete;
  HighScoresManager(const HighScoresManager&) = delete;
  HighScoresManager(HighScoresManager&&) = delete;
  HighScoresManager& operator=(const HighScoresManager&) = delete;
  HighScoresManager& operator=(HighScoresManager&&) = delete;
};
#endif
