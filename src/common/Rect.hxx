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

#ifndef RECT_HXX
#define RECT_HXX

#include <cassert>

#include "bspf.hxx"

namespace Common {

// Shared parsing helper
template <typename T>
requires std::integral<T>
inline bool parsePair(string_view s, T& a, T& b) {
  const char* begin = s.data();
  const char* end   = begin + s.size();

  const auto [ptr, ec] = std::from_chars(begin, end, a);
  if (ec != std::errc{} || ptr == end || *ptr != 'x')
    return false;
  const auto [ptr2, ec2] = std::from_chars(ptr + 1, end, b);
  return ec2 == std::errc{} && ptr2 == end;
}
// Shared toString helper
template <typename T>
requires std::integral<T>
inline string toStringPair(const T& a, const T& b) {
  static_assert(sizeof(T) <= 4, "toStringPair buffer too small");
  char buf[32];
  char* ptr = buf;

  auto [p1, ec] = std::to_chars(ptr, buf + sizeof(buf), a);
  if(ec != std::errc{}) return "";
  *p1++ = 'x';
  const auto [p2, ec2] = std::to_chars(p1, buf + sizeof(buf), b);
  if(ec2 != std::errc{}) return "";

  return string(buf, p2);
}

/*
  This small class is an helper for position and size values.
*/
struct Point
{
  Int32 x{0};  //!< The horizontal part of the point
  Int32 y{0};  //!< The vertical part of the point

  constexpr Point() = default;
  explicit constexpr Point(Int32 x1, Int32 y1) : x{x1}, y{y1} { }
  explicit Point(string_view p) { parse(p); }

  auto operator<=>(const Point&) const = default;
  bool operator==(const Point&) const = default;

  [[nodiscard]] string toString() const { return toStringPair(x, y); }

  friend std::ostream& operator<<(std::ostream& os, const Point& p) {
    os << p.x << "x" << p.y;
    return os;
  }

private:
  void parse(string_view p) {
    if(!parsePair(p, x, y))
      x = y = 0;
  }
};

struct Size
{
  uInt32 w{0};  //!< The width part of the size
  uInt32 h{0};  //!< The height part of the size

  constexpr Size() = default;
  explicit constexpr Size(uInt32 w1, uInt32 h1) : w{w1}, h{h1} { }
  explicit Size(string_view s) { parse(s); }
  [[nodiscard]] constexpr bool valid() const { return w > 0 && h > 0; }

  constexpr void clamp(uInt32 lower_w, uInt32 upper_w, uInt32 lower_h, uInt32 upper_h) {
    w = BSPF::clamp(w, lower_w, upper_w);
    h = BSPF::clamp(h, lower_h, upper_h);
  }

  std::partial_ordering operator<=>(const Size& other) const {
    const bool ge = w >= other.w && h >= other.h;
    const bool le = w <= other.w && h <= other.h;
    if(ge && le) return std::partial_ordering::equivalent;
    if(ge)       return std::partial_ordering::greater;
    if(le)       return std::partial_ordering::less;
    return std::partial_ordering::unordered;
  }
  bool operator==(const Size& other) const {
    return w == other.w && h == other.h;
  }

  [[nodiscard]] string toString() const { return toStringPair(w, h); }

  friend std::ostream& operator<<(std::ostream& os, const Size& s) {
    os << s.w << "x" << s.h;
    return os;
  }

private:
  void parse(string_view s) {
    if(!parsePair(s, w, h))
      w = h = 0;
  }
};

/*
  This small class is an helper for rectangles.
  Note: This implementation is built around the assumption that (top,left) is
  part of the rectangle, but (bottom,right) is not! This is reflected in
  various methods, including contains(), intersects() and others.

  Another very wide spread approach to rectangle classes treats (bottom,right)
  also as a part of the rectangle.

  Conceptually, both are sound, but the approach we use saves many intermediate
  computations (like computing the height in our case is done by doing this:
    height = bottom - top;
  while in the alternate system, it would be
    height = bottom - top + 1;

  When writing code using our Rect class, always keep this principle in mind!

  Based on code from ScummVM - Scumm Interpreter
  Copyright (C) 2002-2004 The ScummVM project
*/
struct Rect
{
private:
  //!< The point at the top left of the rectangle (part of the rect).
  uInt32 top{0}, left{0};
  //!< The point at the bottom right of the rectangle (not part of the rect).
  uInt32 bottom{0}, right{0};

public:
  constexpr Rect() = default;
  constexpr explicit Rect(const Size& s) : bottom{s.h}, right{s.w} { assert(valid()); }
  constexpr Rect(uInt32 w, uInt32 h) : bottom{h}, right{w} { assert(valid()); }
  constexpr Rect(const Point& p, uInt32 w, uInt32 h)
    : top(p.y), left(p.x), bottom(p.y + h), right(p.x + w) { assert(valid()); }
  constexpr Rect(uInt32 x1, uInt32 y1, uInt32 x2, uInt32 y2) : top{y1}, left{x1}, bottom{y2}, right{x2} { assert(valid()); }

  [[nodiscard]] constexpr uInt32 x() const    { return left; }
  [[nodiscard]] constexpr uInt32 y() const    { return top;  }
  [[nodiscard]] constexpr Point point() const { return Point(x(), y()); }

  [[nodiscard]] constexpr uInt32 w() const  { return right - left; }
  [[nodiscard]] constexpr uInt32 h() const  { return bottom - top; }
  [[nodiscard]] constexpr Size size() const { return Size(w(), h()); }

  constexpr void setWidth(uInt32 width)    { right = left + width;  }
  constexpr void setHeight(uInt32 height)  { bottom = top + height; }
  constexpr void setSize(const Size& size) { setWidth(size.w); setHeight(size.h); }

  constexpr void setBounds(uInt32 x1, uInt32 y1, uInt32 x2, uInt32 y2) {
    top = y1;
    left = x1;
    bottom = y2;
    right = x2;
    assert(valid());
  }

  [[nodiscard]] constexpr bool valid() const {
    return (left <= right && top <= bottom);
  }

  [[nodiscard]] constexpr bool empty() const {
    return w() == 0 || h() == 0;
  }

  constexpr void moveTo(uInt32 x, uInt32 y) {
    const uInt32 width  = right - left;
    const uInt32 height = bottom - top;
    left = x;
    top = y;
    right = x + width;
    bottom = y + height;
  }

  constexpr void moveTo(const Point& p) {
    moveTo(p.x, p.y);
  }

  [[nodiscard]] constexpr bool contains(uInt32 x, uInt32 y) const {
    return x >= left && y >= top && x < right && y < bottom;
  }

  // Tests whether 'r' is completely contained within this rectangle.
  // If it isn't, then set 'x' and 'y' such that moving 'r' to this
  // position will make it be contained.
  constexpr bool adjustToFit(uInt32& x, uInt32& y, const Rect& r) const {
    if(r.left < left)          x = left;
    else if(r.right > right)   x = r.left - (r.right - right);
    if(r.top < top)            y = top;
    else if(r.bottom > bottom) y = r.top - (r.bottom - bottom);

    return r.left != x || r.top != y;
  }

  auto operator<=>(const Rect& r) const = default;
  bool operator==(const Rect&) const = default;

  friend std::ostream& operator<<(std::ostream& os, const Rect& r) {
    os << r.point() << "," << r.size();
    return os;
  }
};

} // namespace Common

#endif  // RECT_HXX
