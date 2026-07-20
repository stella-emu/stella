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

#ifndef WRAPPED_TEXT_WIDGET_HXX
#define WRAPPED_TEXT_WIDGET_HXX

#include "StringListWidget.hxx"

namespace GUI {
  class Font;
}  // namespace GUI

/**
  A read-only, word-wrapping block of text.

  Unlike StringListWidget (which renders a fixed list of strings, one per row),
  this widget owns its raw text and re-wraps it to its current width whenever it
  is resized or the font changes.  So a dialog only has to resize it — the
  wrapping is the widget's own concern.

  Its HEIGHT, however, is the layout's: this is squeezable content (see the
  Auto-vs-Stretch note in Layout.cxx), so it is placed in a stretching cell that
  it never has to be told the size of.  It states only the two ends of that
  range — minHeight() and naturalSize() — and scrolls whatever does not fit.
  That is what keeps a column holding one measurable BEFORE anything has been
  given a width, which a widget that sized itself to its wrap could not be: the
  line count is not known until the width is, and the width arrives last.

  @author  Stephen Anthony
*/
class WrappedTextWidget : public StringListWidget
{
  public:
    WrappedTextWidget(GuiObject* boss, const GUI::Font& font,
                      int x, int y, int w, int h,
                      string_view text = "", uInt16 maxLines = 10);
    ~WrappedTextWidget() override = default;

    void setWidth(int w) override;
    void refreshFontMetrics() override;

    /**
      How tall I want to be with none of my text hidden: as many lines as the
      last wrap came to, capped at 'maxLines'.  This DOES depend on my width,
      so it means nothing until I have been given one — before that it is the
      floor below.  Use it as a stretching cell's maximum, so that a roomy tab
      shows the whole text and no more.
    */
    Common::Size naturalSize() const override;

    /**
      The least I am prepared to be, with the scrollbar covering the rest.
      Unlike naturalSize() this does NOT depend on my width, so it is what a
      column holding me can be measured by before it has been laid out.  Use it
      as a stretching cell's base size.
    */
    int minHeight() const { return heightForLines(MIN_LINES); }

  private:
    // Re-wrap the text to the current width.  Note this does NOT resize us:
    // our height belongs to the layout (see the class comment)
    void rewrap();

    int heightForLines(int lines) const { return lines * _lineHeight + 2; }

    // However tall we get, we always show at least this much of the text
    static constexpr int MIN_LINES = 4;

    // The raw, unwrapped text and the most lines to show before scrolling
    string myText;
    uInt16 myMaxLines{10};

    // Lines the last wrap came to; 0 until we have been given a width
    int myLines{0};

  private:
    // Following constructors and assignment operators not supported
    WrappedTextWidget() = delete;
    WrappedTextWidget(const WrappedTextWidget&) = delete;
    WrappedTextWidget(WrappedTextWidget&&) = delete;
    WrappedTextWidget& operator=(const WrappedTextWidget&) = delete;
    WrappedTextWidget& operator=(WrappedTextWidget&&) = delete;
};

#endif  // WRAPPED_TEXT_WIDGET_HXX
