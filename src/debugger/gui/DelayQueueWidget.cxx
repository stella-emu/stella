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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "DelayQueueWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DelayQueueWidget::DelayQueueWidget(
    GuiObject* boss,
    const GUI::Font& font,
    int x, int y
  ) : Widget(boss, font, x, y, 0, 0)
{
  _textcolor = kTextColor;

  _w = 300;
  _h = 3 * font.getLineHeight() + 6;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelayQueueWidget::drawWidget(bool hilite)
{
  FBSurface& surface = _boss->dialog().surface();

  int y = _y,
      x = _x,
      w = _w,
      lineHeight = _font.getLineHeight();

  surface.frameRect(x, y, w, _h, kShadowColor);

  y += 1;
  x += 1;
  w -= 1;
  surface.fillRect(x, y, w - 1, _h - 2, kWidColor);

  y += 2;
  x += 2;
  w -= 3;
  surface.drawString(_font, "1 clk, 40 -> GRP0", x, y, w, _textcolor);

  y += lineHeight;
  surface.drawString(_font, "3 clk, 02 -> VSYNC", x, y, w, _textcolor);

  y += lineHeight;
  surface.drawString(_font, "6 clk, 02 -> HMOVE", x, y, w, _textcolor);
}