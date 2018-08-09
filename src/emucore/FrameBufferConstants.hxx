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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef FRAMEBUFFER_CONSTANTS_HXX
#define FRAMEBUFFER_CONSTANTS_HXX

#include "bspf.hxx"

// Return values for initialization of framebuffer window
enum class FBInitStatus {
  Success,
  FailComplete,
  FailTooLarge,
  FailNotSupported
};

// Positions for onscreen/overlaid messages
enum class MessagePosition {
  TopLeft,
  TopCenter,
  TopRight,
  MiddleLeft,
  MiddleCenter,
  MiddleRight,
  BottomLeft,
  BottomCenter,
  BottomRight
};

// Colors indices to use for the various GUI elements
// Abstract away what a color actually is, so we can easily change it in
// the future, if necessary
using ColorId = uInt32;
static constexpr ColorId
  kColor = 256,
  kBGColor = 257,
  kBGColorLo = 258,
  kBGColorHi = 259,
  kShadowColor = 260,
  kTextColor = 261,
  kTextColorHi = 262,
  kTextColorEm = 263,
  kTextColorInv = 264,
  kDlgColor = 265,
  kWidColor = 266,
  kWidColorHi = 267,
  kWidFrameColor = 268,
  kBtnColor = 269,
  kBtnColorHi = 270,
  kBtnBorderColor = 271,
  kBtnBorderColorHi = 272,
  kBtnTextColor = 273,
  kBtnTextColorHi = 274,
  kCheckColor = 275,
  kScrollColor = 276,
  kScrollColorHi = 277,
  kSliderColor = 278,
  kSliderColorHi = 279,
  kSliderBGColor = 280,
  kSliderBGColorHi = 281,
  kSliderBGColorLo = 282,
  kDbgChangedColor = 283,
  kDbgChangedTextColor = 284,
  kDbgColorHi = 285,
  kDbgColorRed = 286,
  kColorInfo = 287,
  kColorTitleBar = 288,
  kColorTitleText = 289,
  kColorTitleBarLo = 290,
  kColorTitleTextLo = 291,
  kNumColors = 292,
  kNone = 0  // placeholder to represent default/no color
;

// Text alignment modes for drawString()
enum class TextAlign {
  Left,
  Center,
  Right
};

// Line types for drawing rectangular frames
enum class FrameStyle {
  Solid,
  Dashed
};

#endif // FRAMEBUFFER_CONSTANTS_HXX
