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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Font.hxx"
#include "RomWidget.hxx"
#include "EditTextWidget.hxx"
#include "StringListWidget.hxx"
#include "ScrollBarWidget.hxx"
#include "StringParser.hxx"
#include "CartDebugWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartDebugWidget::CartDebugWidget(GuiObject* boss, const GUI::Font& lfont,
                                 const GUI::Font& nfont,
                                 int x, int y, int w, int h)
  : Widget(boss, lfont, x, y, w, h),
    CommandSender(boss),
    _nfont(nfont),
    myFontWidth(lfont.getMaxCharWidth()),
    myFontHeight(lfont.getFontHeight()),
    myLineHeight(lfont.getLineHeight()),
    myButtonHeight(myLineHeight + 4),
    myDesc(nullptr)   { }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartDebugWidget::addBaseInformation(int bytes, const string& manufacturer,
        const string& desc, const uInt16 maxlines)
{
  const int lwidth = _font.getStringWidth("Manufacturer "),
            fwidth = _w - lwidth - 20;
  EditTextWidget* w = nullptr;
  ostringstream buf;

  int x = 10, y = 10;

  // Add ROM size, manufacturer and bankswitch info
  new StaticTextWidget(_boss, _font, x, y, lwidth,
        myFontHeight, "ROM Size ", TextAlign::Left);
  buf << bytes << " bytes";
  if(bytes >= 1024)
    buf << " / " << (bytes/1024) << "KB";

  w = new EditTextWidget(_boss, _nfont, x+lwidth, y,
        fwidth, myLineHeight, buf.str());
  w->setEditable(false);
  y += myLineHeight + 4;

  new StaticTextWidget(_boss, _font, x, y, lwidth,
        myFontHeight, "Manufacturer ", TextAlign::Left);
  w = new EditTextWidget(_boss, _nfont, x+lwidth, y,
        fwidth, myLineHeight, manufacturer);
  w->setEditable(false);
  y += myLineHeight + 4;

  StringParser bs(desc, (fwidth - kScrollBarWidth) / myFontWidth - 4);
  const StringList& sl = bs.stringList();
  uInt32 lines = uInt32(sl.size());
  if(lines < 3) lines = 3;
  if(lines > maxlines) lines = maxlines;

  new StaticTextWidget(_boss, _font, x, y, lwidth,
        myFontHeight, "Description ", TextAlign::Left);
  myDesc = new StringListWidget(_boss, _nfont, x+lwidth, y,
               fwidth, lines * myLineHeight, false);
  myDesc->setEditable(false);
  myDesc->setList(sl);

  y += myDesc->getHeight() + 4;

  return y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebugWidget::invalidate()
{
  sendCommand(RomWidget::kInvalidateListing, -1, -1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebugWidget::loadConfig()
{
  myDesc->setSelected(0);
}
