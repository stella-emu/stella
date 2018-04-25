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

#include "Base.hxx"
#include "MT24LC256.hxx"
#include "FlashWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FlashWidget::FlashWidget(GuiObject* boss, const GUI::Font& font,
                         int x, int y, Controller& controller)
  : ControllerWidget(boss, font, x, y, controller),
    myEEPROMEraseCurrent(nullptr)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FlashWidget::init(GuiObject* boss, const GUI::Font& font, int x, int y)
{
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int lineHeight = font.getLineHeight();
  int xpos = x, ypos = y;

  new StaticTextWidget(boss, font, xpos, ypos + 2, getHeader());

  ypos += lineHeight + 6;

  new StaticTextWidget(boss, ifont, xpos, ypos, "Pages/Ranges used:");

  ypos += lineHeight + 2;
  xpos += 8;

  for(uInt32 page = 0; page < MAX_PAGES; ++page)
  {
    myPage[page] = new StaticTextWidget(boss, ifont, xpos, ypos,
        page ? "                  " : "none              ");
    ypos += lineHeight;
  }

  xpos -= 8; ypos += 2;
  myEEPROMEraseCurrent = new ButtonWidget(boss, font, xpos, ypos,
                                          "Erase used pages", kEEPROMEraseCurrent);
  myEEPROMEraseCurrent->setTarget(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FlashWidget::handleCommand(CommandSender*, int cmd, int, int)
{
  if(cmd == kEEPROMEraseCurrent) {
    eraseCurrent();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// display the pages used by the current ROM and update erase button status
void FlashWidget::loadConfig()
{
  int useCount = 0, startPage = -1;
  for(uInt32 page = 0; page < MT24LC256::PAGE_NUM; ++page)
  {
    if(isPageUsed(page))
    {
      if (startPage == -1)
        startPage = page;
    }
    else
    {
      if(startPage != -1)
      {
        int from = startPage * MT24LC256::PAGE_SIZE;
        int to = page * MT24LC256::PAGE_SIZE - 1;
        ostringstream label;

        label.str("");
        label << Common::Base::HEX3 << startPage;

        if(int(page) - 1 != startPage)
          label << "-" << Common::Base::HEX3 << page - 1;
        else
          label << "    ";
        label << ": " << Common::Base::HEX4 << from << "-" << Common::Base::HEX4 << to;

        myPage[useCount]->setLabel(label.str());

        startPage = -1;
        if(++useCount == MAX_PAGES)
          break;
      }
    }
  }

  myEEPROMEraseCurrent->setEnabled(useCount != 0);
}
