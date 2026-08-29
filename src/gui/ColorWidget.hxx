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

#ifndef COLOR_WIDGET_HXX
#define COLOR_WIDGET_HXX

class ColorDialog;
class GuiObject;

#include "Widget.hxx"
#include "Command.hxx"

/**
  Displays a color from the TIA palette.  This class will eventually
  be expanded with a TIA palette table, to set the color visually.

  @author  Stephen Anthony
*/
class ColorWidget : public Widget, public CommandSender
{
  friend class ColorDialog;

  public:
    /**
      Take this size.  For a swatch that must match something beside it rather
      than stand at its own natural size -- the developer dialog's sits at a
      width of its own choosing, the TIA tab's is inset within its row.
    */
    ColorWidget(GuiObject* boss, const GUI::Font& font,
                int w, int h, bool framed = true);

    /**
      Size me from my own font (see calcWidth/calcHeight below), which is what a
      swatch simply showing a colour wants -- so the dialog states no size at all.
    */
    ColorWidget(GuiObject* boss, const GUI::Font& font, bool framed = true);
    ~ColorWidget() override = default;

    /**
      The shape a colour sample wants: a line of text tall, and half again as
      wide as it is tall.  Named here so that a dialog sizing a swatch to
      something else states its own multiple against the same height, rather
      than each one re-deriving the house proportion from the font.
    */
    static int calcHeight(const GUI::Font& font) {
      return font.getLineHeight();
    }
    static int calcWidth(const GUI::Font& font) {
      return calcHeight(font) * 1.5;
    }

    void setColor(ColorId color);
    ColorId getColor() const { return _color;  }

    // Draws an X across the swatch (e.g. to mark "no color" in a grid)
    void setCrossed(bool enable);

    // A swatch doesn't highlight on hover, unlike a plain Widget
    void handleMouseEntered() override { }
    void handleMouseLeft() override { }

  protected:
    // Fills the swatch with _color (framed or not), then an X if _crossGrid
    void drawWidget(bool hilite) override;

  protected:
    // The color currently shown
    ColorId _color{kNone};
    // Whether a frame is drawn around the swatch
    bool _framed{true};

    // Whether setCrossed() has marked this swatch
    bool _crossGrid{false};

  private:
    // Following constructors and assignment operators not supported
    ColorWidget() = delete;
    ColorWidget(const ColorWidget&) = delete;
    ColorWidget(ColorWidget&&) = delete;
    ColorWidget& operator=(const ColorWidget&) = delete;
    ColorWidget& operator=(ColorWidget&&) = delete;
};

#endif  // COLOR_WIDGET_HXX
