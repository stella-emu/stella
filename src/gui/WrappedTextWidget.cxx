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
                                     string_view text, uInt16 maxLines,
                                     uInt16 minLines)
  : StringListWidget(boss, font, false, true),
    myText{text},
    myMaxLines{maxLines},
    myMinLines{minLines}
{
  rewrap();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WrappedTextWidget::setContent(string_view text)
{
  if(myText != text)
  {
    myText = text;
    rewrap();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WrappedTextWidget::setWidth(int w)
{
  StringListWidget::setWidth(w);
  rewrap();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WrappedTextWidget::refreshFont()
{
  StringListWidget::refreshFont();
  rewrap();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WrappedTextWidget::rewrap()
{
  // Wrap to the width a row is actually DRAWN in; anything wider (_w, say) and the
  // renderer ellipsizes the longest line
  const int usable = textWidth();

  // No width to wrap into yet, so keep the line count we had (0 before the first
  // wrap), which makes naturalSize() fall back to the floor
  if(usable <= 0)
    return;

  const StringParser bs(myText, std::max(usable / _fontWidth, 1));
  const StringList& lines = bs.stringList();
  setList(lines);
  myLines = I32(lines.size());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size WrappedTextWidget::naturalSize() const
{
  // As many lines as the text came to, never fewer than the floor we always
  // show and never more than the cap beyond which we scroll
  const int shown = std::clamp(myLines, I32(myMinLines),
                               I32(myMaxLines));

  return Common::Size(std::max(_w, 0), heightForLines(shown));
}
