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

    // Default everything to '?'
    for(int i = 0; i < 256; ++i)
      lut[i] = '?';

    // ASCII passthrough
    for(int i = 0; i < 128; ++i)
      lut[i] = static_cast<char>(i);

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
    for(auto& c : lut) c = '?';

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

  static constexpr char foldExtended(uInt32 cp) noexcept
  {
    if(cp < 0x0100 || cp > 0x024F) return '?';
    return extLUT[cp - 0x0100];
  }
};

#endif  // ASCII_FOLD_HXX
