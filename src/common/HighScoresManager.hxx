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

class OSystem;

namespace HSM {
  static const uInt32 MAX_PLAYERS = 4;
  static const uInt32 MAX_ADDR_CHARS = 4;
  static const uInt32 MAX_SCORE_DIGITS = 6;
  static const uInt32 MAX_SCORE_ADDR = 3;
  static const uInt32 MAX_SPECIAL_NAME = 5;
  static const uInt32 MAX_SPECIAL_DIGITS = 3;

  static const uInt32 DEFAULT_PLAYER = 1;
  static const uInt32 DEFAULT_VARIATION = 1;
  static const uInt32 DEFAULT_ADDRESS = 0;

  using ScoreAddresses = array<Int16, MAX_SCORE_ADDR>;
  using ScoresAddresses = array<ScoreAddresses, MAX_PLAYERS>;

  struct ScoresInfo {
    // Formats
    uInt32 numDigits;
    uInt32 trailingZeroes;
    bool scoreBCD;
    bool varsBCD;
    bool varsZeroBased;
    string special;
    bool specialBCD;
    bool specialZeroBased;
    // Addresses
    ScoresAddresses scoresAddr;
    uInt16 varsAddr;
    uInt16 playersAddr;
    uInt16 specialAddr;
  };

} // namespace HSM

using namespace HSM;


/**
  This class provides an interface to define, load and save scores. It is meant
  for games which do not support saving highscores.

  TODO: load and saves scores

  @author  Thomas Jentzsch
*/

class HighScoresManager
{
  public:
    HighScoresManager(OSystem& osystem);
    virtual ~HighScoresManager() = default;

    /**
      Get the highscore data of game's properties

      @return True if highscore data exists, else false
    */
    bool get(const Properties& props, uInt32& numPlayers, uInt32& numVariations,
             ScoresInfo& info) const;
    /**
      Set the highscore data of game's properties
    */
    void set(Properties& props, uInt32 numPlayers, uInt32 numVariations,
             const ScoresInfo& info) const;

    /**
      Calculate the number of bytes for one player's score from given parameters

      @return The number of score address bytes
    */
    uInt32 numAddrBytes(Int32 digits, Int32 trailing) const;

    // Retrieve current values from (using given parameters)
    Int32 player(uInt16 addr, uInt32 numPlayers, bool zeroBased = true) const;
    Int32 variation(uInt16 addr, bool varBCD, bool zeroBased, uInt32 numVariations) const;
    /**
      Calculate the score from given parameters

      @return The current score or -1 if no valid data exists
    */
    Int32 score(uInt32 player, uInt32 numAddrBytes, uInt32 trailingZeroes, bool isBCD,
                const ScoreAddresses& scoreAddr) const;

    Int32 special(uInt16 addr, bool varBCD, bool zeroBased) const;

    // Retrieve current values (using game's properties)
    Int32 numVariations() const;
    Int32 player() const;
    string specialLabel() const;
    Int32 variation() const;
    Int32 score() const;
    Int32 special() const;

  private:
    static const uInt32 MAX_VARIATIONS = 256;

    static const uInt32 MAX_TRAILING = 3;
    static const uInt32 DEFAULT_DIGITS = 4;
    static const uInt32 DEFAULT_TRAILING = 0;
    static const bool DEFAULT_SCORE_BCD = true;
    static const bool DEFAULT_VARS_BCD = true;
    static const bool DEFAULT_VARS_ZERO_BASED = false;
    static const bool DEFAULT_PLAYERS_ZERO_BASED = true;
    static const bool DEFAULT_SPECIAL_BCD = true;
    static const bool DEFAULT_SPECIAL_ZERO_BASED = false;

  private:
    // Get individual highscore info from properties
    uInt32 numVariations(const Properties& props) const;
    uInt32 numPlayers(const Properties& props) const;
    uInt16 varAddress(const Properties& props) const;
    uInt16 playerAddress(const Properties& props) const;
    uInt16 specialAddress(const Properties& props) const;
    uInt32 numDigits(const Properties& props) const;
    uInt32 trailingZeroes(const Properties& props) const;
    bool scoreBCD(const Properties& props) const;
    bool varBCD(const Properties& props) const;
    bool varZeroBased(const Properties& props) const;
    string specialLabel(const Properties& props) const;
    bool specialBCD(const Properties& props) const;
    bool specialZeroBased(const Properties& props) const;
    bool playerZeroBased(const Properties& props) const;

    // Calculate the number of bytes for one player's score from property parameters
    uInt32 numAddrBytes(const Properties& props) const;

    // Get properties
    Properties& properties(Properties& props) const;
    // Get value from highscore propterties at given index
    string getPropIdx(const Properties& props, PropType type, uInt32 idx = 0) const;
    // Peek into memory
    Int16 peek(uInt16 addr) const;

    Int32 fromBCD(uInt8 bcd) const;

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
