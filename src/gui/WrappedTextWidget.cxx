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

#include "Font.hxx"
#include "StringParser.hxx"
#include "WrappedTextWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WrappedTextWidget::WrappedTextWidget(GuiObject* boss, const GUI::Font& font,
                                     int x, int y, int w, int h,
                                     string_view text, uInt16 maxLines)
  : StringListWidget(boss, font, x, y, w, h, false, true),
    myText{text},
    myMaxLines{maxLines}
{
  rewrap();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WrappedTextWidget::setWidth(int w)
{
  StringListWidget::setWidth(w);
  rewrap();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WrappedTextWidget::refreshFontMetrics()
{
  StringListWidget::refreshFontMetrics();
  rewrap();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WrappedTextWidget::rewrap()
{
  // Nothing sensible to wrap into until the width is known
  if(_w <= 0)
    return;

  // Wrap into however many characters fit the current drawable width (the
  // scrollbar has already been subtracted from _w)
  const StringParser bs(myText, std::max(_w / _fontWidth, 1));
  const StringList& lines = bs.stringList();
  setList(lines);

  // Grow to fit the text, up to the line cap; the scrollbar covers any overflow
  const int shown = std::min(std::max<int>(static_cast<int>(lines.size()), 3),
                             static_cast<int>(myMaxLines));
  StringListWidget::setHeight(shown * _lineHeight + 2);
}
