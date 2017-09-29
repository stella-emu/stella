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

#include "FlashWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FlashWidget::FlashWidget(GuiObject* boss, const GUI::Font& font,
                         int x, int y, Controller& controller)
  : ControllerWidget(boss, font, x, y, controller) {}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FlashWidget::init(GuiObject* boss, const GUI::Font& font, int x, int y)
{
  bool leftport = myController.jack() == Controller::Left;
  const string& label = (leftport ? "Left (" : "Right (") + getName() + ")";

  const int fontHeight = font.getFontHeight(),
    lineHeight = font.getLineHeight(),
    bwidth = font.getStringWidth("Erase EEPROM area") + 20,
    bheight = lineHeight + 4;

  int xpos = x, ypos = y, lwidth = font.getStringWidth("Right (" + getName() + ")");
  StaticTextWidget* t;

  t = new StaticTextWidget(boss, font, xpos, ypos + 2, lwidth,
                           fontHeight, label, kTextAlignLeft);

  ypos += t->getHeight() + 8;

  myEEPROMEraseCurrent =
    new ButtonWidget(boss, font, xpos, ypos, bwidth, bheight,
                     "Erase EEPROM range", kEEPROMEraseCurrent);
  myEEPROMEraseCurrent->setTarget(this);

  ypos += lineHeight + 8;

  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  lwidth = ifont.getMaxCharWidth() * 20;

  new StaticTextWidget(boss, ifont, xpos, ypos, lwidth,
                       fontHeight, "(*) Erases only the", kTextAlignLeft);
  ypos += lineHeight + 2;
  new StaticTextWidget(boss, ifont, xpos, ypos, lwidth,
                       fontHeight, "current ROM's range", kTextAlignLeft);
  ypos += lineHeight + 8;

  myEEPROMEraseAll =
    new ButtonWidget(boss, font, xpos, ypos, bwidth, bheight,
                     "Erase EEPROM", kEEPROMEraseAll);
  myEEPROMEraseAll->setTarget(this);
  ypos += lineHeight + 8;

  new StaticTextWidget(boss, ifont, xpos, ypos, lwidth,
                       fontHeight, "(*) This will erase", kTextAlignLeft);
  ypos += lineHeight + 2;
  new StaticTextWidget(boss, ifont, xpos, ypos, lwidth,
                       fontHeight, "all EEPROM data!", kTextAlignLeft);

  updateButtonState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FlashWidget::handleCommand(CommandSender*, int cmd, int, int)
{
  if(cmd == kEEPROMEraseAll) {
    eraseAll();
  }
  if(cmd == kEEPROMEraseCurrent) {
    eraseCurrent();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FlashWidget::drawWidget(bool hilite)
{
  updateButtonState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FlashWidget::updateButtonState()
{
  myEEPROMEraseCurrent->setEnabled(isPageDetected());
}