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

#include "OSystem.hxx"
//#include "Props.hxx"
#include "PropsSet.hxx"
#include "Console.hxx"
#include "Launcher.hxx"
#include "System.hxx"

#include "HighScoreManager.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HighScoreManager::HighScoreManager(OSystem& osystem)
  : myOSystem(osystem)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Properties& HighScoreManager::properties(Properties& props) const
{
  if (myOSystem.hasConsole())
  {
    props = myOSystem.console().properties();
  }
  else
  {
    const string& md5 = myOSystem.launcher().selectedRomMD5();
    myOSystem.propSet().getMD5(md5, props);
  }
  return props;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 HighScoreManager::numVariations()
{
  Properties props;
  string numVariations = properties(props).get(PropType::Cart_Variations);

  return (numVariations == EmptyString) ? 1 : std::min(stoi(numVariations), 256);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 HighScoreManager::numPlayers()
{
  Properties props;
  string numPlayers = properties(props).get(PropType::Cart_Players);

  return (numPlayers == EmptyString) ? 1 : std::min(stoi(numPlayers), 4);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HighScoreManager::parseAddresses(Int32& variation, Int32& player, Int32 scores[])
{
  variation = 1; player = 0; scores[0] = 0;

  if (!myOSystem.hasConsole())
    return false;

  System& system = myOSystem.console().system();

  // Formats (all optional):
  //   2,             ; score addresses per player
  //   1,             ; score multiplier
  //   B,             ; score format (BCD, HEX)
  //   B,             ; variation format (BCD, HEX)
  //   0,             ; add to variation
  // Addresses (in hex):
  //   n*p-times xx,  ; score addresses for each player, high to low
  //   xx,            ; variation address (if more than 1 variation)
  //   xx,            ; player address (if more than 1 player)
  // TODO:
  // - variation bits (Centipede)
  // - player bits (Asteroids, Space Invaders)
  // - score swaps (Asteroids)
  Properties props;
  string formats = properties(props).get(PropType::Cart_Formats);
  string addresses = properties(props).get(PropType::Cart_Addresses);
  char scoreFormat;
  char varFormat;
  Int16 addr;
  Int32 varAdd, numScoreAddr, scoreMult;

  // Since istringstream swallows whitespace, we have to make the
  // delimiters be spaces
  std::replace(formats.begin(), formats.end(), ',', ' ');
  std::replace(formats.begin(), formats.end(), '|', ' ');
  std::replace(addresses.begin(), addresses.end(), ',', ' ');
  std::replace(addresses.begin(), addresses.end(), '|', ' ');
  istringstream addrBuf(addresses);
  istringstream formatBuf(formats);

  // 1. retrieve formats
  if (!(formatBuf >> numScoreAddr))
    numScoreAddr = 2;
  if (!(formatBuf >> scoreMult))
    scoreMult = 1;
  if (!(formatBuf >> scoreFormat))
    scoreFormat = 'B';
  if (!(formatBuf >> varFormat))
    varFormat = 'B';
  if (!(formatBuf >> varAdd))
    varAdd = 0;

  // 2. retrieve current scores for all players
  for (int i = 0; i < numPlayers(); ++i)
  {
    Int32 totalScore = 0;

    for (int j = 0; j < numScoreAddr; ++j)
    {
      Int32 score;

      if (!(addrBuf >> std::hex >> addr))
        return false;

      totalScore *= (scoreFormat == 'B') ? 100 : 256;

      score = system.peek(addr);
      if (scoreFormat == 'B')
        score = (score >> 4) * 10 + score % 16;
      totalScore += score;
    }
    scores[i] = totalScore * scoreMult;
  }

  // 3. retrieve current variation (0..255)
  if (numVariations() == 1)
    return true;

  if (!(addrBuf >> std::hex >> addr))
    return false;
  variation = system.peek(addr);
  if (varFormat == 'B')
    variation = (variation >> 4) * 10 + variation % 16;
  variation += varAdd;
  variation = std::min(variation, numVariations());

  // 4. retrieve current player (0..3)
  if (numPlayers() == 1)
    return true;

  if (!(addrBuf >> std::hex >> addr))
    return false;

  player = system.peek(addr);
  player = std::min(player, numPlayers() - 1);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 HighScoreManager::variation()
{
  Int32 variation, player, scores[4];

  if (parseAddresses(variation, player, scores))
    return variation;

  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 HighScoreManager::player()
{
  Int32 variation, player, scores[4];

  if (parseAddresses(variation, player, scores))
    return player;

  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 HighScoreManager::score()
{
  Int32 variation, player, scores[4];

  if (parseAddresses(variation, player, scores))
    return scores[std::min(player, Int32(3))];

  return -1;
}