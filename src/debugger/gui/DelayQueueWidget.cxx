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
#include "DelayQueueIterator.hxx"
#include "OSystem.hxx"
#include "TIATypes.hxx"
#include "Debugger.hxx"
#include "Base.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DelayQueueWidget::DelayQueueWidget(
    GuiObject* boss,
    const GUI::Font& font,
    int x, int y
  ) : Widget(boss, font, x, y, 0, 0)
{
  _textcolor = kTextColor;

  _w = 20 * font.getMaxCharWidth() + 6;
  _h = 3 * font.getLineHeight() + 6;

  myLines[0] = myLines[1] = myLines[2];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelayQueueWidget::loadConfig() {
  shared_ptr<DelayQueueIterator> delayQueueIterator =
      instance().debugger().tiaDebug().delayQueueIterator();

  using Common::Base;
  for (uInt8 i = 0; i < 3; i++) {
    if (delayQueueIterator->isValid() && delayQueueIterator->address() < 64) {
      stringstream ss;

      ss
        << int(delayQueueIterator->delay())
        << " clk, $"
        << Base::toString(delayQueueIterator->value(), Base::Format::F_16_2)
        << " -> "
        << instance().debugger().cartDebug().getLabel(
              delayQueueIterator->address(), false);

      myLines[i] = ss.str();
      delayQueueIterator->next();
    }
    else
      myLines[i] = "";
  }
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
  surface.fillRect(x, y, w - 1, _h - 2, kBGColorLo);

  y += 2;
  x += 2;
  w -= 3;
  surface.drawString(_font, myLines[0], x, y, w, _textcolor);

  y += lineHeight;
  surface.drawString(_font, myLines[1], x, y, w, _textcolor);

  y += lineHeight;
  surface.drawString(_font, myLines[2], x, y, w, _textcolor);
}
