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

#include "Base.hxx"
#include "MT24LC256.hxx"
#include "Layout.hxx"
#include "FlashWidget.hxx"

using Common::Base;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FlashWidget::FlashWidget(GuiObject* boss, const GUI::Font& font,
                         Controller& controller)
  : ControllerWidget(boss, font, controller)
{
  myPage.fill(nullptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FlashWidget::init(GuiObject* boss, const GUI::Font& font,
                       bool embedded)
{
  myEmbedded = embedded;

  // Create the controls at a placeholder position; reflow() lays them out.  The
  // page ranges are filled in by loadConfig(), so they take no text here
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  myPagesLabel = new StaticTextWidget(boss, font, 0, 0,
                                      embedded ? "Pages:" : "Pages/Ranges used:");
  for(uInt32 page = 0; page < MAX_PAGES; ++page)
    myPage[page] = new StaticTextWidget(boss, font, 0, 0, page ? "" : "none");
  myEEPROMEraseCurrent = new ButtonWidget(boss, font, 0, 0,
                                          embedded ? "Erase" : "Erase used pages",
                                          kEEPROMEraseCurrent);
  myEEPROMEraseCurrent->setTarget(this);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  addFocusWidget(myEEPROMEraseCurrent);

  if(!embedded)
    createHeader();
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FlashWidget::layoutContent(GUI::BoxLayout& col)
{
  using GUI::anchoredItem;
  using GUI::indentedFill;

  const int VGAP = _font.getFontHeight() / 4;

  // The "Pages/Ranges used:" caption, then the used-page ranges (filled in by
  // loadConfig) indented under it, then the erase button
  col.addAuto(anchoredItem(myPagesLabel));
  for(auto* page: myPage)
    col.addAuto(indentedFill(page, _font.getMaxCharWidth()));
  col.addSpace(VGAP);
  col.addAuto(anchoredItem(myEEPROMEraseCurrent));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FlashWidget::handleCommand(CommandSender*, int cmd, int, int)
{
  if(cmd == kEEPROMEraseCurrent)
    eraseCurrent();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FlashWidget::loadConfig()
{
  // display the pages used by the current ROM and update erase button status
  int useCount = 0, startPage = -1;
  for(uInt32 page = 0; page < MT24LC256::PAGE_NUM; ++page)
  {
    if(isPageUsed(page))
    {
      if(startPage == -1)
        startPage = page;
    }
    else
    {
      if(startPage != -1)
      {
        const int from = startPage * MT24LC256::PAGE_SIZE;
        const int to   = page * MT24LC256::PAGE_SIZE - 1;
        string label = Base::hex3(startPage);
        if(!myEmbedded)
        {
          if(static_cast<int>(page) - 1 != startPage)
            label += std::format("-{}", Base::hex3(page - 1));
          else
            label += "    ";
          label += std::format(": {}-{}", Base::hex3(from), Base::hex4(to));
        }
        myPage[useCount]->setLabel(label);
        startPage = -1;
        if(std::cmp_equal(++useCount, MAX_PAGES))
          break;
      }
    }
  }
  myEEPROMEraseCurrent->setEnabled(useCount != 0);
}
