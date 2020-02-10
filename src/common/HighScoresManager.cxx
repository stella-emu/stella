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

/*
 Formats (all optional):
   4,             ; score digits per player
   0,             ; trailing zeroes
   B,             ; score format (BCD, HEX)
   B,             ; variation format (BCD, HEX)
   0,             ; zero-based variation
 Addresses (in hex):
   n*p-times xx,  ; score addresses for each player, high to low
   xx,            ; variation address (if more than 1 variation)
   xx,            ; player address (if more than 1 player)

 TODO:
 - variation bits (Centipede)
 - player bits (Asteroids, Space Invaders)
 - score swaps (Asteroids)
*/

#include "OSystem.hxx"
//#include "Props.hxx"
#include "PropsSet.hxx"
#include "Console.hxx"
#include "Launcher.hxx"
#include "System.hxx"

#include "HighScoresManager.hxx"



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HighScoresManager::HighScoresManager(OSystem& osystem)
  : myOSystem(osystem)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int16 HighScoresManager::peek(uInt16 addr)
{
  if (myOSystem.hasConsole())
  {
    System& system = myOSystem.console().system();
    return system.peek(addr);
  }
  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Properties& HighScoresManager::properties(Properties& props) const
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
string HighScoresManager::getPropIdx(PropType type, uInt32 idx) const
{
  Properties props;
  string property = properties(props).get(type);
  std::replace(property.begin(), property.end(), ',', ' ');
  std::replace(property.begin(), property.end(), '|', ' ');
  istringstream buf(property);
  string result;

  for (uInt32 i = 0; i <= idx; ++i)
    if(!(buf >> result))
      return "";

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 HighScoresManager::numPlayers() const
{
  string numPlayers = getPropIdx(PropType::Cart_Players);

  return numPlayers == EmptyString ? 1 : std::min(uInt32(stoi(numPlayers)), MAX_PLAYERS);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 HighScoresManager::numVariations() const
{
  string numVariations = getPropIdx(PropType::Cart_Variations);

  return numVariations == EmptyString ? 1 : std::min(stoi(numVariations), 256);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HighScoresManager::getFormats(Formats& formats) const
{
  formats.numDigits = numDigits();
  formats.trailingZeroes = trailingZeroes();
  formats.scoreBCD = scoreBCD();
  formats.varBCD = varBCD();
  formats.varZeroBased = varZeroBased();

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 HighScoresManager::numDigits() const
{
  string digits = getPropIdx(PropType::Cart_Formats, 0);

  return digits == EmptyString ? 4 : stoi(digits);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 HighScoresManager::trailingZeroes() const
{
  string trailing = getPropIdx(PropType::Cart_Formats, 1);

  return trailing == EmptyString ? 0 : stoi(trailing);}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HighScoresManager::scoreBCD() const
{
  string bcd = getPropIdx(PropType::Cart_Formats, 2);

  return bcd == EmptyString ? true : bcd == "B";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HighScoresManager::varBCD() const
{
  string bcd = getPropIdx(PropType::Cart_Formats, 3);

  return bcd == EmptyString ? true : bcd == "B";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HighScoresManager::varZeroBased() const
{
  string zeroBased = getPropIdx(PropType::Cart_Formats, 4);

  return zeroBased == EmptyString ? false : zeroBased != "0";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HighScoresManager::getAddresses(Addresses& addresses) const
{
  addresses.playerAddr = playerAddress();
  addresses.varAddr = varAddress();
  for (uInt32 p = 0; p < MAX_PLAYERS; ++p)
  {
    if (p < numPlayers())
    {
      for (uInt32 a = 0; a < numAddrBytes(); ++a)
      {
        uInt32 idx = p * numAddrBytes() + a;
        string addr = getPropIdx(PropType::Cart_Addresses, idx);

        addresses.scoreAddr[p][a] = (addr == EmptyString ? 0 : stoi(addr, nullptr, 16));
      }
    }
    else
      for (uInt32 a = 0; a < numAddrBytes(); ++a)
        addresses.scoreAddr[p][a] = -1;
  }

  return (EmptyString != getPropIdx(PropType::Cart_Addresses, 0));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 HighScoresManager::playerAddress() const
{
  uInt32 idx = numAddrBytes() * numPlayers() + 1;
  string addr = getPropIdx(PropType::Cart_Addresses, idx);

  return addr == EmptyString ? 0 : stoi(addr, nullptr, 16);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 HighScoresManager::varAddress() const
{
  uInt32 idx = numAddrBytes() * numPlayers();
  string addr = getPropIdx(PropType::Cart_Addresses, idx);

  return addr == EmptyString ? 0 : stoi(addr, nullptr, 16);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 HighScoresManager::numAddrBytes(Int32 digits, Int32 trailing) const
{
  return ((digits < 0 ? numDigits() : digits) - (trailing < 0 ? trailingZeroes() : trailing) + 1) / 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 HighScoresManager::score(uInt32 player, uInt32 numAddrBytes, bool isBCD) const
{
  if (!myOSystem.hasConsole())
    return -1;

  System& system = myOSystem.console().system();
  Int32 totalScore = 0;

  for (uInt32 b = 0; b < numAddrBytes; ++b)
  {
    uInt32 idx = player * numAddrBytes + b;
    string strAddr = getPropIdx(PropType::Cart_Addresses, idx);
    uInt16 addr = (strAddr == EmptyString ? 0 : stoi(strAddr, nullptr, 16));
    uInt32 score;

    totalScore *= isBCD ? 100 : 256;
    score = system.peek(addr);
    if (isBCD)
    {
      // verify if score is legit
      if (score >= 160)
      {
        totalScore = -1;
        break;
      }
      score = (score >> 4) * 10 + score % 16;
    }
    totalScore += score;
  }

  if (totalScore != -1)
    for (uInt32 i = 0; i < trailingZeroes(); ++i)
      totalScore *= 10;

  return totalScore;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 HighScoresManager::score(uInt32 player) const
{
  return score(player, numAddrBytes(), scoreBCD());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HighScoresManager::parseAddresses(uInt32& variation, uInt32& player, uInt32 scores[])
{
  variation = 1; player = 0; scores[0] = 0;

  if (!myOSystem.hasConsole())
    return false;

  System& system = myOSystem.console().system();


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
  for (uInt32 i = 0; i < numPlayers(); ++i)
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
Int32 HighScoresManager::variation()
{
  uInt32 variation, player, scores[4];

  if (parseAddresses(variation, player, scores))
    return variation;

  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 HighScoresManager::player()
{
  uInt32 variation, player, scores[4];

  if (parseAddresses(variation, player, scores))
    return player;

  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 HighScoresManager::score()
{
  uInt32 variation, player, scores[4];

  if (parseAddresses(variation, player, scores))
    return scores[std::min(player, uInt32(MAX_PLAYERS))];

  return -1;
}