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

static const string formatRegister(uInt8 address)
{
  switch (address) {
    case WSYNC:
      return "WSYNC";

    case RSYNC:
      return "RSYNC";

    case VSYNC:
      return "VSYNC";

    case VBLANK:
      return "VBLANK";

    case AUDV0:
      return "AUDV0";

    case AUDV1:
      return "AUDV1";

    case AUDF0:
      return "AUDF0";

    case AUDF1:
      return "AUDF1";

    case AUDC0:
      return "AUDC0";

    case AUDC1:
      return "AUDC1";

    case HMOVE:
      return "HMOVE";

    case COLUBK:
      return "COLUBK";

    case COLUP0:
      return "COLUP0";

    case COLUP1:
      return "COLUP1";

    case COLUPF:
      return "COLUPF";

    case CTRLPF:
      return "CTRLPF";

    case PF0:
      return "PF0";

    case PF1:
      return "PF1";

    case PF2:
      return "PF2";

    case ENAM0:
      return "ENAM0";

    case ENAM1:
      return "ENAM1";

    case RESM0:
      return "RESM0";

    case RESM1:
      return "RESM1";

    case RESMP0:
      return "RESMP0";

    case RESMP1:
      return "RESMP1";

    case RESP0:
      return "RESP0";

    case RESP1:
      return "RESP1";

    case RESBL:
      return "RESBL";

    case NUSIZ0:
      return "NUSIZ0";

    case NUSIZ1:
      return "NUSIZ1";

    case HMM0:
      return "HMM0";

    case HMM1:
      return "HMM1";

    case HMP0:
      return "HMP0";

    case HMP1:
      return "HMP1";

    case HMBL:
      return "HMBL";

    case HMCLR:
      return "HMCLR";

    case GRP0:
      return "GRP0";

    case GRP1:
      return "GRP1";

    case REFP0:
      return "REFP0";

    case REFP1:
      return "REFP1";

    case VDELP0:
      return "VDELP0";

    case VDELP1:
      return "VDELP1";

    case VDELBL:
      return "VDELBL";

    case ENABL:
      return "ENABL";

    case CXCLR:
      return "CXCLR";

    default:
      return Common::Base::toString(address, Common::Base::Format::F_16_2);
  }
}

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

  myLines[0] = myLines[1] = myLines[2];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelayQueueWidget::loadConfig() {
  shared_ptr<DelayQueueIterator> delayQueueIterator = instance().debugger().tiaDebug().delayQueueIterator();

  for (uInt8 i = 0; i < 3; i++) {
    if (delayQueueIterator->isValid() && delayQueueIterator->address() < 64) {
      stringstream ss;

      ss
        << int(delayQueueIterator->delay())
        << " clk, "
        << Common::Base::toString(delayQueueIterator->value(), Common::Base::Format::F_16_2)
        << " -> "
        << formatRegister(delayQueueIterator->address());

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
  surface.fillRect(x, y, w - 1, _h - 2, kWidColor);

  y += 2;
  x += 2;
  w -= 3;
  surface.drawString(_font, myLines[0], x, y, w, _textcolor);

  y += lineHeight;
  surface.drawString(_font, myLines[1], x, y, w, _textcolor);

  y += lineHeight;
  surface.drawString(_font, myLines[2], x, y, w, _textcolor);
}