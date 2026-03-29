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

#ifndef VARIANT_HXX
#define VARIANT_HXX

#include <variant>

#include "Rect.hxx"
#include "bspf.hxx"

/**
  This class implements a variant type using std::variant.  Whenever
  possible, it stores the data as the specific type, so no conversion is
  necessary.  Otherwise it stores the data as a string, and converts to
  other types as required, with the results cached whenever possible.

  @author  Stephen Anthony
*/
class Variant
{
  public:
    using Value = std::variant<
      string,
      Int32,
      uInt32,
      float,
      double,
      bool,
      Common::Size,
      Common::Point,
      std::monostate
    >;

    Variant() = default;

    // NOLINTBEGIN: we don't want c'tors to be explicit here, so disable the warning
    // String constructors
    Variant(const string& s) : myValue{s} { }
    Variant(string&& s)      : myValue{std::move(s)} { }
    Variant(string_view s)   : myValue{string{s}} { }
    Variant(const char* s)   : myValue{string{s}} { }

    // Numeric + other constructors
    Variant(Int32 v)                : myValue{v} { }
    Variant(uInt32 v)               : myValue{v} { }
    Variant(float v)                : myValue{v} { }
    Variant(double v)               : myValue{v} { }
    Variant(bool v)                 : myValue{v} { }
    Variant(const Common::Size& v)  : myValue{v} { }
    Variant(const Common::Point& v) : myValue{v} { }
    // NOLINTEND

    // Conversion methods
    const string& toString() const {
      if(!myCachedString) {
        myCachedString = std::visit([](const auto& v) -> string {
          using T = std::decay_t<decltype(v)>;

          if constexpr(std::is_same_v<T, std::monostate>)
            return "";
          else if constexpr(std::is_same_v<T, string>)
            return v;
          else if constexpr(std::is_same_v<T, bool>)
            return v ? "1" : "0";
          else if constexpr(std::is_arithmetic_v<T>) {
            char buf[32];
            auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), v);
            return string(buf, ptr - buf);
          }
          else
            return v.toString(); // safe
        }, myValue);
      }
      return *myCachedString;
    }

    const char* toCString() const { return toString().c_str(); }

    Int32 toInt() const {
      if(!myCachedInt) {
        myCachedInt = std::visit([](const auto& v) -> Int32 {
          using T = std::decay_t<decltype(v)>;

          if constexpr(std::is_same_v<T, bool>)
            return v ? 1 : 0;
          else if constexpr(std::is_arithmetic_v<T>)
            return static_cast<Int32>(v);
          else if constexpr(std::is_convertible_v<T, string_view>) {
            Int32 result{};
            auto sv = string_view(v);
            const auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), result);
            return (ec == std::errc{}) ? result : 0;
          }
          else
            return 0;
        }, myValue);
      }
      return *myCachedInt;
    }

    float toFloat() const {
      if(!myCachedFloat) {
        myCachedFloat = std::visit([](const auto& v) -> float {
          using T = std::decay_t<decltype(v)>;

          if constexpr(std::is_arithmetic_v<T>)
            return static_cast<float>(v);
          else if constexpr(std::is_convertible_v<T, string_view>) {
            float result{};
            auto sv = string_view(v);
            const auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), result);
            return (ec == std::errc{}) ? result : 0.F;
          }
          else
            return 0.F;
        }, myValue);
      }
      return *myCachedFloat;
    }

    double toDouble() const {
      if(!myCachedDouble) {
        myCachedDouble = std::visit([](const auto& v) -> double {
          using T = std::decay_t<decltype(v)>;

          if constexpr(std::is_arithmetic_v<T>) {
            return static_cast<double>(v);
          }
          else if constexpr(std::is_convertible_v<T, string_view>) {
            double result{};
            auto sv = string_view(v);
            auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), result);
            return (ec == std::errc{}) ? result : 0.0;
          }
          else
            return 0.0;
        }, myValue);
      }
      return *myCachedDouble;
    }

    bool toBool() const {
      if(!myCachedBool) {
        myCachedBool = std::visit([](const auto& v) -> bool {
          using T = std::decay_t<decltype(v)>;

          if constexpr(std::is_same_v<T, bool>)
            return v;
          else if constexpr(std::is_arithmetic_v<T>)
            return v != 0;
          else if constexpr(std::is_convertible_v<T, string_view>) {
            auto sv = string_view(v);
            return sv == "1" || sv == "true" || sv == "TRUE";
          }
          else
            return false;
        }, myValue);
      }
      return *myCachedBool;
    }

    Common::Size toSize() const {
      if(const auto* p = std::get_if<Common::Size>(&myValue)) return *p;
      return Common::Size(toString());
    }
    Common::Point toPoint() const {
      if(const auto* p = std::get_if<Common::Point>(&myValue)) return *p;
      return Common::Point(toString());
    }

    // Comparison
    std::partial_ordering operator<=>(const Variant& other) const {
      // Compare numerics directly if both are numeric
      constexpr auto isNumeric = [](const Value& v) {
        return std::holds_alternative<Int32>(v)  ||
               std::holds_alternative<uInt32>(v) ||
               std::holds_alternative<float>(v)  ||
               std::holds_alternative<double>(v);
        };
      if(isNumeric(myValue) && isNumeric(other.myValue))
        return toDouble() <=> other.toDouble();

      return myValue <=> other.myValue;
    }
    bool operator==(const Variant& other) const = default;

    friend ostream& operator<<(ostream& os, const Variant& v) {
      return os << v.toString();
    }

  private:
    Value myValue;

    mutable std::optional<string> myCachedString;
    mutable std::optional<Int32>  myCachedInt;
    mutable std::optional<float>  myCachedFloat;
    mutable std::optional<double> myCachedDouble;
    mutable std::optional<bool>   myCachedBool;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline const Variant& EmptyVariant() { static const Variant empty; return empty; }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
using VariantList = vector<std::pair<string,Variant>>;

namespace VarList {
  /**
   * Push a key/value pair into a VariantList.
   * Supports any type for the key and value that can be converted to Variant.
   * Moves temporaries when possible.
   */
  template<typename KeyT, typename ValueT = Variant>
  inline void push_back(VariantList& list, KeyT&& key, ValueT&& value = {})
  {
    string keyStr;
    if constexpr(std::is_same_v<std::decay_t<KeyT>, Variant>)
      keyStr = std::forward<KeyT>(key).toString();
    else if constexpr(std::is_convertible_v<KeyT, string>)
      keyStr = std::forward<KeyT>(key);
    else
      keyStr = Variant(std::forward<KeyT>(key)).toString();

    list.emplace_back(std::move(keyStr), Variant(std::forward<ValueT>(value)));
  }
}  // namespace VarList

#endif
