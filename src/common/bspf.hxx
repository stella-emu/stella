//============================================================================
//
//  BBBBB    SSSS   PPPPP   FFFFFF
//  BB  BB  SS  SS  PP  PP  FF
//  BB  BB  SS      PP  PP  FF
//  BBBBB    SSSS   PPPPP   FFFF    --  "Brad's Simple Portability Framework"
//  BB  BB      SS  PP      FF
//  BB  BB  SS  SS  PP      FF
//  BBBBB    SSSS   PP      FF
//
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef BSPF_HXX
#define BSPF_HXX

/**
  This file defines various basic data types and preprocessor variables
  that need to be defined for different operating systems.

  @author Bradford W. Mott and Stephen Anthony
*/

#include <cstdint>
// Types for 8/16/32/64-bit signed and unsigned integers
using Int8   = int8_t;
using uInt8  = uint8_t;
using Int16  = int16_t;
using uInt16 = uint16_t;
using Int32  = int32_t;
using uInt32 = uint32_t;
using Int64  = int64_t;
using uInt64 = uint64_t;

// The following code should provide access to the standard C++ objects and
// types: cout, cerr, string, ostream, istream, etc.
#include <array>
#include <algorithm>
#include <bit>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <format>
#include <charconv>
#include <sstream>
#include <ctime>
#include <numbers>
#include <ranges>
#include <utility>
#include <vector>

using std::cin;
using std::cout;
using std::cerr;
// Android NDK 26 libc++ lacks ostringstream::view() (C++20 P0408R7)
#if defined(__ANDROID__) && !defined(__cpp_lib_sstream_from_string_view)
class ostringstream : public std::ostringstream {
public:
  using std::ostringstream::ostringstream;
  std::string view() const { return str(); }
};
class stringstream : public std::stringstream {
public:
  using std::stringstream::stringstream;
  std::string view() const { return str(); }
};
#endif

// Frequently used data types
using std::string;
using std::string_view;
using std::unique_ptr;
using std::shared_ptr;
using std::array;
using std::vector;

// Common array types
using IntArray = std::vector<Int32>;
using uIntArray = std::vector<uInt32>;
using BoolArray = std::vector<bool>;
using ByteArray = std::vector<uInt8>;
using ShortArray = std::vector<uInt16>;
using StringList = std::vector<std::string>;
using ByteBuffer = std::unique_ptr<uInt8[]>;
using DWordBuffer = std::unique_ptr<uInt32[]>;

// We use KB a lot; let's make a literal for it
constexpr size_t operator ""_KB(unsigned long long size)
{
  return static_cast<size_t>(size * 1024);
}

// Output contents of a vector
template<typename T>
std::ostream& operator<< (std::ostream& out, const std::vector<T>& v) {
  for(const auto& elem: v)
    out << elem << " ";
  return out;
}

// Output contents of a map
template<typename T>
concept MapLike = requires(T m) {
  typename T::key_type;
  typename T::mapped_type;
};
template<MapLike M>
std::ostream& operator<<(std::ostream& out, const M& m) {
  for(const auto& [key, value]: m)
    out << key << ": " << value << '\n';
  return out;
}

// This is so we can return empty string references with creating temporaries
inline const string& EmptyString() { static const string empty; return empty; }

// This is defined by some systems, but Stella has other uses for it
#undef PAGE_SIZE
#undef PAGE_MASK

// Adaptable refresh is currently not available on MacOS
// In the future, this may expand to other systems
#ifndef BSPF_MACOS
  #define ADAPTABLE_REFRESH_SUPPORT
#endif

namespace BSPF
{
  constexpr float  PI_f = std::numbers::pi_v<float>;
  constexpr double PI_d = std::numbers::pi_v<double>;
  constexpr double ln10 = std::numbers::ln10;
  constexpr double ln2  = std::numbers::ln2;

  // CPU architecture type
  // This isn't complete yet, but takes care of all the major platforms
  #if defined(__i386__) || defined(_M_IX86)
    constexpr string_view ARCH = "i386";
  #elif defined(__x86_64__) || defined(_WIN64)
    constexpr string_view ARCH = "x86_64";
  #elif defined(__powerpc__) || defined(__ppc__)
    constexpr string_view ARCH = "ppc";
  #elif defined(__arm__) || defined(__thumb__)
    constexpr string_view ARCH = "arm32";
  #elif defined(__aarch64__)
    constexpr string_view ARCH = "arm64";
  #else
    constexpr string_view ARCH = "NOARCH";
  #endif

  #if defined(BSPF_WINDOWS) || defined(__WIN32__)
    #define FORCE_INLINE __forceinline
  #else
    #define FORCE_INLINE inline __attribute__((always_inline))
  #endif

  // Get next power of two greater than or equal to the given value
  constexpr size_t nextPowerOfTwo(size_t size) {
    return std::bit_ceil(size);
  }

  // Get next multiple of the given value
  // Note that this only works when multiple is a power of two
  constexpr size_t nextMultipleOf(size_t size, size_t multiple) {
    return (size + multiple - 1) & ~(multiple - 1);
  }

  // Make 2D-arrays using std::array less verbose
  template<typename T, size_t ROW, size_t COL>
  using array2D = std::array<std::array<T, COL>, ROW>;

  // Combines 'max' and 'min', and clamps value to the upper/lower value
  // if it is outside the specified range
  template<typename T> constexpr T clamp(T val, T lower, T upper)
  {
    return std::clamp<T>(val, lower, upper);
  }
  template<typename T> constexpr void clamp(T& val, T lower, T upper, T setVal)
  {
    if(val < lower || val > upper)  val = setVal;
  }
  template<typename T> constexpr T clampw(T val, T lower, T upper)
  {
    return (val < lower) ? upper : (val > upper) ? lower : val;
  }

  // Test whether a container contains the given value
  template<typename Container>
  bool contains(const Container& c, typename Container::const_reference elem) {
    return std::ranges::find(c, elem) != c.end();
  }

  // Convert character to upper/lower case (ASCII only)
  constexpr char toUpperAscii(char c) {
    return (c >= 'a' && c <= 'z') ? (c - 'a' + 'A') : c;
  }
  constexpr char toLowerAscii(char c) {
    return (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
  }
  // Test whether a character is upper/ower case (ASCII only)
  constexpr bool isUpperAscii(char c) { return c >= 'A' && c <= 'Z'; }
  constexpr bool isLowerAscii(char c) { return c >= 'a' && c <= 'z'; }

  // Convert string to given case
  inline const string& toUpperCase(string& s)
  {
    std::ranges::transform(s, s.begin(), [](char c){ return toUpperAscii(c); });
    return s;
  }
  inline const string& toLowerCase(string& s)
  {
    std::ranges::transform(s, s.begin(), [](char c){ return toLowerAscii(c); });
    return s;
  }

  // Convert string to integer, using default value on any error
  template<int BASE = 10>
  inline int stoi(string_view s, int defaultValue = 0)
  {
    // Skip leading spaces safely
    const auto pos = s.find_first_not_of(' ');
    if(pos == string_view::npos)
      return defaultValue;
    s.remove_prefix(pos);

    int i{};
    const auto result = std::from_chars(s.data(), s.data() + s.size(), i, BASE);
    return (result.ec == std::errc()) ? i : defaultValue;
  }

  // Compare two strings (case insensitive)
  // Return negative, zero, positive result for <,==,> respectively
  constexpr int compareIgnoreCase(string_view s1, string_view s2)
  {
    // Only compare up to the length of the shorter string
    const auto maxsize = std::min(s1.size(), s2.size());
    for(size_t i = 0; i < maxsize; ++i)
    {
      const char c1 = toUpperAscii(s1[i]);
      const char c2 = toUpperAscii(s2[i]);
      if(c1 != c2)
        return c1 - c2;
    }
    return (s1.size() < s2.size()) ? -1 : (s1.size() > s2.size() ? 1 : 0);
  }

  // Test whether two strings are equal (case insensitive)
  constexpr bool equalsIgnoreCase(string_view s1, string_view s2)
  {
    return s1.size() == s2.size() && compareIgnoreCase(s1, s2) == 0;
  }

  // Test whether the first string starts with the second one (case insensitive)
  constexpr bool startsWithIgnoreCase(string_view s1, string_view s2)
  {
    return s1.size() >= s2.size() &&
           compareIgnoreCase(s1.substr(0, s2.size()), s2) == 0;
  }

  // Test whether the first string ends with the second one (case insensitive)
  constexpr bool endsWithIgnoreCase(string_view s1, string_view s2)
  {
    return s1.size() >= s2.size() &&
           compareIgnoreCase(s1.substr(s1.size() - s2.size()), s2) == 0;
  }

  // Find location (if any) of the second string within the first,
  // starting from 'startpos' in the first string
  // Returns the absolute position in s1, or string_view::npos if not found.
  constexpr size_t findIgnoreCase(string_view s1, string_view s2,
                                  size_t startpos = 0)
  {
    if(startpos > s1.size()) return string_view::npos;
    const auto pos = std::search(s1.begin() + startpos, s1.end(), // NOLINT: issues with auto
                                 s2.begin(), s2.end(),
            [&](char ch1, char ch2) {
              return toUpperAscii(ch1) == toUpperAscii(ch2);
            }
    );
    return pos == s1.end() ? string_view::npos : pos - s1.begin();
  }

  // Test whether the first string contains the second one (case insensitive)
  constexpr bool containsIgnoreCase(string_view s1, string_view s2)
  {
    return findIgnoreCase(s1, s2) != string_view::npos;
  }

  // Test whether the first string matches the second one (case insensitive)
  // - the first character must match
  // - the following characters must appear in the order of the first string
  constexpr bool matchesIgnoreCase(string_view s1, string_view s2)
  {
    if(s2.empty()) return true;
    if(s1.empty()) return false;

    // First character must match
    if(toUpperAscii(s1[0]) != toUpperAscii(s2[0]))
      return false;

    // Remaining characters must appear in order
    size_t pos = 1;
    for(size_t j = 1; j < s2.size(); ++j)
    {
      const char target = toUpperAscii(s2[j]);
      while(pos < s1.size() && toUpperAscii(s1[pos]) != target)
        ++pos;
      if(pos == s1.size())
        return false;
      ++pos;
    }
    return true;
  }

  // Test whether the first string matches the second one
  //  (case sensitive for upper case characters in second string, except first one)
  // - the first character must match
  // - the following characters must appear in the order of the first string
  constexpr bool matchesCamelCase(string_view s1, string_view s2)
  {
    if(s1.empty() || s2.empty()) return false;

    // Skip leading '_' for both strings if both start with it
    const size_t ofs = (s1[0] == '_' && s2[0] == '_') ? 1 : 0;

    // First character must match (case insensitive)
    if(toUpperAscii(s1[ofs]) != toUpperAscii(s2[ofs]))
      return false;

    size_t pos = 1 + ofs;  // current search position in s1 (absolute)

    for(size_t j = 1 + ofs; j < s2.size(); ++j)
    {
      const char c2 = s2[j];

      if(isUpperAscii(c2))
      {
        // Scan forward in s1; fail if we encounter an uppercase char
        // before finding c2 (which must be an exact case match)
        while(pos < s1.size() && s1[pos] != c2)
        {
          if(isUpperAscii(s1[pos]))
            return false;  // skipped an uppercase — not a valid match
          ++pos;
        }
        if(pos == s1.size())
          return false;
      }
      else
      {
        // Case-insensitive scan for a lowercase pattern char;
        // uppercase chars in s1 are not a barrier here
        const char c2u = toUpperAscii(c2);
        while(pos < s1.size() && toUpperAscii(s1[pos]) != c2u)
          ++pos;
        if(pos == s1.size())
          return false;
      }
      ++pos;
    }
    return true;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Search if string contains pattern including '?' as joker.
  // @param str      The searched string
  // @param pattern  The pattern to search for
  // @return  Position of pattern in string.
  constexpr size_t matchWithJoker(string_view str, string_view pattern)
  {
    if(str.length() < pattern.length())
      return string_view::npos;

    // Find the first literal (non-'?') character in the pattern to use as
    // a fast-skip anchor; if none exists every position trivially matches
    size_t anchorPat = 0;
    while(anchorPat < pattern.length() && pattern[anchorPat] == '?')
      ++anchorPat;

    if(anchorPat == pattern.length())
      return 0;  // pattern is all '?', matches at position 0

    const char anchor = pattern[anchorPat];
    const size_t maxPos = str.length() - pattern.length();

    for(size_t pos = 0; pos <= maxPos; )
    {
      // Jump ahead to next occurrence of the anchor character
      const size_t found = str.find(anchor, pos + anchorPat);
      if(found == string_view::npos || found - anchorPat > maxPos)
        return string_view::npos;

      pos = found - anchorPat;

      // Verify the full pattern at this position
      bool match = true;
      for(size_t i = 0; i < pattern.length(); ++i)
      {
        if(pattern[i] != '?' && pattern[i] != str[pos + i])
        {
          match = false;
          break;
        }
      }
      if(match)
        return pos;

      ++pos;
    }
    return string_view::npos;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Search if string contains pattern including wildcard '*'
  // and '?' as joker.
  // @param str      The searched string
  // @param pattern  The pattern to search for
  // @return  True if pattern was found.
  constexpr bool matchWithWildcards(string_view str, string_view pattern)
  {
    size_t si = 0;        // current position in str
    size_t pi = 0;        // current position in pattern
    size_t starPi = string_view::npos;  // position of last '*' in pattern
    size_t starSi = 0;    // position in str when last '*' was matched

    while(si < str.length())
    {
      if(pi < pattern.length() &&
        (pattern[pi] == '?' || pattern[pi] == str[si]))
      {
        // Direct match or joker: advance both
        ++si;
        ++pi;
      }
      else if(pi < pattern.length() && pattern[pi] == '*')
      {
        // '*' matches zero characters for now; remember position to backtrack
        starPi = pi++;
        starSi = si;
      }
      else if(starPi != string_view::npos)
      {
        // Mismatch, but we have a previous '*' to backtrack to;
        // let it consume one more character from str
        pi = starPi + 1;
        si = ++starSi;
      }
      else
        return false;  // Mismatch with no '*' to backtrack to
    }

    // Consume any remaining '*' in pattern (they match empty string)
    while(pi < pattern.length() && pattern[pi] == '*')
      ++pi;

    return pi == pattern.length();
  }

  // Modify 'str', replacing all occurrences of 'from' with 'to'
  constexpr void replaceAll(string& str, string_view from, string_view to)
  {
    if(from.empty()) return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != string_view::npos)
    {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length(); // In case 'to' contains 'from',
                                // like replacing 'x' with 'yx'
    }
  }

  // Trim leading and trailing whitespace from a string
  constexpr string trim(string_view str)
  {
    const auto first = str.find_first_not_of(' ');
    if(first == string_view::npos)
      return {};

    const auto last = str.find_last_not_of(' ');
    return string{str.substr(first, last - first + 1)};
  }

  // C++11 way to get local time
  // Equivalent to the C-style localtime() function, but is thread-safe
  inline std::tm localTime()
  {
    const auto currtime = std::time(nullptr);
    std::tm tm_snapshot{};
  #if (defined BSPF_WINDOWS || defined __WIN32__) && (!defined __GNUG__ || defined __MINGW32__)
    std::ignore = localtime_s(&tm_snapshot, &currtime);
  #else
    std::ignore = localtime_r(&currtime, &tm_snapshot);
  #endif
    return tm_snapshot;
  }

  constexpr bool isWhiteSpace(const char c)
  {
    constexpr string_view spaces{" ,.;:+-*&/\\'"};
    return spaces.find(c) != string_view::npos;
  }
} // namespace BSPF

#endif
