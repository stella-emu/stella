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

#include "Font.hxx"
#include "RomWidget.hxx"
#include "EditTextWidget.hxx"
#include "WrappedTextWidget.hxx"
#include "Layout.hxx"
#include "CartDebugWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartDebugWidget::CartDebugWidget(GuiObject* boss, const GUI::Font& lfont,
                                 const GUI::Font& nfont,
                                 int x, int y, int w, int h)
  : Widget(boss, lfont, x, y, w, h),
    CommandSender(boss),
    _nfont{nfont},
    myFontWidth{lfont.getMaxCharWidth()},
    myFontHeight{lfont.getFontHeight()},
    myLineHeight{lfont.getLineHeight()},
    myButtonHeight{myLineHeight + 4}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartDebugWidget::addBaseInformation(size_t bytes, string_view manufacturer,
        string_view desc, uInt16 maxlines)
{
  const int lwidth = _font.getStringWidth("Manufacturer "),
            fwidth = _w - lwidth - 12;
  EditTextWidget* w = nullptr;

  constexpr int x = 2;
  int y = 8;

  // Add ROM size, manufacturer and bankswitch info
  new StaticTextWidget(_boss, _font, x, y + 1, "ROM size ");

  w = new EditTextWidget(_boss, _nfont, x+lwidth, y - 1, fwidth, myLineHeight,
    bytes >= 1024
      ? std::format("{} bytes / {}KB", bytes, bytes / 1024)
      : std::format("{} bytes", bytes));
  w->setEditable(false);
  y += myLineHeight + 4;

  new StaticTextWidget(_boss, _font, x, y + 1, "Manufacturer ");
  w = new EditTextWidget(_boss, _nfont, x+lwidth, y - 1,
                         fwidth, manufacturer);
  w->setEditable(false);
  y += myLineHeight + 4;

  // The description wraps itself to the width given here
  new StaticTextWidget(_boss, _font, x, y + 1, "Description ");
  myDesc = new WrappedTextWidget(_boss, _nfont, x+lwidth, y - 1,
                                 fwidth, 1, desc, maxlines);
  myDesc->setEditable(false);
  myDesc->setEnabled(false);

  y += myDesc->getHeight() + 4;

  return y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebugWidget::createBaseInformation(size_t bytes, string_view manufacturer,
        string_view desc, uInt16 maxlines)
{
  // Everything is created at a placeholder position; layoutBaseInformation()
  // positions it, and the description re-wraps itself, at reflow() time
  myROMSizeLabel = new StaticTextWidget(_boss, _font, 0, 0, "ROM size ");
  myROMSize = new EditTextWidget(_boss, _nfont, 0, 0, 1, _lineHeight,
    bytes >= 1024
      ? std::format("{} bytes / {}KB", bytes, bytes / 1024)
      : std::format("{} bytes", bytes));
  myROMSize->setEditable(false);

  myManufacturerLabel = new StaticTextWidget(_boss, _font, 0, 0, "Manufacturer ");
  myManufacturer = new EditTextWidget(_boss, _nfont, 0, 0, 1,
                                      manufacturer);
  myManufacturer->setEditable(false);

  myDescLabel = new StaticTextWidget(_boss, _font, 0, 0, "Description ");
  myDesc = new WrappedTextWidget(_boss, _nfont, 0, 0, 1, 1, desc, maxlines);
  myDesc->setEditable(false);
  myDesc->setEnabled(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebugWidget::layoutBaseInformation(GUI::BoxLayout& col)
{
  using GUI::labeledRow;

  const int lwidth = _font.getStringWidth("Manufacturer ");

  // Give the description its width up front so it can re-wrap and report the
  // height its row needs (word wrap couples width to height).  This must match
  // the field width the column below hands it: content width minus the label
  myDesc->setWidth(_w - HBORDER - RBORDER - lwidth);

  col.addFixed(labeledRow(myROMSizeLabel, myROMSize, lwidth, 0, true),
               myROMSize->getHeight());
  col.addFixed(labeledRow(myManufacturerLabel, myManufacturer, lwidth, 0, true),
               myManufacturer->getHeight());
  col.addFixed(labeledRow(myDescLabel, myDesc, lwidth, 0, true),
               myDesc->getHeight());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebugWidget::setArea(int x, int y, int w, int h)
{
  Widget::setArea(x, y, w, h);
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebugWidget::invalidate()
{
  sendCommand(RomWidget::kInvalidateListing, -1, -1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebugWidget::loadConfig()
{
}
