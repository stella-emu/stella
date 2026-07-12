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

#ifndef EDIT_TEXT_WIDGET_HXX
#define EDIT_TEXT_WIDGET_HXX

#include "Rect.hxx"
#include "EditableWidget.hxx"

/* EditTextWidget */
class EditTextWidget : public EditableWidget
{
  public:
    EditTextWidget(GuiObject* boss, const GUI::Font& font,
                   int x, int y, int w, int h, string_view text = "");

    /**
      A box of a single line: I take my height from the font, so a dialog states
      only how WIDE a field it needs (which is its own decision — see calcWidth).
      For a box of several lines, pass calcHeight(font, lines) to the ctor above.
    */
    EditTextWidget(GuiObject* boss, const GUI::Font& font,
                   int x, int y, int w, string_view text = "");

    ~EditTextWidget() override = default;

    void setText(string_view str, bool changed = false) override;

    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;

    // The frame insets the text by the same amount however tall the box is.  A
    // box built to show several lines (EventMappingWidget's two-line "Action"
    // box) keeps its first line at the top, so the lines below it follow on;
    // centering would leave the first line floating in the middle of the box.
    // For a box of a single line this IS the centered position
    int firstTextY() const override {
      return (_font.getLineHeight() + 2 - _font.getFontHeight()) / 2;
    }

    void refreshFontMetrics() override;

    // Get total width of widget
    static int calcWidth(const GUI::Font& font, int length = 0)
    {
      return length * font.getMaxCharWidth()
        + (font.getFontHeight() < 24 ? 3 * 2 : 5 * 2);
    }
    // Get total width of widget
    static int calcWidth(const GUI::Font& font, string_view str)
    {
      return calcWidth(font, static_cast<int>(str.size()));
    }
    // The height of a box built to show this many lines.  A dialog says it in
    // LINES; the pixels (and the frame the ctor adds) are the widget's business
    static int calcHeight(const GUI::Font& font, int lines = 1)
    {
      return font.getLineHeight() + font.getFontHeight() * (lines - 1);
    }

  protected:
    void drawWidget(bool hilite) override;
    void lostFocusWidget() override;

    void endEditMode() override;
    void abortEditMode() override;

    Common::Rect getEditRect() const override;

  protected:
    bool   _changed{false};
    int    _textOfs{0};
    // Lines of text the box was built to show, so a font change can restore a
    // multi-line height rather than collapsing the box to a single line
    int    _lines{1};

  private:
    // Following constructors and assignment operators not supported
    EditTextWidget() = delete;
    EditTextWidget(const EditTextWidget&) = delete;
    EditTextWidget(EditTextWidget&&) = delete;
    EditTextWidget& operator=(const EditTextWidget&) = delete;
    EditTextWidget& operator=(EditTextWidget&&) = delete;
};

#endif  // EDIT_TEXT_WIDGET_HXX
