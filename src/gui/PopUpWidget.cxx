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
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "bspf.hxx"

#include "FrameBuffer.hxx"
#include "ContextMenu.hxx"
#include "DialogContainer.hxx"

#include "PopUpWidget.hxx"

#define UP_DOWN_BOX_HEIGHT	10

// Little up/down arrow
static unsigned int up_down_arrows[8] = {
  0x00000000,
  0x00001000,
  0x00011100,
  0x00111110,
  0x00000000,
  0x00111110,
  0x00011100,
  0x00001000,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PopUpWidget::PopUpWidget(GuiObject* boss, const GUI::Font& font,
                         int x, int y, int w, int h, const StringMap& list,
                         const string& label, int labelWidth, int cmd)
  : Widget(boss, font, x, y - 1, w, h + 2),
    CommandSender(boss),
    _label(label),
    _labelWidth(labelWidth)
{
  _flags = WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS;
  _type = kPopUpWidget;
  _bgcolor = kDlgColor;
  _bgcolorhi = kWidColor;
  _textcolor = kTextColor;
  _textcolorhi = kTextColor;

  if(!_label.empty() && _labelWidth == 0)
    _labelWidth = _font->getStringWidth(_label);

  _w = w + _labelWidth + 15;

  // vertically center the arrows and text
  myTextY   = (_h - _font->getFontHeight()) / 2;
  myArrowsY = (_h - 8) / 2;

  myMenu = new ContextMenu(this, font, list, cmd);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PopUpWidget::~PopUpWidget()
{
  delete myMenu;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
  if(isEnabled())
  {
    // Add menu just underneath parent widget
    const GUI::Rect& image = instance().frameBuffer().imageRect();
    uInt32 tx, ty;
    dialog().surface().getPos(tx, ty);
    tx += getAbsX() + _labelWidth - image.x();
    ty += getAbsY() + getHeight() - image.y();
    myMenu->show(tx, ty, myMenu->getSelected());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PopUpWidget::handleEvent(Event::Type e)
{
  if(!isEnabled())
    return false;

  switch(e)
  {
    case Event::UISelect:
      handleMouseDown(0, 0, 1, 0);
      return true;
    default:
      return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  // Intercept all events sent through the PromptWidget
  // They're likely from our ContextMenu, indicating a redraw is required
  dialog().setDirty();

  // Pass the cmd on to our parent
  sendCommand(cmd, data, id);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::drawWidget(bool hilite)
{
//cerr << "PopUpWidget::drawWidget\n";
  FBSurface& s = dialog().surface();

  int x = _x + _labelWidth;
  int w = _w - _labelWidth;

  // Draw the label, if any
  if (_labelWidth > 0)
    s.drawString(_font, _label, _x, _y + myTextY, _labelWidth,
                 isEnabled() ? _textcolor : kColor, kTextAlignRight);

  // Draw a thin frame around us.
  s.hLine(x, _y, x + w - 1, kColor);
  s.hLine(x, _y +_h-1, x + w - 1, kShadowColor);
  s.vLine(x, _y, _y+_h-1, kColor);
  s.vLine(x + w - 1, _y, _y +_h - 1, kShadowColor);

  // Fill the background
  s.fillRect(x + 1, _y + 1, w - 2, _h - 2, kWidColor);

  // Draw an arrow pointing down at the right end to signal this is a dropdown/popup
  s.drawBitmap(up_down_arrows, x+w - 10, _y + myArrowsY,
               !isEnabled() ? kColor : hilite ? kTextColorHi : kTextColor);

  // Draw the selected entry, if any
  const string& name = myMenu->getSelectedName();
  TextAlignment align = (_font->getStringWidth(name) > w-6) ?
                         kTextAlignRight : kTextAlignLeft;
  s.drawString(_font, name, x+2, _y+myTextY, w-6,
               !isEnabled() ? kColor : kTextColor, align);
}
