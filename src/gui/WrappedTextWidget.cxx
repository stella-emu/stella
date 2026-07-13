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
  // Wrap into however many characters fit the width a row is actually DRAWN in.
  // Wrap to anything wider -- _w, say -- and the longest line does not fit its
  // row, so the renderer ELLIPSIZES it, eating the end of the very word that
  // wrapping should have carried whole onto the next line
  const int usable = textWidth();

  // Nothing sensible to wrap into until we have a width to speak of
  if(usable <= 0)
    return;

  const StringParser bs(myText, std::max(usable / _fontWidth, 1));
  const StringList& lines = bs.stringList();
  setList(lines);

  // Grow to fit the text, up to the line cap; the scrollbar covers any overflow
  const int shown = std::min(std::max<int>(static_cast<int>(lines.size()), 3),
                             static_cast<int>(myMaxLines));
  StringListWidget::setHeight(shown * _lineHeight + 2);
}
