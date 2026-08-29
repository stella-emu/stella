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

#ifndef ICON_HXX
#define ICON_HXX

#include "bspf.hxx"

namespace GUI {

struct IconDesc
{
  // Icon size in pixels
  int width{0};
  int height{0};

  explicit constexpr IconDesc(int _width, int _height)
    : width{_width}, height{_height} { }
};

/**
  A monochrome bitmap: one uInt32 per row, most significant bit leftmost, so
  an icon is at most 32 pixels wide.

  The geometry travels with the rows, so nothing that draws an Icon has to be
  told its size separately -- see FBSurface::drawIcon().

  An Icon is a view over rows that live elsewhere, in a constexpr array which
  outlives it; the Icon itself is constexpr too, so a header full of them
  costs nothing at run time.
*/
class Icon
{
  public:
    constexpr Icon(IconDesc desc, SpanOf<uInt32> bitmap)
      : myIconDesc{desc}, myBitmap{bitmap} { }
    // Convenience overload taking width/height directly instead of an IconDesc
    constexpr Icon(int width, int height, SpanOf<uInt32> bitmap)
      : Icon(IconDesc(width, height), bitmap) { }
    ~Icon() = default;

    // Width and height as a single descriptor
    constexpr const IconDesc& desc() const { return myIconDesc; }
    constexpr int height() const { return myIconDesc.height; }
    constexpr int width() const { return myIconDesc.width; }
    // First of 'height' rows, one uInt32 per row
    constexpr const uInt32* bitmap() const { return myBitmap.data(); }

  private:
    // This icon's size
    IconDesc myIconDesc;
    // The row data itself, owned elsewhere (see the class comment)
    SpanOf<uInt32> myBitmap;

  private:
    // Following constructors and assignment operators not supported
    Icon() = delete;
    Icon(const Icon&) = delete;
    Icon(Icon&&) = delete;
    Icon& operator=(const Icon&) = delete;
    Icon& operator=(Icon&&) = delete;
};

}  // namespace GUI

#endif  // ICON_HXX
