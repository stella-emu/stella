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
// Copyright (c) 1995-2024 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "CartELFWidget.hxx"

#include "CartELF.hxx"
#include "Widget.hxx"
#include "StringParser.hxx"
#include "ScrollBarWidget.hxx"
#include "StringListWidget.hxx"

CartridgeELFWidget::CartridgeELFWidget(GuiObject* boss, const GUI::Font& lfont,
                       const GUI::Font& nfont,
                       int x, int y, int w, int h,
                       CartridgeELF& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h)
{
  addBaseInformation(cart.myImageSize, "AtariAge", "see log below", 1);

  const auto lineHeight = lfont.getLineHeight();
  const auto logWidth = _w - 12;
  constexpr uInt32 visibleLogLines = 20;

  const StringParser parser(
    cart.getDebugLog(),
    (logWidth - ScrollBarWidget::scrollBarWidth(lfont)) / lfont.getMaxCharWidth()
  );

  const auto& logLines = parser.stringList();
  const bool useScrollbar = logLines.size() > visibleLogLines;

  auto logWidget = new StringListWidget(
    boss, lfont, 2, (9 * lineHeight) / 2, logWidth, visibleLogLines * lineHeight, false, useScrollbar
  );

  logWidget->setEditable(false);
  logWidget->setEnabled(true);
  logWidget->setList(logLines);
}
