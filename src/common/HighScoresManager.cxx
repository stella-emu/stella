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
Int16 HighScoresManager::peek(uInt16 addr) const
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
string HighScoresManager::getPropIdx(const Properties& props, PropType type, uInt32 idx) const
{
  string property = props.get(type);

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
uInt32 HighScoresManager::numPlayers(const Properties& props) const
{
  string numPlayers = getPropIdx(props, PropType::Cart_Players);

  return numPlayers == EmptyString ?
    DEFAULT_PLAYER : std::min(uInt32(stoi(numPlayers)), MAX_PLAYERS);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 HighScoresManager::numVariations(const Properties& props) const
{
  string numVariations = getPropIdx(props, PropType::Cart_Variations);

  return numVariations == EmptyString ?
    DEFAULT_VARIATION : std::min(uInt32(stoi(numVariations)), MAX_VARIATIONS);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HighScoresManager::get(const Properties& props, uInt32& numPlayersR, uInt32& numVariationsR,
                            Formats& formats, Addresses& addresses) const
{
  numPlayersR = numPlayers(props);
  numVariationsR = numVariations(props);

  formats.numDigits = numDigits(props);
  formats.trailingZeroes = trailingZeroes(props);
  formats.scoreBCD = scoreBCD(props);
  formats.varsBCD = varBCD(props);
  formats.varsZeroBased = varZeroBased(props);

  addresses.playersAddr = playerAddress(props);
  addresses.varsAddr = varAddress(props);
  for (uInt32 p = 0; p < MAX_PLAYERS; ++p)
  {
    if (p < numPlayersR)
    {
      for (uInt32 a = 0; a < numAddrBytes(props); ++a)
      {
        uInt32 idx = p * numAddrBytes(props) + a;
        string addr = getPropIdx(props, PropType::Cart_Addresses, idx);

        addresses.scoresAddr[p][a] = (addr == EmptyString ? 0 : stoi(addr, nullptr, 16));
      }
    }
    else
      for (uInt32 a = 0; a < numAddrBytes(props); ++a)
        addresses.scoresAddr[p][a] = -1;
  }

  return (EmptyString != getPropIdx(props, PropType::Cart_Addresses, 0));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresManager::set(Properties& props, uInt32 numPlayers, uInt32 numVariations,
                            const Formats& formats, const Addresses& addresses) const
{
  ostringstream buf;

  props.set(PropType::Cart_Players, std::to_string(numPlayers));
  props.set(PropType::Cart_Variations, std::to_string(std::min(numVariations, MAX_VARIATIONS)));



  buf << formats.numDigits << ","
    << formats.trailingZeroes << ","
    << (formats.scoreBCD ? "B" : "H") << ","
    << (formats.varsBCD ? "B" : "D") << ","
    << formats.varsZeroBased;
  props.set(PropType::Cart_Formats, buf.str());

  buf.str("");
  for (uInt32 p = 0; p < numPlayers; ++p)
  {
    for (uInt32 a = 0; a < numAddrBytes(formats.numDigits, formats.trailingZeroes); ++a)
      buf << std::hex << addresses.scoresAddr[p][a] << ",";
  }
  buf << addresses.varsAddr << ",";
  buf << addresses.playersAddr;
  props.set(PropType::Cart_Addresses, buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 HighScoresManager::numDigits(const Properties& props) const
{
  string digits = getPropIdx(props, PropType::Cart_Formats, 0);

  return digits == EmptyString ? DEFAULT_DIGITS : stoi(digits);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 HighScoresManager::trailingZeroes(const Properties& props) const
{
  string trailing = getPropIdx(props, PropType::Cart_Formats, 1);

  return trailing == EmptyString ? DEFAULT_TRAILING : stoi(trailing);}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HighScoresManager::scoreBCD(const Properties& props) const
{
  string bcd = getPropIdx(props, PropType::Cart_Formats, 2);

  return bcd == EmptyString ? DEFAULT_SCORE_BCD : bcd == "B";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HighScoresManager::varBCD(const Properties& props) const
{
  string bcd = getPropIdx(props, PropType::Cart_Formats, 3);

  return bcd == EmptyString ? DEFAULT_VARS_BCD : bcd == "B";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HighScoresManager::varZeroBased(const Properties& props) const
{
  string zeroBased = getPropIdx(props, PropType::Cart_Formats, 4);

  return zeroBased == EmptyString ? DEFAULT_VARS_ZERO_BASED : zeroBased != "0";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HighScoresManager::playerZeroBased(const Properties& props) const
{
  return DEFAULT_PLAYERS_ZERO_BASED;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 HighScoresManager::playerAddress(const Properties& props) const
{
  uInt32 idx = numAddrBytes(props) * numPlayers(props) + 1;
  string addr = getPropIdx(props, PropType::Cart_Addresses, idx);

  return addr == EmptyString ? DEFAULT_ADDRESS : stoi(addr, nullptr, 16);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 HighScoresManager::varAddress(const Properties& props) const
{
  uInt32 idx = numAddrBytes(props) * numPlayers(props);
  string addr = getPropIdx(props, PropType::Cart_Addresses, idx);

  return addr == EmptyString ? DEFAULT_ADDRESS : stoi(addr, nullptr, 16);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 HighScoresManager::numAddrBytes(Int32 digits, Int32 trailing) const
{
  return (digits - trailing + 1) / 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 HighScoresManager::numAddrBytes(const Properties& props) const
{
  return numAddrBytes(numDigits(props), trailingZeroes(props));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 HighScoresManager::score(uInt32 player, uInt32 numAddrBytes, uInt32 trailingZeroes, bool isBCD,
                               const ScoreAddresses& scoreAddr) const
{
  if (!myOSystem.hasConsole())
    return -1;

  Int32 totalScore = 0;

  for (uInt32 b = 0; b < numAddrBytes; ++b)
  {
    uInt16 addr = scoreAddr[b];
    uInt32 score;

    totalScore *= isBCD ? 100 : 256;
    score = peek(addr);
    if (isBCD)
    {
      // verify if score is legit
      if (score >= 160)
        return -1;

      score = (score >> 4) * 10 + score % 16;
    }
    totalScore += score;
  }

  if (totalScore != -1)
    for (uInt32 i = 0; i < trailingZeroes; ++i)
      totalScore *= 10;

  return totalScore;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 HighScoresManager::player() const
{
  Properties props;
  uInt16 addr = playerAddress(properties(props));

  if (addr == DEFAULT_ADDRESS)
    return DEFAULT_PLAYER;

  return peek(addr) + playerZeroBased(props) ? 1 : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 HighScoresManager::variation() const
{
  Properties props;
  uInt16 addr = varAddress(properties(props));

  if (addr == DEFAULT_ADDRESS)
    return DEFAULT_VARIATION;

  return peek(addr) + varZeroBased(props) ? 1 : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 HighScoresManager::score() const
{
  Properties props;
  uInt32 numBytes = numAddrBytes(properties(props));
  uInt32 currentPlayer = player() - playerZeroBased(props) ? 1 : 0;
  uInt32 idx = numBytes * currentPlayer;
  ScoreAddresses scoreAddr;

  for (uInt32 b = 0; b < numBytes; ++b)
  {
    string addr = getPropIdx(props, PropType::Cart_Addresses, idx + b);

    if (addr == EmptyString)
      return -1;
    scoreAddr[b] = stoi(addr, nullptr, 16);
  }

  return score(currentPlayer, numBytes, trailingZeroes(props), scoreBCD(props), scoreAddr);
}
