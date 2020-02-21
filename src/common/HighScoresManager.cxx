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
   4,             ; score digits
   0,             ; trailing zeroes
   B,             ; score format (BCD, HEX)


   0,             ; invert score order

   B,             ; variation format (BCD, HEX)
   0,             ; zero-based variation
   "",            ; special label (5 chars)
   B,             ; special format (BCD, HEX)
   0,             ; zero-based special
 Addresses (in hex):
   n-times xx,    ; score info, high to low
   xx,            ; variation address (if more than 1 variation)
   xx             ; special address (if defined)

 TODO:
 - variation bits (Centipede)
 - score swaps (Asteroids)
 - special: one optional and named value extra per game (round, level...)
*/

#include <cmath>

#include "OSystem.hxx"
#include "PropsSet.hxx"
#include "Console.hxx"
#include "Launcher.hxx"
#include "System.hxx"

#include "HighScoresManager.hxx"

using namespace BSPF;
using namespace std;
using namespace HSM;

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

  replace(property.begin(), property.end(), ',', ' ');
  replace(property.begin(), property.end(), '|', ' ');
  istringstream buf(property);
  string result;

  for (uInt32 i = 0; i <= idx; ++i)
    if(!(buf >> result))
      return "";

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HighScoresManager::enabled() const
{
  Properties props;

  return (!getPropIdx(properties(props), PropType::Cart_Addresses, 0).empty());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 HighScoresManager::numVariations(const Properties& props) const
{
  string numVariations = getPropIdx(props, PropType::Cart_Variations);

  return min(uInt32(stringToInt(numVariations, DEFAULT_VARIATION)), MAX_VARIATIONS);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HighScoresManager::get(const Properties& props, uInt32& numVariationsR,
                            ScoresInfo& info) const
{
  numVariationsR = numVariations(props);

  //info.armRAM = armRAM(props);
  info.numDigits = numDigits(props);
  info.trailingZeroes = trailingZeroes(props);
  info.scoreBCD = scoreBCD(props);
  info.scoreInvert = scoreInvert(props);
  info.varsBCD = varBCD(props);
  info.varsZeroBased = varZeroBased(props);
  info.special = specialLabel(props);
  info.specialBCD = specialBCD(props);
  info.specialZeroBased = specialZeroBased(props);

  info.varsAddr = varAddress(props);
  info.specialAddr = specialAddress(props);

  for (uInt32 a = 0; a < numAddrBytes(props); ++a)
  {
    string addr = getPropIdx(props, PropType::Cart_Addresses, a);

    info.scoreAddr[a] = stringToIntBase16(addr);
  }

  return (!getPropIdx(props, PropType::Cart_Addresses, 0).empty());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresManager::set(Properties& props, uInt32 numVariations,
                            const ScoresInfo& info) const
{
  ostringstream buf;
  string output;

  props.set(PropType::Cart_Variations, to_string(min(numVariations, MAX_VARIATIONS)));

  // fill from the back to skip default values
  if (output.length() || info.specialZeroBased != DEFAULT_SPECIAL_ZERO_BASED)
    output.insert(0, info.specialZeroBased ? ",1" : ",0");
  if (output.length() || info.specialBCD != DEFAULT_SPECIAL_BCD)
    output.insert(0, info.specialBCD ? ",B" : ",D");
  if (output.length() || !info.special.empty())
    output.insert(0, "," + (info.special.empty() ? "-" : info.special));

  if (output.length() || info.varsZeroBased != DEFAULT_VARS_ZERO_BASED)
    output.insert(0, info.varsZeroBased ? ",1" : ",0");
  if (output.length() || info.varsBCD != DEFAULT_VARS_BCD)
    output.insert(0, info.varsBCD ? ",B" : ",D");

  if (output.length() || info.scoreInvert != DEFAULT_SCORE_REVERSED)
    output.insert(0, info.scoreInvert ? ",1" : ",0");
  if (output.length() || info.scoreBCD != DEFAULT_SCORE_BCD)
    output.insert(0, info.scoreBCD ? ",B" : ",H");
  if (output.length() || info.trailingZeroes != DEFAULT_TRAILING)
    output.insert(0, "," + to_string(info.trailingZeroes));
  if (output.length() || info.numDigits != DEFAULT_DIGITS)
    output.insert(0, to_string(info.numDigits));
  //if (output.length() || info.armRAM != DEFAULT_ARM_RAM)
  //  output.insert(0, info.armRAM ? "1" : "0"); // TODO add ',' to numDigits!

  props.set(PropType::Cart_Formats, output);

  for (uInt32 a = 0; a < numAddrBytes(info.numDigits, info.trailingZeroes); ++a)
    buf << hex << info.scoreAddr[a] << ",";

  // add optional addresses
  if (numVariations != DEFAULT_VARIATION || !info.special.empty())
    buf << info.varsAddr << "," ;
  if (!info.special.empty())
    buf << info.specialAddr << "," ;

  output = buf.str();
  output.pop_back();
  props.set(PropType::Cart_Addresses, output);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 HighScoresManager::numDigits(const Properties& props) const
{
  string digits = getPropIdx(props, PropType::Cart_Formats, IDX_SCORE_DIGITS);

  return min(uInt32(stringToInt(digits, DEFAULT_DIGITS)), MAX_SCORE_DIGITS);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 HighScoresManager::trailingZeroes(const Properties& props) const
{
  string trailing = getPropIdx(props, PropType::Cart_Formats, IDX_TRAILING_ZEROES);

  return min(uInt32(stringToInt(trailing, DEFAULT_TRAILING)), MAX_TRAILING);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HighScoresManager::scoreBCD(const Properties& props) const
{
  string bcd = getPropIdx(props, PropType::Cart_Formats, IDX_SCORE_BCD);

  return bcd.empty() ? DEFAULT_SCORE_BCD : bcd == "B";
}

bool HighScoresManager::scoreInvert(const Properties& props) const
{
  string reversed = getPropIdx(props, PropType::Cart_Formats, IDX_SCORE_INVERT);

  return reversed.empty() ? DEFAULT_SCORE_REVERSED : reversed != "0";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HighScoresManager::varBCD(const Properties& props) const
{
  string bcd = getPropIdx(props, PropType::Cart_Formats, IDX_VAR_BCD);

  return bcd.empty() ? DEFAULT_VARS_BCD : bcd == "B";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HighScoresManager::varZeroBased(const Properties& props) const
{
  string zeroBased = getPropIdx(props, PropType::Cart_Formats, IDX_VAR_ZERO_BASED);

  return zeroBased.empty() ? DEFAULT_VARS_ZERO_BASED : zeroBased != "0";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string HighScoresManager::specialLabel(const Properties& props) const
{
  string orgLabel, label;

  // some ugly formatting
  orgLabel = label = getPropIdx(props, PropType::Cart_Formats, IDX_SPECIAL_LABEL);
  label = BSPF::toLowerCase(label);
  label[0] = orgLabel[0];

  return label;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HighScoresManager::specialBCD(const Properties& props) const
{
  string bcd = getPropIdx(props, PropType::Cart_Formats, IDX_SPECIAL_BCD);

  return bcd.empty() ? DEFAULT_SPECIAL_BCD : bcd == "B";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HighScoresManager::specialZeroBased(const Properties& props) const
{
  string zeroBased = getPropIdx(props, PropType::Cart_Formats, IDX_SPECIAL_ZERO_BASED);

  return zeroBased.empty() ? DEFAULT_SPECIAL_ZERO_BASED : zeroBased != "0";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*bool HighScoresManager::armRAM(const Properties& props) const
{
  //string armRAM = getPropIdx(props, PropType::Cart_Formats, IDX_ARM_RAM);

  //return armRAM.empty() ? DEFAULT_ARM_RAM : armRAM != "0";
  return false;
}*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 HighScoresManager::varAddress(const Properties& props) const
{
  uInt32 idx = numAddrBytes(props) + IDX_VARS_ADDRESS;
  string addr = getPropIdx(props, PropType::Cart_Addresses, idx);

  return stringToIntBase16(addr, DEFAULT_ADDRESS);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 HighScoresManager::specialAddress(const Properties& props) const
{
  uInt32 idx = numAddrBytes(props) + IDX_SPECIAL_ADDRESS;
  string addr = getPropIdx(props, PropType::Cart_Addresses, idx);

  return stringToIntBase16(addr, DEFAULT_ADDRESS);
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
Int32 HighScoresManager::numVariations() const
{
  Properties props;
  uInt16 vars = numVariations(properties(props));

  return vars;;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string HighScoresManager::specialLabel() const
{
  Properties props;

  return specialLabel(properties(props));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 HighScoresManager::variation(uInt16 addr, bool varBCD, bool zeroBased,
                                   uInt32 numVariations) const
{
  if (!myOSystem.hasConsole())
    return DEFAULT_VARIATION;

  Int32 var = peek(addr);

  return convert(var, numVariations, varBCD, zeroBased);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 HighScoresManager::variation() const
{
  Properties props;
  uInt16 addr = varAddress(properties(props));

  if (addr == DEFAULT_ADDRESS)
    return DEFAULT_VARIATION;

  return variation(addr, varBCD(props), varZeroBased(props), numVariations(props));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 HighScoresManager::score(uInt32 numAddrBytes, uInt32 trailingZeroes,
                               bool isBCD, const ScoreAddresses& scoreAddr) const
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
      score = fromBCD(score);
      // verify if score is legit
      if (score == -1)
        return -1;
    }
    totalScore += score;
  }

  if (totalScore != -1)
    for (uInt32 i = 0; i < trailingZeroes; ++i)
      totalScore *= 10;

  return totalScore;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 HighScoresManager::score() const
{
  Properties props;
  uInt32 numBytes = numAddrBytes(properties(props));
  ScoreAddresses scoreAddr;

  for (uInt32 b = 0; b < numBytes; ++b)
  {
    string addr = getPropIdx(props, PropType::Cart_Addresses, b);

    if (addr.empty())
      return -1;
    scoreAddr[b] = stringToIntBase16(addr);
  }

  return score(numBytes, trailingZeroes(props), scoreBCD(props), scoreAddr);
}

bool HighScoresManager::scoreInvert() const
{
  Properties props;
  return scoreInvert(properties(props));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 HighScoresManager::special() const
{
  Properties props;
  uInt16 addr = specialAddress(properties(props));

  if (addr == DEFAULT_ADDRESS)
    return -1;

  return special(addr, specialBCD(props), specialZeroBased(props));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 HighScoresManager::special(uInt16 addr, bool varBCD, bool zeroBased) const
{
  if (!myOSystem.hasConsole())
    return -1;

  Int32 var = peek(addr);

  if (varBCD)
    var = fromBCD(var);

  var += zeroBased ? 1 : 0;

  return var;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 HighScoresManager::convert(uInt32 val, uInt32 maxVal, bool isBCD, bool zeroBased) const
{
  maxVal += zeroBased ? 0 : 1;
  Int32 bits = isBCD
    ? ceil(log(maxVal) / log(10) * 4)
    : ceil(log(maxVal) / log(2));

  if (isBCD)
    val = fromBCD(val);

  // limit to maxVal's bits
  val %= 1 << bits;
  val += zeroBased ? 1 : 0;

  return val;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 HighScoresManager::fromBCD(uInt8 bcd) const
{
  // verify if score is legit
  if ((bcd & 0xF0) >= 0xA0 || (bcd & 0xF) >= 0xA)
    return -1;

  return (bcd >> 4) * 10 + bcd % 16;
}
