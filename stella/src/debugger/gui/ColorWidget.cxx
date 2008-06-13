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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: ColorWidget.cxx,v 1.9 2008-06-13 13:14:50 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "bspf.hxx"

#include "Command.hxx"
#include "FrameBuffer.hxx"
#include "GuiObject.hxx"
#include "OSystem.hxx"

#include "ColorWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ColorWidget::ColorWidget(GuiObject* boss, const GUI::Font& font,
                         int x, int y, int w, int h, int cmd)
  : Widget(boss, font, x, y, w, h),
    CommandSender(boss),
    _color(0),
    _cmd(cmd)
{
  _flags = WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS;
  _type = kColorWidget;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ColorWidget::~ColorWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ColorWidget::setColor(int color)
{
  _color = color;
  setDirty(); draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ColorWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
// TODO - add ColorDialog, which will show all 256 colors in the
//         TIA palette
//  if(isEnabled())
//    parent()->addDialog(myColorDialog);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ColorWidget::drawWidget(bool hilite)
{
  FBSurface& s = dialog().surface();

  // Draw a thin frame around us.
  s.hLine(_x, _y, _x + _w - 1, kColor);
  s.hLine(_x, _y +_h, _x + _w - 1, kShadowColor);
  s.vLine(_x, _y, _y+_h, kColor);
  s.vLine(_x + _w - 1, _y, _y +_h - 1, kShadowColor);

  // Show the currently selected color
  s.fillRect(_x+1, _y+1, _w-2, _h-1, _color);
}
