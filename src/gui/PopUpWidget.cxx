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
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "Font.hxx"
#include "ContextMenu.hxx"
#include "DialogContainer.hxx"
#include "PopUpWidget.hxx"

// Little up/down arrow
#ifndef FLAT_UI
static uInt32 up_down_arrows[8] = {
  0b00000000,
  0b00001000,
  0b00011100,
  0b00111110,
  0b00000000,
  0b00111110,
  0b00011100,
  0b00001000,
};
#else
static uInt32 down_arrow[8] = {
  0b100000001,
  0b110000011,
  0b111000111,
  0b011101110,
  0b001111100,
  0b000111000,
  0b000010000,
  0b000000000
};
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PopUpWidget::PopUpWidget(GuiObject* boss, const GUI::Font& font,
                         int x, int y, int w, int h, const VariantList& list,
                         const string& label, int labelWidth, int cmd)
  : Widget(boss, font, x, y - 1, w, h + 2),
    CommandSender(boss),
    _label(label),
    _labelWidth(labelWidth)
{
  _flags = WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS;
  _bgcolor = kDlgColor;
  _bgcolorhi = kDlgColor;     // do not highlight the label
  _textcolor = kTextColor;
  _textcolorhi = kTextColor;  // do not highlight the label

  if(!_label.empty() && _labelWidth == 0)
    _labelWidth = _font.getStringWidth(_label);

#ifndef FLAT_UI
  _w = w + _labelWidth + 15;
#else
  _w = w + _labelWidth + 23;
#endif

  // vertically center the arrows and text
  myTextY   = (_h - _font.getFontHeight()) / 2;
  myArrowsY = (_h - 8) / 2;

  myMenu = make_unique<ContextMenu>(this, font, list, cmd, w);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::addItems(const VariantList& items)
{
  myMenu->addItems(items);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::setSelected(const Variant& tag, const Variant& def)
{
  myMenu->setSelected(tag, def);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::setSelectedIndex(int idx)
{
  myMenu->setSelectedIndex(idx);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::setSelectedMax()
{
  myMenu->setSelectedMax();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::clearSelection()
{
  myMenu->clearSelection();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int PopUpWidget::getSelected() const
{
  return myMenu->getSelected();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& PopUpWidget::getSelectedName() const
{
  return myMenu->getSelectedName();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Variant& PopUpWidget::getSelectedTag() const
{
  return myMenu->getSelectedTag();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
  if(isEnabled() && !myMenu->isVisible())
  {
    // Add menu just underneath parent widget
    const GUI::Rect& image = instance().frameBuffer().imageRect();
    const GUI::Rect& srect = dialog().surface().dstRect();
    uInt32 tx = srect.x(), ty = srect.y();
    tx += getAbsX() + _labelWidth - image.x();
    ty += getAbsY() + getHeight() - image.y();
    myMenu->show(tx, ty, myMenu->getSelected());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::handleMouseWheel(int x, int y, int direction)
{
  if(isEnabled() && !myMenu->isVisible())
  {
    if(direction < 0)
      myMenu->sendSelectionUp();
    else
      myMenu->sendSelectionDown();
  }
}

#ifdef FLAT_UI
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::handleMouseEntered(int button)
{
  setFlags(WIDGET_HILITED);
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::handleMouseLeft(int button)
{
  clearFlags(WIDGET_HILITED);
  setDirty();
}
#endif

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
    case Event::UIUp:
    case Event::UILeft:
    case Event::UIPgUp:
      return myMenu->sendSelectionUp();
    case Event::UIDown:
    case Event::UIRight:
    case Event::UIPgDown:
      return myMenu->sendSelectionDown();
    case Event::UIHome:
      return myMenu->sendSelectionFirst();
    case Event::UIEnd:
      return myMenu->sendSelectionLast();
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
                 isEnabled() ? _textcolor : uInt32(kColor), TextAlign::Right);

#ifndef FLAT_UI
  // Draw a thin frame around us.
  s.hLine(x, _y, x + w - 1, kColor);
  s.hLine(x, _y +_h-1, x + w - 1, kShadowColor);
  s.vLine(x, _y, _y+_h-1, kColor);
  s.vLine(x + w - 1, _y, _y +_h - 1, kShadowColor);
#else
  s.frameRect(x, _y, w, _h, kColor);
  s.frameRect(x + w - 16, _y + 1, 15, _h - 2, hilite ? kTextColorHi : kBGColorLo);
#endif // !FLAT_UI

#ifndef FLAT_UI
  // Fill the background
  s.fillRect(x + 1, _y + 1, w - 2, _h - 2, kWidColor);
  // Draw an arrow pointing down at the right end to signal this is a dropdown/popup
  s.drawBitmap(up_down_arrows, x+w - 10, _y + myArrowsY,
               !isEnabled() ? kColor : hilite ? kTextColorHi : kTextColor);
#else
  // Fill the background
  s.fillRect(x + 1, _y + 1, w - 17, _h - 2, kWidColor);
  s.fillRect(x + w - 15, _y + 2, 13, _h - 4, kBGColorHi);
  //s.vLine(x + w - 17, _y, _y + _h - 1, kShadowColor);
  // Draw an arrow pointing down at the right end to signal this is a dropdown/popup
  s.drawBitmap(down_arrow, x + w - 13, _y + myArrowsY + 1,
               !isEnabled() ? kColor : kTextColor, 9u, 8u);
#endif

  // Draw the selected entry, if any
  const string& name = myMenu->getSelectedName();
  TextAlign align = (_font.getStringWidth(name) > w-6) ?
                     TextAlign::Right : TextAlign::Left;
  s.drawString(_font, name, x+2, _y+myTextY, w-6,
               !isEnabled() ? kColor : kTextColor, align);
}
