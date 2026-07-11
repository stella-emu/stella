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
  is resized or the font changes, sizing its own height to the wrapped line
  count up to 'maxLines' (a scrollbar covers any overflow).  So a dialog only
  has to resize it — the wrapping is the widget's own concern.

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

  private:
    // Re-wrap the text to the current width and size to the resulting lines
    void rewrap();

    // The raw, unwrapped text and the most lines to show before scrolling
    string myText;
    uInt16 myMaxLines{10};

  private:
    // Following constructors and assignment operators not supported
    WrappedTextWidget() = delete;
    WrappedTextWidget(const WrappedTextWidget&) = delete;
    WrappedTextWidget(WrappedTextWidget&&) = delete;
    WrappedTextWidget& operator=(const WrappedTextWidget&) = delete;
    WrappedTextWidget& operator=(WrappedTextWidget&&) = delete;
};

#endif  // WRAPPED_TEXT_WIDGET_HXX
