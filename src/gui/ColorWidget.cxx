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

#include "bspf.hxx"
#include "Command.hxx"
#include "Dialog.hxx"
#include "FBSurface.hxx"
#include "GuiObject.hxx"
#include "OSystem.hxx"
#include "ColorWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ColorWidget::ColorWidget(GuiObject* boss, const GUI::Font& font,
                         int x, int y, int w, int h, int cmd)
  : Widget(boss, font, x, y, w, h),
    CommandSender(boss),
    _color(0),
    _cmd(cmd),
    _crossGrid(false)
{
  _flags = WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ColorWidget::setColor(int color)
{
  _color = color;
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ColorWidget::drawWidget(bool hilite)
{
  FBSurface& s = dialog().surface();

  // Draw a thin frame around us.
#ifndef FLAT_UI
  s.hLine(_x, _y, _x + _w - 1, kColor);
  s.hLine(_x, _y +_h, _x + _w - 1, kShadowColor);
  s.vLine(_x, _y, _y+_h, kColor);
  s.vLine(_x + _w - 1, _y, _y +_h - 1, kShadowColor);
#else
  s.frameRect(_x, _y, _w, _h + 1, kColor);
#endif

  // Show the currently selected color
  s.fillRect(_x+1, _y+1, _w-2, _h-1, isEnabled() ? _color : kWidColor);

  // Cross out the grid?
  if(_crossGrid)
  {
#ifndef FLAT_UI
    for(uInt32 row = 1; row < 4; ++row)
      s.hLine(_x, _y + (row * _h/4), _x + _w - 2, kColor);
#else
    s.line(_x + 1, _y + 1, _x + _w - 2, _y + _h - 1, kColor);
    s.line(_x + _w - 2, _y + 1, _x + 1, _y + _h - 1, kColor);
#endif
  }
}
