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
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int lineHeight = font.getLineHeight();
  const int iLineHeight = ifont.getLineHeight();
  int xpos = x, ypos = y;

  bool leftport = myController.jack() == Controller::Left;
  new StaticTextWidget(boss, font, xpos, ypos + 2, (leftport ? "Left (" : "Right (") + getName() + ")");

  ypos += lineHeight + 8;

  myEEPROMEraseCurrent = new ButtonWidget(boss, font, xpos, ypos,
                                          "Erase EEPROM range", kEEPROMEraseCurrent);
  myEEPROMEraseCurrent->setTarget(this);
  ypos += lineHeight + 8;
  new StaticTextWidget(boss, ifont, xpos, ypos, "(*) Erases only the");
  ypos += iLineHeight;
  new StaticTextWidget(boss, ifont, xpos, ypos, "current ROM's range");

  ypos += iLineHeight + 8;

  myEEPROMEraseAll = new ButtonWidget(boss, font, xpos, ypos, 
                                      myEEPROMEraseCurrent->getWidth(), myEEPROMEraseCurrent->getHeight(),
                                      "Erase EEPROM", kEEPROMEraseAll);
  myEEPROMEraseAll->setTarget(this);
  ypos += lineHeight + 8;
  new StaticTextWidget(boss, ifont, xpos, ypos, "(*) This will erase");
  ypos += iLineHeight;
  new StaticTextWidget(boss, ifont, xpos, ypos, "all EEPROM data!");

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