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

#include "EditTextWidget.hxx"
#include "GuiObject.hxx"
#include "CartDebug.hxx"
#include "StringParser.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "StringListWidget.hxx"
#include "ScrollBarWidget.hxx"
#include "CartDebugWidget.hxx"
#include "CartRamWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartRamWidget::CartRamWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartDebugWidget& cartDebug)
  : Widget(boss, lfont, x, y, w, h),
    CommandSender(boss),
    _nfont{nfont}
{
  const int lwidth = lfont.getStringWidth("Description "),
            fwidth = w - lwidth - 20;

  constexpr int xpos = 2;
  int ypos = 8;

  // Add RAM size
  new StaticTextWidget(_boss, _font, xpos, ypos + 1, "RAM size ");

  const uInt32 ramsize = cartDebug.internalRamSize();
  const string ramsizeStr = ramsize >= 1024
    ? std::format("{} bytes / {}KB", ramsize, ramsize / 1024)
    : std::format("{} bytes", ramsize);

  auto* etw = new EditTextWidget(boss, nfont, xpos+lwidth, ypos - 1,
                                 fwidth, _lineHeight, ramsizeStr);
  etw->setEditable(false);
  ypos += _lineHeight + 4;

  // Add Description
  const string& desc = cartDebug.internalRamDescription();
  constexpr uInt16 maxlines = 6;
  const StringParser bs(desc, (fwidth - ScrollBarWidget::scrollBarWidth(_font)) / _fontWidth);
  const StringList& sl = bs.stringList();

  bool useScrollbar = false;
  auto lines = std::max<uInt32>(static_cast<uInt32>(sl.size()), 2);
  if(lines > maxlines)
  {
    lines = maxlines;
    useScrollbar = true;
  }

  new StaticTextWidget(_boss, _font, xpos, ypos + 1, "Description ");
  myDesc = new StringListWidget(boss, nfont, xpos+lwidth, ypos - 1,
                                fwidth, lines * _lineHeight, false, useScrollbar);
  myDesc->setEditable(false);
  myDesc->setEnabled(false);
  myDesc->setList(sl);

  ypos += myDesc->getHeight() + lfont.getFontHeight() / 2;

  // Add RAM widget
  myRam = new InternalRamWidget(boss, lfont, nfont, 2, ypos, w, h-ypos, cartDebug);
  addToFocusList(myRam->getFocusList());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartRamWidget::loadConfig()
{
  myRam->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartRamWidget::setOpsWidget(DataGridOpsWidget* w)
{
  myRam->setOpsWidget(w);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartRamWidget::setArea(int x, int y, int w, int h)
{
  Widget::setArea(x, y, w, h);

  // The RAM view is a sibling widget which fills whatever is left below the
  // size and description fields, so it does not move but does resize
  myRam->setArea(2, myRam->getTop(), w, h - myRam->getTop());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartRamWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  myRam->handleCommand(sender, cmd, data, id);
}

///////////////////////////////////
// Internal RAM implementation
///////////////////////////////////

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartRamWidget::InternalRamWidget::InternalRamWidget(GuiObject* boss,
        const GUI::Font& lfont, const GUI::Font& nfont,
        int x, int y, int w, int h,
        CartDebugWidget& dbg)
  : RamWidget(boss, lfont, nfont, x, y, w, h,
      dbg.internalRamSize(), std::min(dbg.internalRamSize() / 16, 16U),
      std::min(dbg.internalRamSize() / 16, 16U) * 16, "CartridgeRAMInformation"),
    myCart(dbg)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartRamWidget::InternalRamWidget::getValue(int addr) const
{
  return myCart.internalRamGetValue(addr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartRamWidget::InternalRamWidget::setValue(int addr, uInt8 value)
{
  myCart.internalRamSetValue(addr, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartRamWidget::InternalRamWidget::getLabel(int addr) const
{
  return myCart.internalRamLabel(addr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartRamWidget::InternalRamWidget::fillList(uInt32 start, uInt32 size,
          IntArray& alist, IntArray& vlist, BoolArray& changed) const
{
  const ByteArray& oldRam  = myCart.internalRamOld(start, size);
  const ByteArray& currRam = myCart.internalRamCurrent(start, size);

  for(uInt32 i = 0; i < size; ++i)
    alist.push_back(i + start);

  std::ranges::copy(currRam, std::back_inserter(vlist));
  std::ranges::transform(currRam, oldRam, std::back_inserter(changed),
                         std::not_equal_to{});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartRamWidget::InternalRamWidget::readPort(uInt32 start) const
{
  return myCart.internalRamRPort(start);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartRamWidget::InternalRamWidget::currentRam(uInt32 start) const
{
  return myCart.internalRamCurrent(start, myCart.internalRamSize());
}
