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

#include "AtariVox.hxx"
#include "MT24LC256.hxx"
#include "AtariVoxWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AtariVoxWidget::AtariVoxWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, Controller& controller)
  : ControllerWidget(boss, font, x, y, controller)
{
  bool leftport = myController.jack() == Controller::Left;
  const string& label = leftport ? "Left (AtariVox)" : "Right (AtariVox)";

  const int fontHeight = font.getFontHeight(),
            lineHeight = font.getLineHeight(),
            bwidth  = font.getStringWidth("Erase EEPROM") + 20,
            bheight = lineHeight + 4;

  int xpos = x, ypos = y, lwidth = font.getStringWidth("Right (AtariVox)");
  StaticTextWidget* t;

  t = new StaticTextWidget(boss, font, xpos, ypos+2, lwidth,
                           fontHeight, label, kTextAlignLeft);

  ypos += t->getHeight() + 20;
  myEEPROMErase =
    new ButtonWidget(boss, font, xpos+10, ypos, bwidth, bheight,
                     "Erase EEPROM", kEEPROMErase);
  myEEPROMErase->setTarget(this);
  ypos += lineHeight + 20;

  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  lwidth = ifont.getMaxCharWidth() * 20;
  new StaticTextWidget(boss, ifont, xpos, ypos, lwidth,
                       fontHeight, "(*) This will erase", kTextAlignLeft);
  ypos += lineHeight + 2;
  new StaticTextWidget(boss, ifont, xpos, ypos, lwidth,
                       fontHeight, "all EEPROM data, not", kTextAlignLeft);
  ypos += lineHeight + 2;
  new StaticTextWidget(boss, ifont, xpos, ypos, lwidth,
                       fontHeight, "just the range used", kTextAlignLeft);
  ypos += lineHeight + 2;
  new StaticTextWidget(boss, ifont, xpos, ypos, lwidth,
                       fontHeight, "for this ROM", kTextAlignLeft);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AtariVoxWidget::handleCommand(CommandSender*, int cmd, int, int)
{
  if(cmd == kEEPROMErase)
  {
    AtariVox& avox = static_cast<AtariVox&>(myController);
    avox.myEEPROM->erase();
  }
}
