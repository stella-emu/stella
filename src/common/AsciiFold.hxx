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

#ifndef ASCII_FOLD_HXX
#define ASCII_FOLD_HXX

#include "bspf.hxx"

class AsciiFold
{
public:
  /**
   * Convert UTF-8 string to ASCII-like string.
   * - ASCII characters pass through unchanged
   * - Latin-1 accents are folded (é → e, ö → o, etc.)
   * - Latin Extended-A/B (U+0100–U+024F) accents are folded where possible
   * - Everything else becomes '?'
   */
  static string toAscii(string_view input)
  {
    string out;
    out.reserve(input.size());

    const unsigned char* s =
      reinterpret_cast<const unsigned char*>(input.data());
    const unsigned char* const end = s + input.size();

    while(s < end)
    {
      uInt32 cp = *s++;

      // Fast ASCII path
      if(cp < 0x80)
      {
        out.push_back(static_cast<char>(cp));
        continue;
      }

      // Minimal UTF-8 decoding (assumes valid input)
      if((cp >> 5) == 0x6)               // 2-byte
      {
        if(s >= end) break;
        const uInt32 b1 = *s++;
        cp = ((cp & 0x1F) << 6) | (b1 & 0x3F);
      }
      else if((cp >> 4) == 0xE)          // 3-byte
      {
        if(end - s < 2) break;
        const uInt32 b1 = *s++;
        const uInt32 b2 = *s++;
        cp = ((cp & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
      }
      else if((cp >> 3) == 0x1E)         // 4-byte
      {
        if(end - s < 3) break;
        const uInt32 b1 = *s++;
        const uInt32 b2 = *s++;
        const uInt32 b3 = *s++;
        cp = ((cp & 0x07) << 18) | ((b1 & 0x3F) << 12) |
             ((b2 & 0x3F) << 6)  |  (b3 & 0x3F);
      }

      // Branch-minimized folding
      out.push_back(cp <= 0xFF ? LUT[cp] : foldExtended(cp));
    }
    return out;
  }

private:
  /**
   * Compile-time ASCII folding lookup table (Latin-1 range).
   */
  static constexpr array<char, 256> LUT = [] {
    array<char, 256> lut{};

    // ASCII passthrough
    for(int i = 0; i < 128; ++i)
      lut[i] = static_cast<char>(i);

    // Default everything else to '?'
    for(int i = 128; i < 256; ++i)
      lut[i] = '?';

    // --- Uppercase Latin-1 accents ---
    lut[0xC0] = lut[0xC1] = lut[0xC2] = lut[0xC3] =
    lut[0xC4] = lut[0xC5] = 'A';
    lut[0xC6] = 'A'; // Æ → A (approx)
    lut[0xC7] = 'C';
    lut[0xC8] = lut[0xC9] = lut[0xCA] = lut[0xCB] = 'E';
    lut[0xCC] = lut[0xCD] = lut[0xCE] = lut[0xCF] = 'I';
    lut[0xD0] = 'D'; // Ð
    lut[0xD1] = 'N';
    lut[0xD2] = lut[0xD3] = lut[0xD4] = lut[0xD5] =
    lut[0xD6] = 'O';
    lut[0xD8] = 'O'; // Ø
    lut[0xD9] = lut[0xDA] = lut[0xDB] = lut[0xDC] = 'U';
    lut[0xDD] = 'Y';

    // --- Lowercase Latin-1 accents ---
    lut[0xE0] = lut[0xE1] = lut[0xE2] = lut[0xE3] =
    lut[0xE4] = lut[0xE5] = 'a';
    lut[0xE6] = 'a'; // æ
    lut[0xE7] = 'c';
    lut[0xE8] = lut[0xE9] = lut[0xEA] = lut[0xEB] = 'e';
    lut[0xEC] = lut[0xED] = lut[0xEE] = lut[0xEF] = 'i';
    lut[0xF0] = 'd'; // ð
    lut[0xF1] = 'n';
    lut[0xF2] = lut[0xF3] = lut[0xF4] = lut[0xF5] =
    lut[0xF6] = 'o';
    lut[0xF8] = 'o'; // ø
    lut[0xF9] = lut[0xFA] = lut[0xFB] = lut[0xFC] = 'u';
    lut[0xFD] = lut[0xFF] = 'y';

    // --- Special cases ---
    lut[0xDF] = 's'; // ß → s (fast approximation)

    return lut;
  }();

  /**
   * Compile-time folding table for Latin Extended-A/B (U+0100–U+024F).
   * Indexed by (cp - 0x0100). Unrecognised codepoints default to '?'.
   */
  static constexpr array<char, 0x0150> extLUT = [] {
    array<char, 0x0150> lut{};

    // Default everything to '?'
    for(auto& c: lut) c = '?';

    // --- Latin Extended-A (U+0100–U+017F) ---
    lut[0x0100 - 0x0100] = 'A'; lut[0x0101 - 0x0100] = 'a'; // Āā
    lut[0x0102 - 0x0100] = 'A'; lut[0x0103 - 0x0100] = 'a'; // Ăă
    lut[0x0104 - 0x0100] = 'A'; lut[0x0105 - 0x0100] = 'a'; // Ąą
    lut[0x0106 - 0x0100] = 'C'; lut[0x0107 - 0x0100] = 'c'; // Ćć
    lut[0x0108 - 0x0100] = 'C'; lut[0x0109 - 0x0100] = 'c'; // Ĉĉ
    lut[0x010A - 0x0100] = 'C'; lut[0x010B - 0x0100] = 'c'; // Ċċ
    lut[0x010C - 0x0100] = 'C'; lut[0x010D - 0x0100] = 'c'; // Čč
    lut[0x010E - 0x0100] = 'D'; lut[0x010F - 0x0100] = 'd'; // Ďď
    lut[0x0110 - 0x0100] = 'D'; lut[0x0111 - 0x0100] = 'd'; // Đđ
    lut[0x0112 - 0x0100] = 'E'; lut[0x0113 - 0x0100] = 'e'; // Ēē
    lut[0x0114 - 0x0100] = 'E'; lut[0x0115 - 0x0100] = 'e'; // Ĕĕ
    lut[0x0116 - 0x0100] = 'E'; lut[0x0117 - 0x0100] = 'e'; // Ėė
    lut[0x0118 - 0x0100] = 'E'; lut[0x0119 - 0x0100] = 'e'; // Ęę
    lut[0x011A - 0x0100] = 'E'; lut[0x011B - 0x0100] = 'e'; // Ěě
    lut[0x011C - 0x0100] = 'G'; lut[0x011D - 0x0100] = 'g'; // Ĝĝ
    lut[0x011E - 0x0100] = 'G'; lut[0x011F - 0x0100] = 'g'; // Ğğ
    lut[0x0120 - 0x0100] = 'G'; lut[0x0121 - 0x0100] = 'g'; // Ġġ
    lut[0x0122 - 0x0100] = 'G'; lut[0x0123 - 0x0100] = 'g'; // Ģģ
    lut[0x0124 - 0x0100] = 'H'; lut[0x0125 - 0x0100] = 'h'; // Ĥĥ
    lut[0x0126 - 0x0100] = 'H'; lut[0x0127 - 0x0100] = 'h'; // Ħħ
    lut[0x0128 - 0x0100] = 'I'; lut[0x0129 - 0x0100] = 'i'; // Ĩĩ
    lut[0x012A - 0x0100] = 'I'; lut[0x012B - 0x0100] = 'i'; // Īī
    lut[0x012C - 0x0100] = 'I'; lut[0x012D - 0x0100] = 'i'; // Ĭĭ
    lut[0x012E - 0x0100] = 'I'; lut[0x012F - 0x0100] = 'i'; // Įį
    lut[0x0130 - 0x0100] = 'I'; lut[0x0131 - 0x0100] = 'i'; // İı
    lut[0x0132 - 0x0100] = 'I'; lut[0x0133 - 0x0100] = 'i'; // IJij
    lut[0x0134 - 0x0100] = 'J'; lut[0x0135 - 0x0100] = 'j'; // Ĵĵ
    lut[0x0136 - 0x0100] = 'K'; lut[0x0137 - 0x0100] = 'k'; // Ķķ
    lut[0x0138 - 0x0100] = 'k';                             // ĸ (lone)
    lut[0x0139 - 0x0100] = 'L'; lut[0x013A - 0x0100] = 'l'; // Ĺĺ
    lut[0x013B - 0x0100] = 'L'; lut[0x013C - 0x0100] = 'l'; // Ļļ
    lut[0x013D - 0x0100] = 'L'; lut[0x013E - 0x0100] = 'l'; // Ľľ
    lut[0x013F - 0x0100] = 'L'; lut[0x0140 - 0x0100] = 'l'; // Ŀŀ
    lut[0x0141 - 0x0100] = 'L'; lut[0x0142 - 0x0100] = 'l'; // Łł
    lut[0x0143 - 0x0100] = 'N'; lut[0x0144 - 0x0100] = 'n'; // Ńń
    lut[0x0145 - 0x0100] = 'N'; lut[0x0146 - 0x0100] = 'n'; // Ņņ
    lut[0x0147 - 0x0100] = 'N'; lut[0x0148 - 0x0100] = 'n'; // Ňň
    lut[0x0149 - 0x0100] = 'n';                             // ŉ (lone)
    lut[0x014A - 0x0100] = 'N'; lut[0x014B - 0x0100] = 'n'; // Ŋŋ
    lut[0x014C - 0x0100] = 'O'; lut[0x014D - 0x0100] = 'o'; // Ōō
    lut[0x014E - 0x0100] = 'O'; lut[0x014F - 0x0100] = 'o'; // Ŏŏ
    lut[0x0150 - 0x0100] = 'O'; lut[0x0151 - 0x0100] = 'o'; // Őő
    lut[0x0152 - 0x0100] = 'O'; lut[0x0153 - 0x0100] = 'o'; // OEoe
    lut[0x0154 - 0x0100] = 'R'; lut[0x0155 - 0x0100] = 'r'; // Ŕŕ
    lut[0x0156 - 0x0100] = 'R'; lut[0x0157 - 0x0100] = 'r'; // Ŗŗ
    lut[0x0158 - 0x0100] = 'R'; lut[0x0159 - 0x0100] = 'r'; // Řř
    lut[0x015A - 0x0100] = 'S'; lut[0x015B - 0x0100] = 's'; // Śś
    lut[0x015C - 0x0100] = 'S'; lut[0x015D - 0x0100] = 's'; // Ŝŝ
    lut[0x015E - 0x0100] = 'S'; lut[0x015F - 0x0100] = 's'; // Şş
    lut[0x0160 - 0x0100] = 'S'; lut[0x0161 - 0x0100] = 's'; // Šš
    lut[0x0162 - 0x0100] = 'T'; lut[0x0163 - 0x0100] = 't'; // Ţţ
    lut[0x0164 - 0x0100] = 'T'; lut[0x0165 - 0x0100] = 't'; // Ťť
    lut[0x0166 - 0x0100] = 'T'; lut[0x0167 - 0x0100] = 't'; // Ŧŧ
    lut[0x0168 - 0x0100] = 'U'; lut[0x0169 - 0x0100] = 'u'; // Ũũ
    lut[0x016A - 0x0100] = 'U'; lut[0x016B - 0x0100] = 'u'; // Ūū
    lut[0x016C - 0x0100] = 'U'; lut[0x016D - 0x0100] = 'u'; // Ŭŭ
    lut[0x016E - 0x0100] = 'U'; lut[0x016F - 0x0100] = 'u'; // Ůů
    lut[0x0170 - 0x0100] = 'U'; lut[0x0171 - 0x0100] = 'u'; // Űű
    lut[0x0172 - 0x0100] = 'U'; lut[0x0173 - 0x0100] = 'u'; // Ųų
    lut[0x0174 - 0x0100] = 'W'; lut[0x0175 - 0x0100] = 'w'; // Ŵŵ
    lut[0x0176 - 0x0100] = 'Y'; lut[0x0177 - 0x0100] = 'y'; // Ŷŷ
    lut[0x0178 - 0x0100] = 'Y';                             // Ÿ (lone uppercase)
    lut[0x0179 - 0x0100] = 'Z'; lut[0x017A - 0x0100] = 'z'; // Źź
    lut[0x017B - 0x0100] = 'Z'; lut[0x017C - 0x0100] = 'z'; // Żż
    lut[0x017D - 0x0100] = 'Z'; lut[0x017E - 0x0100] = 'z'; // Žž
    lut[0x017F - 0x0100] = 's';                             // ſ long s

    // --- Latin Extended-B (U+0180–U+024F), common cases only ---
    lut[0x0180 - 0x0100] = 'b';
    lut[0x0181 - 0x0100] = 'B'; lut[0x0182 - 0x0100] = 'B'; lut[0x0183 - 0x0100] = 'b';
    lut[0x0189 - 0x0100] = 'D'; lut[0x018A - 0x0100] = 'D'; lut[0x018B - 0x0100] = 'D'; lut[0x018C - 0x0100] = 'd';
    lut[0x0191 - 0x0100] = 'F'; lut[0x0192 - 0x0100] = 'f'; // Ƒƒ
    lut[0x0197 - 0x0100] = 'I';
    lut[0x0198 - 0x0100] = 'K'; lut[0x0199 - 0x0100] = 'k';
    lut[0x019A - 0x0100] = 'l'; lut[0x019D - 0x0100] = 'N'; lut[0x019E - 0x0100] = 'n';
    lut[0x019F - 0x0100] = 'O'; lut[0x01A0 - 0x0100] = 'O'; lut[0x01A1 - 0x0100] = 'o';
    lut[0x01A2 - 0x0100] = 'O'; lut[0x01A3 - 0x0100] = 'o';
    lut[0x01A4 - 0x0100] = 'P'; lut[0x01A5 - 0x0100] = 'p';
    lut[0x01AB - 0x0100] = 't';
    lut[0x01AC - 0x0100] = 'T'; lut[0x01AD - 0x0100] = 't';
    lut[0x01AE - 0x0100] = 'T';
    lut[0x01AF - 0x0100] = 'U'; lut[0x01B0 - 0x0100] = 'u';
    lut[0x01B2 - 0x0100] = 'V';
    lut[0x01B3 - 0x0100] = 'Y'; lut[0x01B4 - 0x0100] = 'y';
    lut[0x01B5 - 0x0100] = 'Z'; lut[0x01B6 - 0x0100] = 'z';
    lut[0x01C4 - 0x0100] = 'D'; lut[0x01C5 - 0x0100] = 'D'; lut[0x01C6 - 0x0100] = 'd'; // DŽ variants
    lut[0x01C7 - 0x0100] = 'L'; lut[0x01C8 - 0x0100] = 'L'; lut[0x01C9 - 0x0100] = 'l'; // LJ variants
    lut[0x01CA - 0x0100] = 'N'; lut[0x01CB - 0x0100] = 'N'; lut[0x01CC - 0x0100] = 'n'; // NJ variants
    lut[0x01CD - 0x0100] = 'A'; lut[0x01CE - 0x0100] = 'a'; // Ǎǎ
    lut[0x01CF - 0x0100] = 'I'; lut[0x01D0 - 0x0100] = 'i'; // Ǐǐ
    lut[0x01D1 - 0x0100] = 'O'; lut[0x01D2 - 0x0100] = 'o'; // Ǒǒ
    lut[0x01D3 - 0x0100] = 'U'; lut[0x01D4 - 0x0100] = 'u'; // Ǔǔ
    lut[0x01D5 - 0x0100] = 'U'; lut[0x01D6 - 0x0100] = 'u'; // Ǖǖ
    lut[0x01D7 - 0x0100] = 'U'; lut[0x01D8 - 0x0100] = 'u'; // Ǘǘ
    lut[0x01D9 - 0x0100] = 'U'; lut[0x01DA - 0x0100] = 'u'; // Ǚǚ
    lut[0x01DB - 0x0100] = 'U'; lut[0x01DC - 0x0100] = 'u'; // Ǜǜ
    lut[0x01DE - 0x0100] = 'A'; lut[0x01DF - 0x0100] = 'a';
    lut[0x01E0 - 0x0100] = 'A'; lut[0x01E1 - 0x0100] = 'a';
    lut[0x01E2 - 0x0100] = 'A'; lut[0x01E3 - 0x0100] = 'a';
    lut[0x01E4 - 0x0100] = 'G'; lut[0x01E5 - 0x0100] = 'g';
    lut[0x01E6 - 0x0100] = 'G'; lut[0x01E7 - 0x0100] = 'g';
    lut[0x01E8 - 0x0100] = 'K'; lut[0x01E9 - 0x0100] = 'k';
    lut[0x01EA - 0x0100] = 'O'; lut[0x01EB - 0x0100] = 'o';
    lut[0x01EC - 0x0100] = 'O'; lut[0x01ED - 0x0100] = 'o';
    lut[0x01F0 - 0x0100] = 'j';
    lut[0x01F1 - 0x0100] = 'D'; lut[0x01F2 - 0x0100] = 'D'; lut[0x01F3 - 0x0100] = 'd'; // DZ variants
    lut[0x01F4 - 0x0100] = 'G'; lut[0x01F5 - 0x0100] = 'g';
    lut[0x01F8 - 0x0100] = 'N'; lut[0x01F9 - 0x0100] = 'n';
    lut[0x01FA - 0x0100] = 'A'; lut[0x01FB - 0x0100] = 'a';
    lut[0x01FC - 0x0100] = 'A'; lut[0x01FD - 0x0100] = 'a';
    lut[0x01FE - 0x0100] = 'O'; lut[0x01FF - 0x0100] = 'o';
    lut[0x0200 - 0x0100] = 'A'; lut[0x0201 - 0x0100] = 'a';
    lut[0x0202 - 0x0100] = 'A'; lut[0x0203 - 0x0100] = 'a';
    lut[0x0204 - 0x0100] = 'E'; lut[0x0205 - 0x0100] = 'e';
    lut[0x0206 - 0x0100] = 'E'; lut[0x0207 - 0x0100] = 'e';
    lut[0x0208 - 0x0100] = 'I'; lut[0x0209 - 0x0100] = 'i';
    lut[0x020A - 0x0100] = 'I'; lut[0x020B - 0x0100] = 'i';
    lut[0x020C - 0x0100] = 'O'; lut[0x020D - 0x0100] = 'o';
    lut[0x020E - 0x0100] = 'O'; lut[0x020F - 0x0100] = 'o';
    lut[0x0210 - 0x0100] = 'R'; lut[0x0211 - 0x0100] = 'r';
    lut[0x0212 - 0x0100] = 'R'; lut[0x0213 - 0x0100] = 'r';
    lut[0x0214 - 0x0100] = 'U'; lut[0x0215 - 0x0100] = 'u';
    lut[0x0216 - 0x0100] = 'U'; lut[0x0217 - 0x0100] = 'u';
    lut[0x0218 - 0x0100] = 'S'; lut[0x0219 - 0x0100] = 's';
    lut[0x021A - 0x0100] = 'T'; lut[0x021B - 0x0100] = 't';
    lut[0x021E - 0x0100] = 'H'; lut[0x021F - 0x0100] = 'h';
    lut[0x0220 - 0x0100] = 'N';
    lut[0x0224 - 0x0100] = 'Z'; lut[0x0225 - 0x0100] = 'z';
    lut[0x0226 - 0x0100] = 'A'; lut[0x0227 - 0x0100] = 'a';
    lut[0x0228 - 0x0100] = 'E'; lut[0x0229 - 0x0100] = 'e';
    lut[0x022A - 0x0100] = 'O'; lut[0x022B - 0x0100] = 'o';
    lut[0x022C - 0x0100] = 'O'; lut[0x022D - 0x0100] = 'o';
    lut[0x022E - 0x0100] = 'O'; lut[0x022F - 0x0100] = 'o';
    lut[0x0230 - 0x0100] = 'O'; lut[0x0231 - 0x0100] = 'o';
    lut[0x0232 - 0x0100] = 'Y'; lut[0x0233 - 0x0100] = 'y';

    return lut;
  }();

  /**
   * Compile-time transliteration table for Greek (U+0370–U+03FF).
   * Indexed by (cp - 0x0370). Covers the modern Greek alphabet including
   * accented vowels (tonos/dialytika). Unrecognised codepoints default to '?'.
   */
  static constexpr array<char, 0x0090> greekLUT = [] {
    array<char, 0x0090> lut{};
    for(auto& c: lut) c = '?';

    // Uppercase
    lut[0x0391 - 0x0370] = 'A'; // Α
    lut[0x0392 - 0x0370] = 'B'; // Β
    lut[0x0393 - 0x0370] = 'G'; // Γ
    lut[0x0394 - 0x0370] = 'D'; // Δ
    lut[0x0395 - 0x0370] = 'E'; // Ε
    lut[0x0396 - 0x0370] = 'Z'; // Ζ
    lut[0x0397 - 0x0370] = 'I'; // Η
    lut[0x0398 - 0x0370] = 'T'; // Θ
    lut[0x0399 - 0x0370] = 'I'; // Ι
    lut[0x039A - 0x0370] = 'K'; // Κ
    lut[0x039B - 0x0370] = 'L'; // Λ
    lut[0x039C - 0x0370] = 'M'; // Μ
    lut[0x039D - 0x0370] = 'N'; // Ν
    lut[0x039E - 0x0370] = 'X'; // Ξ
    lut[0x039F - 0x0370] = 'O'; // Ο
    lut[0x03A0 - 0x0370] = 'P'; // Π
    lut[0x03A1 - 0x0370] = 'R'; // Ρ
    lut[0x03A3 - 0x0370] = 'S'; // Σ
    lut[0x03A4 - 0x0370] = 'T'; // Τ
    lut[0x03A5 - 0x0370] = 'Y'; // Υ
    lut[0x03A6 - 0x0370] = 'F'; // Φ
    lut[0x03A7 - 0x0370] = 'X'; // Χ
    lut[0x03A8 - 0x0370] = 'P'; // Ψ
    lut[0x03A9 - 0x0370] = 'O'; // Ω

    // Uppercase with tonos/dialytika (U+0386, U+0388–U+038F)
    lut[0x0386 - 0x0370] = 'A'; // Ά
    lut[0x0388 - 0x0370] = 'E'; // Έ
    lut[0x0389 - 0x0370] = 'I'; // Ή
    lut[0x038A - 0x0370] = 'I'; // Ί
    lut[0x038C - 0x0370] = 'O'; // Ό
    lut[0x038E - 0x0370] = 'Y'; // Ύ
    lut[0x038F - 0x0370] = 'O'; // Ώ

    // Lowercase
    lut[0x03B1 - 0x0370] = 'a'; // α
    lut[0x03B2 - 0x0370] = 'b'; // β
    lut[0x03B3 - 0x0370] = 'g'; // γ
    lut[0x03B4 - 0x0370] = 'd'; // δ
    lut[0x03B5 - 0x0370] = 'e'; // ε
    lut[0x03B6 - 0x0370] = 'z'; // ζ
    lut[0x03B7 - 0x0370] = 'i'; // η
    lut[0x03B8 - 0x0370] = 't'; // θ
    lut[0x03B9 - 0x0370] = 'i'; // ι
    lut[0x03BA - 0x0370] = 'k'; // κ
    lut[0x03BB - 0x0370] = 'l'; // λ
    lut[0x03BC - 0x0370] = 'm'; // μ
    lut[0x03BD - 0x0370] = 'n'; // ν
    lut[0x03BE - 0x0370] = 'x'; // ξ
    lut[0x03BF - 0x0370] = 'o'; // ο
    lut[0x03C0 - 0x0370] = 'p'; // π
    lut[0x03C1 - 0x0370] = 'r'; // ρ
    lut[0x03C2 - 0x0370] = 's'; // ς (final sigma)
    lut[0x03C3 - 0x0370] = 's'; // σ
    lut[0x03C4 - 0x0370] = 't'; // τ
    lut[0x03C5 - 0x0370] = 'y'; // υ
    lut[0x03C6 - 0x0370] = 'f'; // φ
    lut[0x03C7 - 0x0370] = 'x'; // χ
    lut[0x03C8 - 0x0370] = 'p'; // ψ
    lut[0x03C9 - 0x0370] = 'o'; // ω

    // Lowercase with tonos/dialytika (U+03AC–U+03CE)
    lut[0x03AC - 0x0370] = 'a'; // ά
    lut[0x03AD - 0x0370] = 'e'; // έ
    lut[0x03AE - 0x0370] = 'i'; // ή
    lut[0x03AF - 0x0370] = 'i'; // ί
    lut[0x03CC - 0x0370] = 'o'; // ό
    lut[0x03CD - 0x0370] = 'y'; // ύ
    lut[0x03CE - 0x0370] = 'o'; // ώ

    // Lowercase with dialytika and tonos
    lut[0x03CA - 0x0370] = 'i'; // ϊ
    lut[0x03CB - 0x0370] = 'y'; // ϋ

    return lut;
  }();

/**
   * Compile-time transliteration table for Cyrillic (U+0400–U+04FF).
   * Indexed by (cp - 0x0400). Covers Russian, Ukrainian, Bulgarian, and
   * Serbian. Unrecognised codepoints default to '?'.
   */
  static constexpr array<char, 0x0100> cyrillicLUT = [] {
    array<char, 0x0100> lut{};
    for(auto& c: lut) c = '?';

    // Uppercase Russian/Bulgarian/Serbian
    lut[0x0410 - 0x0400] = 'A'; // А
    lut[0x0411 - 0x0400] = 'B'; // Б
    lut[0x0412 - 0x0400] = 'V'; // В
    lut[0x0413 - 0x0400] = 'G'; // Г
    lut[0x0414 - 0x0400] = 'D'; // Д
    lut[0x0415 - 0x0400] = 'E'; // Е
    lut[0x0416 - 0x0400] = 'Z'; // Ж
    lut[0x0417 - 0x0400] = 'Z'; // З
    lut[0x0418 - 0x0400] = 'I'; // И
    lut[0x0419 - 0x0400] = 'Y'; // Й
    lut[0x041A - 0x0400] = 'K'; // К
    lut[0x041B - 0x0400] = 'L'; // Л
    lut[0x041C - 0x0400] = 'M'; // М
    lut[0x041D - 0x0400] = 'N'; // Н
    lut[0x041E - 0x0400] = 'O'; // О
    lut[0x041F - 0x0400] = 'P'; // П
    lut[0x0420 - 0x0400] = 'R'; // Р
    lut[0x0421 - 0x0400] = 'S'; // С
    lut[0x0422 - 0x0400] = 'T'; // Т
    lut[0x0423 - 0x0400] = 'U'; // У
    lut[0x0424 - 0x0400] = 'F'; // Ф
    lut[0x0425 - 0x0400] = 'K'; // Х
    lut[0x0426 - 0x0400] = 'T'; // Ц
    lut[0x0427 - 0x0400] = 'C'; // Ч
    lut[0x0428 - 0x0400] = 'S'; // Ш
    lut[0x0429 - 0x0400] = 'S'; // Щ
    lut[0x042A - 0x0400] = 'S'; // Ъ (hard sign → approx)
    lut[0x042B - 0x0400] = 'Y'; // Ы
    lut[0x042C - 0x0400] = 'S'; // Ь (soft sign → approx)
    lut[0x042D - 0x0400] = 'E'; // Э
    lut[0x042E - 0x0400] = 'Y'; // Ю
    lut[0x042F - 0x0400] = 'Y'; // Я

    // Lowercase Russian/Bulgarian/Serbian
    lut[0x0430 - 0x0400] = 'a'; // а
    lut[0x0431 - 0x0400] = 'b'; // б
    lut[0x0432 - 0x0400] = 'v'; // в
    lut[0x0433 - 0x0400] = 'g'; // г
    lut[0x0434 - 0x0400] = 'd'; // д
    lut[0x0435 - 0x0400] = 'e'; // е
    lut[0x0436 - 0x0400] = 'z'; // ж
    lut[0x0437 - 0x0400] = 'z'; // з
    lut[0x0438 - 0x0400] = 'i'; // и
    lut[0x0439 - 0x0400] = 'y'; // й
    lut[0x043A - 0x0400] = 'k'; // к
    lut[0x043B - 0x0400] = 'l'; // л
    lut[0x043C - 0x0400] = 'm'; // м
    lut[0x043D - 0x0400] = 'n'; // н
    lut[0x043E - 0x0400] = 'o'; // о
    lut[0x043F - 0x0400] = 'p'; // п
    lut[0x0440 - 0x0400] = 'r'; // р
    lut[0x0441 - 0x0400] = 's'; // с
    lut[0x0442 - 0x0400] = 't'; // т
    lut[0x0443 - 0x0400] = 'u'; // у
    lut[0x0444 - 0x0400] = 'f'; // ф
    lut[0x0445 - 0x0400] = 'k'; // х
    lut[0x0446 - 0x0400] = 't'; // ц
    lut[0x0447 - 0x0400] = 'c'; // ч
    lut[0x0448 - 0x0400] = 's'; // ш
    lut[0x0449 - 0x0400] = 's'; // щ
    lut[0x044A - 0x0400] = 's'; // ъ (hard sign → approx)
    lut[0x044B - 0x0400] = 'y'; // ы
    lut[0x044C - 0x0400] = 's'; // ь (soft sign → approx)
    lut[0x044D - 0x0400] = 'e'; // э
    lut[0x044E - 0x0400] = 'y'; // ю
    lut[0x044F - 0x0400] = 'y'; // я

    // Ukrainian extras
    lut[0x0400 - 0x0400] = 'I'; // Ѐ (Е with grave)
    lut[0x0401 - 0x0400] = 'Y'; // Ё
    lut[0x0404 - 0x0400] = 'E'; // Є
    lut[0x0406 - 0x0400] = 'I'; // І
    lut[0x0407 - 0x0400] = 'I'; // Ї
    lut[0x0408 - 0x0400] = 'J'; // Ј
    lut[0x0450 - 0x0400] = 'i'; // ѐ (е with grave)
    lut[0x0451 - 0x0400] = 'y'; // ё
    lut[0x0454 - 0x0400] = 'e'; // є
    lut[0x0456 - 0x0400] = 'i'; // і
    lut[0x0457 - 0x0400] = 'i'; // ї
    lut[0x0458 - 0x0400] = 'j'; // ј

    // Serbian extras
    lut[0x0402 - 0x0400] = 'D'; // Ђ
    lut[0x0403 - 0x0400] = 'G'; // Ѓ
    lut[0x0409 - 0x0400] = 'L'; // Љ
    lut[0x040A - 0x0400] = 'N'; // Њ
    lut[0x040B - 0x0400] = 'T'; // Ћ
    lut[0x040C - 0x0400] = 'K'; // Ќ
    lut[0x040F - 0x0400] = 'D'; // Џ
    lut[0x0452 - 0x0400] = 'd'; // ђ
    lut[0x0453 - 0x0400] = 'g'; // ѓ
    lut[0x0459 - 0x0400] = 'l'; // љ
    lut[0x045A - 0x0400] = 'n'; // њ
    lut[0x045B - 0x0400] = 't'; // ћ
    lut[0x045C - 0x0400] = 'k'; // ќ
    lut[0x045F - 0x0400] = 'd'; // џ

    return lut;
  }();

/**
   * Compile-time folding table for Latin Extended Additional (U+1E00–U+1EFF).
   * Indexed by (cp - 0x1E00). Covers precomposed characters used in
   * Vietnamese, Welsh, and academic transliterations of ancient languages.
   * Unrecognised codepoints default to '?'.
   */
  static constexpr array<char, 0x0100> latinExtAddLUT = [] {
    array<char, 0x0100> lut{};
    for(auto& c: lut) c = '?';

    // Ḁḁ A/a with ring below
    lut[0x1E00 - 0x1E00] = 'A'; lut[0x1E01 - 0x1E00] = 'a';
    // Ḃḃ B/b with dot above
    lut[0x1E02 - 0x1E00] = 'B'; lut[0x1E03 - 0x1E00] = 'b';
    // Ḅḅ B/b with dot below
    lut[0x1E04 - 0x1E00] = 'B'; lut[0x1E05 - 0x1E00] = 'b';
    // Ḇḇ B/b with line below
    lut[0x1E06 - 0x1E00] = 'B'; lut[0x1E07 - 0x1E00] = 'b';
    // Ḉḉ C/c with cedilla and acute
    lut[0x1E08 - 0x1E00] = 'C'; lut[0x1E09 - 0x1E00] = 'c';
    // Ḋḋ D/d with dot above
    lut[0x1E0A - 0x1E00] = 'D'; lut[0x1E0B - 0x1E00] = 'd';
    // Ḍḍ D/d with dot below
    lut[0x1E0C - 0x1E00] = 'D'; lut[0x1E0D - 0x1E00] = 'd';
    // Ḏḏ D/d with line below
    lut[0x1E0E - 0x1E00] = 'D'; lut[0x1E0F - 0x1E00] = 'd';
    // Ḑḑ D/d with cedilla
    lut[0x1E10 - 0x1E00] = 'D'; lut[0x1E11 - 0x1E00] = 'd';
    // Ḓḓ D/d with circumflex below
    lut[0x1E12 - 0x1E00] = 'D'; lut[0x1E13 - 0x1E00] = 'd';
    // Ḕḕ E/e with macron and grave
    lut[0x1E14 - 0x1E00] = 'E'; lut[0x1E15 - 0x1E00] = 'e';
    // Ḗḗ E/e with macron and acute
    lut[0x1E16 - 0x1E00] = 'E'; lut[0x1E17 - 0x1E00] = 'e';
    // Ḙḙ E/e with circumflex below
    lut[0x1E18 - 0x1E00] = 'E'; lut[0x1E19 - 0x1E00] = 'e';
    // Ḛḛ E/e with tilde below
    lut[0x1E1A - 0x1E00] = 'E'; lut[0x1E1B - 0x1E00] = 'e';
    // Ḝḝ E/e with cedilla and breve
    lut[0x1E1C - 0x1E00] = 'E'; lut[0x1E1D - 0x1E00] = 'e';
    // Ḟḟ F/f with dot above
    lut[0x1E1E - 0x1E00] = 'F'; lut[0x1E1F - 0x1E00] = 'f';
    // Ḡḡ G/g with macron
    lut[0x1E20 - 0x1E00] = 'G'; lut[0x1E21 - 0x1E00] = 'g';
    // Ḣḣ H/h with dot above
    lut[0x1E22 - 0x1E00] = 'H'; lut[0x1E23 - 0x1E00] = 'h';
    // Ḥḥ H/h with dot below
    lut[0x1E24 - 0x1E00] = 'H'; lut[0x1E25 - 0x1E00] = 'h';
    // Ḧḧ H/h with diaeresis
    lut[0x1E26 - 0x1E00] = 'H'; lut[0x1E27 - 0x1E00] = 'h';
    // Ḩḩ H/h with cedilla
    lut[0x1E28 - 0x1E00] = 'H'; lut[0x1E29 - 0x1E00] = 'h';
    // Ḫḫ H/h with breve below
    lut[0x1E2A - 0x1E00] = 'H'; lut[0x1E2B - 0x1E00] = 'h';
    // Ḭḭ I/i with tilde below
    lut[0x1E2C - 0x1E00] = 'I'; lut[0x1E2D - 0x1E00] = 'i';
    // Ḯḯ I/i with diaeresis and acute
    lut[0x1E2E - 0x1E00] = 'I'; lut[0x1E2F - 0x1E00] = 'i';
    // Ḱḱ K/k with acute
    lut[0x1E30 - 0x1E00] = 'K'; lut[0x1E31 - 0x1E00] = 'k';
    // Ḳḳ K/k with dot below
    lut[0x1E32 - 0x1E00] = 'K'; lut[0x1E33 - 0x1E00] = 'k';
    // Ḵḵ K/k with line below
    lut[0x1E34 - 0x1E00] = 'K'; lut[0x1E35 - 0x1E00] = 'k';
    // Ḷḷ L/l with dot below
    lut[0x1E36 - 0x1E00] = 'L'; lut[0x1E37 - 0x1E00] = 'l';
    // Ḹḹ L/l with dot below and macron
    lut[0x1E38 - 0x1E00] = 'L'; lut[0x1E39 - 0x1E00] = 'l';
    // Ḻḻ L/l with line below
    lut[0x1E3A - 0x1E00] = 'L'; lut[0x1E3B - 0x1E00] = 'l';
    // Ḽḽ L/l with circumflex below
    lut[0x1E3C - 0x1E00] = 'L'; lut[0x1E3D - 0x1E00] = 'l';
    // Ḿḿ M/m with acute
    lut[0x1E3E - 0x1E00] = 'M'; lut[0x1E3F - 0x1E00] = 'm';
    // Ṁṁ M/m with dot above
    lut[0x1E40 - 0x1E00] = 'M'; lut[0x1E41 - 0x1E00] = 'm';
    // Ṃṃ M/m with dot below
    lut[0x1E42 - 0x1E00] = 'M'; lut[0x1E43 - 0x1E00] = 'm';
    // Ṅṅ N/n with dot above
    lut[0x1E44 - 0x1E00] = 'N'; lut[0x1E45 - 0x1E00] = 'n';
    // Ṇṇ N/n with dot below
    lut[0x1E46 - 0x1E00] = 'N'; lut[0x1E47 - 0x1E00] = 'n';
    // Ṉṉ N/n with line below
    lut[0x1E48 - 0x1E00] = 'N'; lut[0x1E49 - 0x1E00] = 'n';
    // Ṋṋ N/n with circumflex below
    lut[0x1E4A - 0x1E00] = 'N'; lut[0x1E4B - 0x1E00] = 'n';
    // Ṍṍ O/o with tilde and acute
    lut[0x1E4C - 0x1E00] = 'O'; lut[0x1E4D - 0x1E00] = 'o';
    // Ṏṏ O/o with tilde and diaeresis
    lut[0x1E4E - 0x1E00] = 'O'; lut[0x1E4F - 0x1E00] = 'o';
    // Ṑṑ O/o with macron and grave
    lut[0x1E50 - 0x1E00] = 'O'; lut[0x1E51 - 0x1E00] = 'o';
    // Ṓṓ O/o with macron and acute
    lut[0x1E52 - 0x1E00] = 'O'; lut[0x1E53 - 0x1E00] = 'o';
    // Ṕṕ P/p with acute
    lut[0x1E54 - 0x1E00] = 'P'; lut[0x1E55 - 0x1E00] = 'p';
    // Ṗṗ P/p with dot above
    lut[0x1E56 - 0x1E00] = 'P'; lut[0x1E57 - 0x1E00] = 'p';
    // Ṙṙ R/r with dot above
    lut[0x1E58 - 0x1E00] = 'R'; lut[0x1E59 - 0x1E00] = 'r';
    // Ṛṛ R/r with dot below
    lut[0x1E5A - 0x1E00] = 'R'; lut[0x1E5B - 0x1E00] = 'r';
    // Ṝṝ R/r with dot below and macron
    lut[0x1E5C - 0x1E00] = 'R'; lut[0x1E5D - 0x1E00] = 'r';
    // Ṟṟ R/r with line below
    lut[0x1E5E - 0x1E00] = 'R'; lut[0x1E5F - 0x1E00] = 'r';
    // Ṡṡ S/s with dot above
    lut[0x1E60 - 0x1E00] = 'S'; lut[0x1E61 - 0x1E00] = 's';
    // Ṣṣ S/s with dot below
    lut[0x1E62 - 0x1E00] = 'S'; lut[0x1E63 - 0x1E00] = 's';
    // Ṥṥ S/s with acute and dot above
    lut[0x1E64 - 0x1E00] = 'S'; lut[0x1E65 - 0x1E00] = 's';
    // Ṧṧ S/s with caron and dot above
    lut[0x1E66 - 0x1E00] = 'S'; lut[0x1E67 - 0x1E00] = 's';
    // Ṩṩ S/s with dot below and dot above
    lut[0x1E68 - 0x1E00] = 'S'; lut[0x1E69 - 0x1E00] = 's';
    // Ṫṫ T/t with dot above
    lut[0x1E6A - 0x1E00] = 'T'; lut[0x1E6B - 0x1E00] = 't';
    // Ṭṭ T/t with dot below
    lut[0x1E6C - 0x1E00] = 'T'; lut[0x1E6D - 0x1E00] = 't';
    // Ṯṯ T/t with line below
    lut[0x1E6E - 0x1E00] = 'T'; lut[0x1E6F - 0x1E00] = 't';
    // Ṱṱ T/t with circumflex below
    lut[0x1E70 - 0x1E00] = 'T'; lut[0x1E71 - 0x1E00] = 't';
    // Ṳṳ U/u with diaeresis below
    lut[0x1E72 - 0x1E00] = 'U'; lut[0x1E73 - 0x1E00] = 'u';
    // Ṵṵ U/u with tilde below
    lut[0x1E74 - 0x1E00] = 'U'; lut[0x1E75 - 0x1E00] = 'u';
    // Ṷṷ U/u with circumflex below
    lut[0x1E76 - 0x1E00] = 'U'; lut[0x1E77 - 0x1E00] = 'u';
    // Ṹṹ U/u with tilde and acute
    lut[0x1E78 - 0x1E00] = 'U'; lut[0x1E79 - 0x1E00] = 'u';
    // Ṻṻ U/u with macron and diaeresis
    lut[0x1E7A - 0x1E00] = 'U'; lut[0x1E7B - 0x1E00] = 'u';
    // Ṽṽ V/v with tilde
    lut[0x1E7C - 0x1E00] = 'V'; lut[0x1E7D - 0x1E00] = 'v';
    // Ṿṿ V/v with dot below
    lut[0x1E7E - 0x1E00] = 'V'; lut[0x1E7F - 0x1E00] = 'v';
    // Ẁẁ W/w with grave
    lut[0x1E80 - 0x1E00] = 'W'; lut[0x1E81 - 0x1E00] = 'w';
    // Ẃẃ W/w with acute
    lut[0x1E82 - 0x1E00] = 'W'; lut[0x1E83 - 0x1E00] = 'w';
    // Ẅẅ W/w with diaeresis
    lut[0x1E84 - 0x1E00] = 'W'; lut[0x1E85 - 0x1E00] = 'w';
    // Ẇẇ W/w with dot above
    lut[0x1E86 - 0x1E00] = 'W'; lut[0x1E87 - 0x1E00] = 'w';
    // Ẉẉ W/w with dot below
    lut[0x1E88 - 0x1E00] = 'W'; lut[0x1E89 - 0x1E00] = 'w';
    // Ẋẋ X/x with dot above
    lut[0x1E8A - 0x1E00] = 'X'; lut[0x1E8B - 0x1E00] = 'x';
    // Ẍẍ X/x with diaeresis
    lut[0x1E8C - 0x1E00] = 'X'; lut[0x1E8D - 0x1E00] = 'x';
    // Ẏẏ Y/y with dot above
    lut[0x1E8E - 0x1E00] = 'Y'; lut[0x1E8F - 0x1E00] = 'y';
    // Ẑẑ Z/z with circumflex
    lut[0x1E90 - 0x1E00] = 'Z'; lut[0x1E91 - 0x1E00] = 'z';
    // Ẓẓ Z/z with dot below
    lut[0x1E92 - 0x1E00] = 'Z'; lut[0x1E93 - 0x1E00] = 'z';
    // Ẕẕ Z/z with line below
    lut[0x1E94 - 0x1E00] = 'Z'; lut[0x1E95 - 0x1E00] = 'z';
    // ẖ h with line below
    lut[0x1E96 - 0x1E00] = 'h';
    // ẗ t with diaeresis
    lut[0x1E97 - 0x1E00] = 't';
    // ẘ w with ring above
    lut[0x1E98 - 0x1E00] = 'w';
    // ẙ y with ring above
    lut[0x1E99 - 0x1E00] = 'y';

    // --- Vietnamese (U+1EA0–U+1EF9) ---
    // Ạạ A/a with dot below
    lut[0x1EA0 - 0x1E00] = 'A'; lut[0x1EA1 - 0x1E00] = 'a';
    // Ảả A/a with hook above
    lut[0x1EA2 - 0x1E00] = 'A'; lut[0x1EA3 - 0x1E00] = 'a';
    // Ấấ A/a with circumflex and acute
    lut[0x1EA4 - 0x1E00] = 'A'; lut[0x1EA5 - 0x1E00] = 'a';
    // Ầầ A/a with circumflex and grave
    lut[0x1EA6 - 0x1E00] = 'A'; lut[0x1EA7 - 0x1E00] = 'a';
    // Ẩẩ A/a with circumflex and hook
    lut[0x1EA8 - 0x1E00] = 'A'; lut[0x1EA9 - 0x1E00] = 'a';
    // Ẫẫ A/a with circumflex and tilde
    lut[0x1EAA - 0x1E00] = 'A'; lut[0x1EAB - 0x1E00] = 'a';
    // Ậậ A/a with circumflex and dot below
    lut[0x1EAC - 0x1E00] = 'A'; lut[0x1EAD - 0x1E00] = 'a';
    // Ắắ A/a with breve and acute
    lut[0x1EAE - 0x1E00] = 'A'; lut[0x1EAF - 0x1E00] = 'a';
    // Ằằ A/a with breve and grave
    lut[0x1EB0 - 0x1E00] = 'A'; lut[0x1EB1 - 0x1E00] = 'a';
    // Ẳẳ A/a with breve and hook
    lut[0x1EB2 - 0x1E00] = 'A'; lut[0x1EB3 - 0x1E00] = 'a';
    // Ẵẵ A/a with breve and tilde
    lut[0x1EB4 - 0x1E00] = 'A'; lut[0x1EB5 - 0x1E00] = 'a';
    // Ặặ A/a with breve and dot below
    lut[0x1EB6 - 0x1E00] = 'A'; lut[0x1EB7 - 0x1E00] = 'a';
    // Ẹẹ E/e with dot below
    lut[0x1EB8 - 0x1E00] = 'E'; lut[0x1EB9 - 0x1E00] = 'e';
    // Ẻẻ E/e with hook above
    lut[0x1EBA - 0x1E00] = 'E'; lut[0x1EBB - 0x1E00] = 'e';
    // Ẽẽ E/e with tilde
    lut[0x1EBC - 0x1E00] = 'E'; lut[0x1EBD - 0x1E00] = 'e';
    // Ếế E/e with circumflex and acute
    lut[0x1EBE - 0x1E00] = 'E'; lut[0x1EBF - 0x1E00] = 'e';
    // Ềề E/e with circumflex and grave
    lut[0x1EC0 - 0x1E00] = 'E'; lut[0x1EC1 - 0x1E00] = 'e';
    // Ểể E/e with circumflex and hook
    lut[0x1EC2 - 0x1E00] = 'E'; lut[0x1EC3 - 0x1E00] = 'e';
    // Ễễ E/e with circumflex and tilde
    lut[0x1EC4 - 0x1E00] = 'E'; lut[0x1EC5 - 0x1E00] = 'e';
    // Ệệ E/e with circumflex and dot below
    lut[0x1EC6 - 0x1E00] = 'E'; lut[0x1EC7 - 0x1E00] = 'e';
    // Ỉỉ I/i with hook above
    lut[0x1EC8 - 0x1E00] = 'I'; lut[0x1EC9 - 0x1E00] = 'i';
    // Ịị I/i with dot below
    lut[0x1ECA - 0x1E00] = 'I'; lut[0x1ECB - 0x1E00] = 'i';
    // Ọọ O/o with dot below
    lut[0x1ECC - 0x1E00] = 'O'; lut[0x1ECD - 0x1E00] = 'o';
    // Ỏỏ O/o with hook above
    lut[0x1ECE - 0x1E00] = 'O'; lut[0x1ECF - 0x1E00] = 'o';
    // Ốố O/o with circumflex and acute
    lut[0x1ED0 - 0x1E00] = 'O'; lut[0x1ED1 - 0x1E00] = 'o';
    // Ồồ O/o with circumflex and grave
    lut[0x1ED2 - 0x1E00] = 'O'; lut[0x1ED3 - 0x1E00] = 'o';
    // Ổổ O/o with circumflex and hook
    lut[0x1ED4 - 0x1E00] = 'O'; lut[0x1ED5 - 0x1E00] = 'o';
    // Ỗỗ O/o with circumflex and tilde
    lut[0x1ED6 - 0x1E00] = 'O'; lut[0x1ED7 - 0x1E00] = 'o';
    // Ộộ O/o with circumflex and dot below
    lut[0x1ED8 - 0x1E00] = 'O'; lut[0x1ED9 - 0x1E00] = 'o';
    // Ớớ O/o with horn and acute
    lut[0x1EDA - 0x1E00] = 'O'; lut[0x1EDB - 0x1E00] = 'o';
    // Ờờ O/o with horn and grave
    lut[0x1EDC - 0x1E00] = 'O'; lut[0x1EDD - 0x1E00] = 'o';
    // Ởở O/o with horn and hook
    lut[0x1EDE - 0x1E00] = 'O'; lut[0x1EDF - 0x1E00] = 'o';
    // Ỡỡ O/o with horn and tilde
    lut[0x1EE0 - 0x1E00] = 'O'; lut[0x1EE1 - 0x1E00] = 'o';
    // Ợợ O/o with horn and dot below
    lut[0x1EE2 - 0x1E00] = 'O'; lut[0x1EE3 - 0x1E00] = 'o';
    // Ụụ U/u with dot below
    lut[0x1EE4 - 0x1E00] = 'U'; lut[0x1EE5 - 0x1E00] = 'u';
    // Ủủ U/u with hook above
    lut[0x1EE6 - 0x1E00] = 'U'; lut[0x1EE7 - 0x1E00] = 'u';
    // Ứứ U/u with horn and acute
    lut[0x1EE8 - 0x1E00] = 'U'; lut[0x1EE9 - 0x1E00] = 'u';
    // Ừừ U/u with horn and grave
    lut[0x1EEA - 0x1E00] = 'U'; lut[0x1EEB - 0x1E00] = 'u';
    // Ửử U/u with horn and hook
    lut[0x1EEC - 0x1E00] = 'U'; lut[0x1EED - 0x1E00] = 'u';
    // Ữữ U/u with horn and tilde
    lut[0x1EEE - 0x1E00] = 'U'; lut[0x1EEF - 0x1E00] = 'u';
    // Ựự U/u with horn and dot below
    lut[0x1EF0 - 0x1E00] = 'U'; lut[0x1EF1 - 0x1E00] = 'u';
    // Ỳỳ Y/y with grave
    lut[0x1EF2 - 0x1E00] = 'Y'; lut[0x1EF3 - 0x1E00] = 'y';
    // Ỵỵ Y/y with dot below
    lut[0x1EF4 - 0x1E00] = 'Y'; lut[0x1EF5 - 0x1E00] = 'y';
    // Ỷỷ Y/y with hook above
    lut[0x1EF6 - 0x1E00] = 'Y'; lut[0x1EF7 - 0x1E00] = 'y';
    // Ỹỹ Y/y with tilde
    lut[0x1EF8 - 0x1E00] = 'Y'; lut[0x1EF9 - 0x1E00] = 'y';

    return lut;
  }();

  static constexpr char foldExtended(uInt32 cp) noexcept
  {
    if(cp <= 0x024F) return extLUT[cp - 0x0100];
    if(cp <= 0x03FF) return greekLUT[cp - 0x0370];
    if(cp <= 0x04FF) return cyrillicLUT[cp - 0x0400];
    if(cp >= 0x1E00 && cp <= 0x1EFF) return latinExtAddLUT[cp - 0x1E00];
    return '?';
  }
};

#endif  // ASCII_FOLD_HXX
